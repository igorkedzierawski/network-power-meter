#include "sampler.h"

#define _SMM_SAMPLER_RINGBUF_SHOW_PRODUCER_FUNCTIONS_
#include "sampler_ringbuf.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_system.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gptimer.h"

static const char *TAG = "Sampler";

#define ASSERT(condition, msg)                       \
    if (!condition) {                                \
        ESP_LOGE(TAG, "%s: sampler " msg, __func__); \
        return ESP_ERR_INVALID_STATE;                \
    }
#define ASSERT_INITIALIZED() ASSERT(sampler.cfg.initialized, "must be initialized")
#define ASSERT_NOT_INITIALIZED() ASSERT(!sampler.cfg.initialized, "must not be initialized")
#define ASSERT_RUNNING() ASSERT(vflag_running, "must be running")
#define ASSERT_NOT_RUNNING() ASSERT(!vflag_running, "must not be running")

static struct {
    struct {
        bool initialized;
        spi_device_handle_t *mcp_handle;
        mcp3208_command_e read_cmds[3];
        uint32_t available_channels;
        uint32_t resolution_hz;
        uint64_t sampling_interval;
        uint32_t idle_watchdog;
    } cfg; // doesnt change after init

    struct {
        StaticSemaphore_t buffer;
        SemaphoreHandle_t handle;
    } semaphore;

    gptimer_handle_t gptimer;

    uint32_t selected_channels;
} sampler = {0};

/* Indicates if the sampler is currently in sample acquisition mode. */
volatile bool vflag_running;

/* Indicates if sample acquisition should be suppressed.
 * This flag is used for low-overhead signaling from ISR to the sampler task. */
volatile bool vflag_supress_sample_acquisition;

/* Watchdog value that gets decremented every time.
 * This flag is used for low-overhead signaling from ISR to the sampler task. */
volatile uint32_t vflag_sampler_watchdog;

volatile int32_t COUNTER_ISR = 0;
volatile int32_t COUNTER_TASK = 0;

static void task(void *arg);
static bool IRAM_ATTR timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data);

esp_err_t sampler_init(const sampler_config_t *config) {
    ASSERT_NOT_INITIALIZED();
    esp_err_t ret;

    if (config == NULL) {
        ESP_LOGE(TAG, "null config");
        return ESP_ERR_INVALID_ARG;
    }

    if (config->mcp_handle == NULL) {
        ESP_LOGE(TAG, "null mcp handle");
        return ESP_ERR_INVALID_ARG;
    }
    sampler.cfg.mcp_handle = config->mcp_handle;

    if (config->timing.sampling_interval == 0) {
        ESP_LOGE(TAG, "0 sample interval");
        return ESP_ERR_INVALID_ARG;
    }
    sampler.cfg.sampling_interval = config->timing.sampling_interval;

    if (config->timing.idle_watchdog == 0) {
        ESP_LOGE(TAG, "0 idle watchdog value");
        return ESP_ERR_INVALID_ARG;
    }
    sampler.cfg.idle_watchdog = config->timing.idle_watchdog;

    sampler.cfg.available_channels = 0;
    size_t num_of_channels = 0;
    for (size_t i = 0; i < 3; i++) {
        if ((sampler.cfg.read_cmds[i] = config->read_cmds[i]) != MCP3208_CMD_NONE) {
            sampler.cfg.available_channels |= (1 << i);
            num_of_channels++;
        }
    }
    if (num_of_channels == 0) {
        ESP_LOGE(TAG, "0 channels defined");
        return ESP_ERR_INVALID_ARG;
    }

    if ((sampler.semaphore.handle = xSemaphoreCreateBinaryStatic(&sampler.semaphore.buffer)) == NULL) {
        ESP_LOGE(TAG, "failed to create semaphore");
        goto cleanup_return;
    }

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = config->timing.timer_freq_hz,
    };
    ESP_GOTO_ON_ERROR(ret = gptimer_new_timer(&timer_config, &sampler.gptimer),
                      cleanup_semaphore, TAG, "timer error: %s", esp_err_to_name(ret));
    ESP_GOTO_ON_ERROR(ret = gptimer_get_resolution(sampler.gptimer, &sampler.cfg.resolution_hz),
                      cleanup_semaphore, TAG, "timer error: %s", esp_err_to_name(ret));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_callback,
    };
    ESP_GOTO_ON_ERROR(ret = gptimer_register_event_callbacks(sampler.gptimer, &cbs, NULL),
                      cleanup_del_timer, TAG, "timer error: %s", esp_err_to_name(ret));

    ESP_GOTO_ON_ERROR(ret = gptimer_enable(sampler.gptimer),
                      cleanup_del_timer, TAG, "timer error: %s", esp_err_to_name(ret));

    if (xTaskCreatePinnedToCore(task, "Sampler", 2648, NULL, 24, NULL, config->core_id) != pdPASS) {
        ESP_LOGE(TAG, "failed to create task");
        goto cleanup_disable_timer;
    }

    sampler.cfg.initialized = true;
    vflag_running = false;
    ESP_LOGI(TAG, "initialized sampler: channels=%d, core=%d", num_of_channels, config->core_id);
    return ESP_OK;

cleanup_disable_timer:
    gptimer_disable(sampler.gptimer);
cleanup_del_timer:
    gptimer_del_timer(sampler.gptimer);
cleanup_semaphore:
    vSemaphoreDelete(sampler.semaphore.handle);
cleanup_return:
    return ESP_FAIL;
}

