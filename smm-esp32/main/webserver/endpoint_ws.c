#include "endpoints.h"

#include <freertos/FreeRTOS.h>
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <esp_http_server.h>

#include "../sampler/sampler.h"
#include "../sampler/sampler_ringbuf.h"

#define ENDPOINT_WS_REQUEST_NEXT "NEXT"   /* Fetch next block */
#define ENDPOINT_WS_REQUEST_CLOSE "CLOSE" /* Close connection */

#define ENDPOINT_WS_RESPONSE_NO_BLOCKS "NO_BLOCKS"     /* No blocks available */
#define ENDPOINT_WS_RESPONSE_SAMPLER_OFF "SAMPLER_OFF" /* Sampler is not running */

static const char *TAG = "EP_WS";

typedef struct {
    httpd_handle_t handle;
    int fd;
    void *sess_ctx;
} async_response_arg_t;

static void free_sess_ctx(void *arg);
static esp_err_t send_next_block_trigger_async(httpd_handle_t handle, httpd_req_t *req);
static void send_next_block_async(void *arg);
static const samplerd_sample_block_t *try_to_load_next_block(uint32_t *next_block);

static esp_err_t ws_handler(httpd_req_t *req) {
    esp_err_t ret;
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Rozpoczęto połączenie przez WebSocket");
        ESP_RETURN_ON_FALSE((req->sess_ctx = calloc(1, sizeof(uint32_t))) != NULL, ESP_ERR_NO_MEM,
                            TAG, "couldnt allocate sess_ctx");
        req->free_ctx = free_sess_ctx;
        return ESP_OK;
    }

    uint8_t buffer[8] = {0};
    httpd_ws_frame_t packet = {0};

    ESP_RETURN_ON_ERROR(ret = httpd_ws_recv_frame(req, &packet, 0),
                        TAG, "packet length error: %s", esp_err_to_name(ret));
    ESP_RETURN_ON_FALSE(packet.len <= sizeof(buffer), ret = ESP_ERR_INVALID_SIZE,
                        TAG, "packet read error: %s", esp_err_to_name(ret));

    packet.payload = buffer;
    ESP_RETURN_ON_ERROR(ret = httpd_ws_recv_frame(req, &packet, packet.len),
                        TAG, "packet read error: %s", esp_err_to_name(ret));

    if (strncmp(ENDPOINT_WS_REQUEST_NEXT, (char *)packet.payload, packet.len) == 0) {
        ESP_RETURN_ON_ERROR(ret = send_next_block_trigger_async(req->handle, req),
                            TAG, "block send error: %s", esp_err_to_name(ret));
    }
    if (strncmp(ENDPOINT_WS_REQUEST_CLOSE, (char *)packet.payload, packet.len) == 0) {
        ESP_RETURN_ON_ERROR(ret = httpd_sess_trigger_close(req->handle, httpd_req_to_sockfd(req)),
                            TAG, "conn close error: %s", esp_err_to_name(ret));
    }
    return ESP_OK;
}

static esp_err_t send_next_block_trigger_async(httpd_handle_t handle, httpd_req_t *req) {
    esp_err_t ret;
    async_response_arg_t *response_arg;

    if ((response_arg = calloc(1, sizeof(*response_arg))) == NULL) {
        return ESP_ERR_NO_MEM;
    }

    response_arg->handle = req->handle;
    response_arg->fd = httpd_req_to_sockfd(req);
    response_arg->sess_ctx = req->sess_ctx;

    if ((ret = httpd_queue_work(handle, send_next_block_async, response_arg)) != ESP_OK) {
        free(response_arg);
        return ret;
    }
    return ESP_OK;
}

static void send_next_block_async(void *arg) {
    async_response_arg_t *response_arg = arg;
    httpd_ws_frame_t packet = {0};

    if (!sampler_is_running()) {
        packet.type = HTTPD_WS_TYPE_TEXT;
        packet.payload = (uint8_t *)ENDPOINT_WS_RESPONSE_SAMPLER_OFF;
        packet.len = strlen(ENDPOINT_WS_RESPONSE_SAMPLER_OFF);
    } else {
        const samplerd_sample_block_t *block = try_to_load_next_block((uint32_t *)response_arg->sess_ctx);
        if (block == NULL) {
            packet.type = HTTPD_WS_TYPE_TEXT;
            packet.payload = (uint8_t *)ENDPOINT_WS_RESPONSE_NO_BLOCKS;
            packet.len = strlen(ENDPOINT_WS_RESPONSE_NO_BLOCKS);
        } else {
            packet.type = HTTPD_WS_TYPE_BINARY;
            packet.payload = (uint8_t *)block;
            packet.len = sizeof(*block);
        }
    }

    httpd_ws_send_frame_async(response_arg->handle, response_arg->fd, &packet);
    free(response_arg);
}

static const samplerd_sample_block_t *try_to_load_next_block(uint32_t *block_index) {
    sampler_clear_watchdog();

    uint32_t logical_block_index = sampler_ringbuf_get_current_logical_block_unchecked();

    if (logical_block_index == *block_index) {
        // Block is currently being modified, nothing is sent
        return NULL;
    } else if (logical_block_index > *block_index) {
        // Block is in the past, adjust block_index to the oldest available if necessary
        if (logical_block_index - *block_index > (NEW_SAMPLER__BLOCK_COUNT - 1)) {
            *block_index = logical_block_index - (NEW_SAMPLER__BLOCK_COUNT - 1);
        }
    } else {
        // Block is in the future, move block_index to the newest available
        *block_index = logical_block_index > (NEW_SAMPLER__BLOCK_COUNT - 1)
                           ? logical_block_index - (NEW_SAMPLER__BLOCK_COUNT - 1)
                           : 0;
    }

    const samplerd_sample_block_t * block = sampler_ringbuf_access_block_unchecked(*block_index);
    (*block_index) = (*block_index)+1;
    return block;
}

static void free_sess_ctx(void *arg) {
    ESP_LOGI(TAG, "Zakończono połączenie przez WebSocket %lu", sizeof(samplerd_sample_block_t));
    free(arg);
}

const httpd_uri_t endpoint_ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = ws_handler,
    .user_ctx = NULL,
    .is_websocket = true,
};
