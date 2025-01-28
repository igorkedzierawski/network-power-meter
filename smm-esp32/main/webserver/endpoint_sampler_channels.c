#include "endpoints.h"

#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include <errno.h>
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <esp_http_server.h>

#include "../sampler/sampler.h"

static const char *TAG = "EP_SCH";

static esp_err_t get_selected_channels_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    uint32_t channels;
    char buffer[32];
    if (sampler_get_selected_channels(&channels) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "sampler uninitialized");
    }

    snprintf(buffer, sizeof(buffer), "{\"channels\":%lu}", channels);
    httpd_resp_set_type(req, "application/json");

    return httpd_resp_send(req, buffer, strnlen(buffer, sizeof(buffer)));
}

static esp_err_t get_available_channels_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    uint32_t channels;
    char buffer[32];
    if (sampler_get_available_channels(&channels) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "sampler uninitialized");
        return ESP_OK;
    }

    snprintf(buffer, sizeof(buffer), "{\"channels\":%lu}", channels);
    httpd_resp_set_type(req, "application/json");

    return httpd_resp_send(req, buffer, strnlen(buffer, sizeof(buffer)));
}

static esp_err_t set_selected_channels_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    char *buf;
    size_t buf_len;
    char param_buf[16];
    uint32_t channels;
    esp_err_t ret;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len <= 1) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "no params found");
    }

    buf = malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) != ESP_OK) {
        free(buf);
        return httpd_resp_send_500(req);
    }

    memset(param_buf, 0, sizeof(param_buf));
    if (httpd_query_key_value(buf, "channels", param_buf, sizeof(param_buf)) != ESP_OK) {
        free(buf);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid channels param");
    }
    free(buf);

    ESP_LOGI(TAG, "%s", param_buf);

    channels = strtoul(param_buf, NULL, 10);
    ESP_LOGI(TAG, "%d", channels);

    ret = sampler_select_channels(channels);
    if(ret == ESP_ERR_INVALID_STATE) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "invalid sampler state");
    }
    if(ret == ESP_ERR_INVALID_ARG) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "invalid channels");
    }

    return httpd_resp_sendstr(req, "");
}

const httpd_uri_t endpoint_channels_get_selected = {
    .uri = "/sampler/channels/get_selected",
    .method = HTTP_GET,
    .handler = get_selected_channels_handler,
    .user_ctx = NULL,
};

const httpd_uri_t endpoint_channels_get_available = {
    .uri = "/sampler/channels/get_available",
    .method = HTTP_GET,
    .handler = get_available_channels_handler,
    .user_ctx = NULL,
};

const httpd_uri_t endpoint_channels_set_selected = {
    .uri = "/sampler/channels/set_selected",
    .method = HTTP_POST,
    .handler = set_selected_channels_handler,
    .user_ctx = NULL,
};
