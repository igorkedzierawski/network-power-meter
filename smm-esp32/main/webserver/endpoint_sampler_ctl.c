#include "endpoints.h"

#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include <errno.h>
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <esp_http_server.h>

#include "../sampler/sampler.h"
#include "../sampler/sampler_ringbuf.h"

static const char *TAG = "EP_CTL";

static esp_err_t get_info_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    uint32_t freq_hz;
    char buffer[64];
    if (sampler_get_frequency(&freq_hz) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "sampler uninitialized");
    }

    snprintf(buffer, sizeof(buffer), "{\"freq\":%lu,\"bsamplecapacity\":%d}", freq_hz, NEW_SAMPLER__BLOCK_SAMPLE_CAPACITY);
    httpd_resp_set_type(req, "application/json");

    return httpd_resp_send(req, buffer, strnlen(buffer, sizeof(buffer)));
}

static esp_err_t is_running_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "{\"running\":%s}", sampler_is_running() ? "true" : "false");
    httpd_resp_set_type(req, "application/json");

    return httpd_resp_send(req, buffer, strnlen(buffer, sizeof(buffer)));
}

static esp_err_t start_sampler_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    esp_err_t ret;

    ret = sampler_start();
    if(ret == ESP_ERR_INVALID_STATE) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "sampler running or not initialized");
    }
    if(ret != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "sampler failed to start");
    }
    
    return httpd_resp_sendstr(req, "");
}

static esp_err_t stop_sampler_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    esp_err_t ret;

    ret = sampler_stop();
    if(ret == ESP_ERR_INVALID_STATE) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "sampler not running");
    }
    
    return httpd_resp_sendstr(req, "");
}

const httpd_uri_t endpoint_sampler_start = {
    .uri = "/sampler/start",
    .method = HTTP_POST,
    .handler = start_sampler_handler,
    .user_ctx = NULL,
};

const httpd_uri_t endpoint_sampler_stop = {
    .uri = "/sampler/stop",
    .method = HTTP_POST,
    .handler = stop_sampler_handler,
    .user_ctx = NULL,
};

const httpd_uri_t endpoint_sampler_is_running = {
    .uri = "/sampler/is_running",
    .method = HTTP_GET,
    .handler = is_running_handler,
    .user_ctx = NULL,
};

const httpd_uri_t endpoint_sampler_get_info = {
    .uri = "/sampler/get_info",
    .method = HTTP_GET,
    .handler = get_info_handler,
    .user_ctx = NULL,
};
