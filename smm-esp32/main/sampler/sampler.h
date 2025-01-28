#ifndef _SMM_SAMPLER_H_
#define _SMM_SAMPLER_H_

#include "driver/spi_master.h"
#include <inttypes.h>
#include "mcp3208.h"

typedef struct {
    spi_device_handle_t *mcp_handle; /* Handle for MCP3208 SPI device */
    mcp3208_command_e read_cmds[3];  /* Read commands for specific ADC channels (NONE if unused) */
    BaseType_t core_id;              /* Core ID for running the sampler task */
    struct {
        uint32_t timer_freq_hz;     /* Internal timer frequency (actual frequency may differ from this value) */
        uint64_t sampling_interval; /* Timer cycles between sample acquisitions  //TODO make this a settable value*/
        uint32_t idle_watchdog;     /* Num. of sampling cycles after which without clearing watchdog register, the sampler will stop automatically  */
    } timing;
} sampler_config_t;

/**
 * @brief  Initialize sampler with the given configuration and start the task.
 *         The sampler transitions into initialized, disabled state.
 *
 * @param  config  Pointer to sampler configuration.
 *
 * @return
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_ARG: Invalid configuration
 *         - ESP_ERR_INVALID_STATE: Sampler is already initialized
 *         - ESP_FAIL: Initialization failed
 */
esp_err_t sampler_init(const sampler_config_t *config);

// TODO
//  esp_err_t sampler_deinit();

/**
 * @brief Select channels to be sampled. The sampler must be initialized and disabled.
 *
 * @param channels Bitmask of selected channels.
 *
 * @return
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_STATE: Sampler is not in the expected state
 *         - ESP_ERR_INVALID_ARG: Selected channel does not exist
 */
esp_err_t sampler_select_channels(uint32_t channels);

/**
 * @brief Get the mask of currently selected channels.
 *        The sampler must be initialized.
 *
 * @param channels_buf Pointer to the variable to store the channel mask.
 *
 * @return
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_STATE: Sampler is not in the expected state
 */
esp_err_t sampler_get_selected_channels(uint32_t *channels_buf);

/**
 * @brief Get the mask of currently available channels.
 *        The sampler must be initialized.
 *
 * @param channels_buf Pointer to the variable to store the channel mask.
 *
 * @return
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_STATE: Sampler is not in the expected state
 */
esp_err_t sampler_get_available_channels(uint32_t *channels_buf);

/**
 * @brief Get the frequency the sampler operates at
 *
 * @param freq_hz_buf Pointer to store the maximum frequency in Hz.
 *
 * @return
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_STATE: Sampler is not in the expected state
 */
esp_err_t sampler_get_frequency(uint32_t *freq_hz_buf);

/**
 * @brief Get the current sampling period.
 *        The sampler must be initialized.
 *
 * @param period_us Pointer to store the sampling period in microseconds.
 *
 * @return
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_STATE: Sampler is not in the expected state
 */
// uint32_t sampler_get_sampling_period_us(uint32_t *period_us_buf);

/**
 * @brief Start periodic sampling from selected channels and store samples in a buffer.
 *        The sampler must be initialized and disabled.
 *
 * @return
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_STATE: Sampler is not in the expected state
 *         - ESP_FAIL: Failed to start Sampler
 */
esp_err_t sampler_start();

/**
 * @brief Stop the sampling process.
 *        The sampler must be initialized and enabled.
 *
 * @return
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_STATE: Sampler is not in the expected state
 */
esp_err_t sampler_stop();

/**
 * @brief Clear watchdog register to prevent sampler from automatically shutting down.
 *        The sampler must be initialized and enabled.
 *
 * @return
 *         - ESP_OK: Success
 *         - ESP_ERR_INVALID_STATE: Sampler is not in the expected state
 */
esp_err_t sampler_clear_watchdog();

/**
 * @brief Check if the sampler is initialized.
 */
bool sampler_is_init();

/**
 * @brief Check if the sampler is running.
 */
bool sampler_is_running();

#endif