esp_err_t sampler_select_channels(uint32_t channels) {
    ASSERT_INITIALIZED();
    ASSERT_NOT_RUNNING();
    if ((channels & sampler.cfg.available_channels) == channels) {
        sampler.selected_channels = channels;
        return ESP_OK;
    }
    ESP_LOGE(TAG, "tried to select 0x%02X when only 0x%02X available", channels, sampler.cfg.available_channels);
    return ESP_ERR_INVALID_ARG;
}

esp_err_t sampler_get_selected_channels(uint32_t *channels_buf) {
    ASSERT_INITIALIZED();
    *channels_buf = sampler.selected_channels;
    return ESP_OK;
}

esp_err_t sampler_get_available_channels(uint32_t *channels_buf) {
    ASSERT_INITIALIZED();
    *channels_buf = sampler.cfg.available_channels;
    return ESP_OK;
}

esp_err_t sampler_get_frequency(uint32_t *freq_hz_buf) {
    ASSERT_INITIALIZED();
    *freq_hz_buf = sampler.cfg.resolution_hz / sampler.cfg.sampling_interval;
    return ESP_OK;
}

esp_err_t sampler_start() {
    ASSERT_INITIALIZED();
    ASSERT_NOT_RUNNING();
    if (xSemaphoreGive(sampler.semaphore.handle) != pdTRUE) {
        ESP_LOGE(TAG, "failed to start sampler");
        return ESP_FAIL;
    }
    COUNTER_ISR = 0;
    COUNTER_TASK = 0;
    return ESP_OK;
}

esp_err_t sampler_stop() {
    ASSERT_INITIALIZED();
    ASSERT_RUNNING();
    vflag_running = false;
    return ESP_OK;
}

esp_err_t sampler_clear_watchdog() {
    ASSERT_INITIALIZED();
    ASSERT_RUNNING();
    size_t num_of_channels = 0;
    for (size_t i = 0; i < 3; i++) {
        if (sampler.selected_channels & (1 << i)) {
            num_of_channels++;
        }
    }
    vflag_sampler_watchdog = sampler.cfg.idle_watchdog;
    if (num_of_channels != 0) {
        vflag_sampler_watchdog /= num_of_channels;
    }
    if (vflag_sampler_watchdog == 0) {
        vflag_sampler_watchdog = 1;
    }
    return ESP_OK;
}

inline bool sampler_is_init() {
    return sampler.cfg.initialized;
}

inline bool sampler_is_running() {
    return vflag_running;
}

static void task(void *arg) {
    esp_err_t ret;
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    mcp3208_command_e cmd_queue[3];
    size_t num_of_channels;
    spi_transaction_t transaction = mcp3208_init_transaction(MCP3208_CMD_NONE);
    while (sampler.cfg.initialized) {
        xSemaphoreTake(sampler.semaphore.handle, portMAX_DELAY);
        vflag_running = true;

        num_of_channels = 0;
        for (size_t i = 0; i < 3; i++) {
            if (sampler.selected_channels & (1 << i)) {
                cmd_queue[num_of_channels++] = sampler.cfg.read_cmds[i];
            }
        }
        if (num_of_channels == 0) {
            ESP_LOGE(TAG, "0 channels were selected => stopping");
            vflag_running = false;
            continue;
        }

        alarm_config.alarm_count = sampler.cfg.sampling_interval;
        if ((ret = gptimer_set_alarm_action(sampler.gptimer, &alarm_config)) != ESP_OK) {
            ESP_LOGE(TAG, "failed to configure timer: %s => stopping", esp_err_to_name(ret));
            vflag_running = false;
            continue;
        }

        if ((ret = gptimer_start(sampler.gptimer)) != ESP_OK) {
            ESP_LOGE(TAG, "failed to start timer: %s => stopping", esp_err_to_name(ret));
            vflag_running = false;
            continue;
        }

        ESP_LOGI(TAG, "Sampler started!");
        sampler_ringbuf_restart(num_of_channels);
        sampler_clear_watchdog();
        COUNTER_TASK = 0;
        spi_device_acquire_bus(*sampler.cfg.mcp_handle, portMAX_DELAY);
        while (vflag_running && vflag_sampler_watchdog) {
            vflag_supress_sample_acquisition = true;
            while (vflag_supress_sample_acquisition) {
            }
            if (!vflag_running) {
                break;
            }
            COUNTER_TASK++;
            for (size_t i = 0; i < num_of_channels; i++) {
                transaction.cmd = cmd_queue[i];
                spi_device_polling_transmit(*sampler.cfg.mcp_handle, &transaction);
                sampler_ringbuf_push_sample(transaction.rx_data);
            }
            if (COUNTER_TASK == 100000) {
                ESP_LOGI(TAG, "(%d, %d) lastRes=%d DRIFT=%d", 99, 99, 69, COUNTER_ISR - COUNTER_TASK);
                COUNTER_ISR = 0;
                COUNTER_TASK = 0;
            }
            vflag_sampler_watchdog--;
        }
        spi_device_release_bus(*sampler.cfg.mcp_handle);
        ESP_LOGI(TAG, "Sampler stopped (reason: %s)!", !vflag_sampler_watchdog ? "watchdog" : "external");
        vflag_running = 0;
        vflag_sampler_watchdog = 0;
        if ((ret = gptimer_stop(sampler.gptimer)) != ESP_OK) {
            ESP_LOGE(TAG, "failed to stop timer: %s => panic?", esp_err_to_name(ret));
            vflag_running = false;
            continue;
        }
    }
}

static bool IRAM_ATTR timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {
    vflag_supress_sample_acquisition = false;
    COUNTER_ISR++;
    return true;
}
