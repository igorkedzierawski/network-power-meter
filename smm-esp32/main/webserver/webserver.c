#include "webserver.h"

#include "endpoints.h"

#include "esp_event.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <esp_http_server.h>

static const char *TAG = "HTTP_SERVER";

static httpd_handle_t server = NULL;

static esp_err_t redirect_to_index_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_status(req, "301 Moved Permanently");
    httpd_resp_set_hdr(req, "Location", "/index.html");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t scope_ep = {
    .uri = "/scope",
    .method = HTTP_GET,
    .handler = redirect_to_index_handler,
    .user_ctx = NULL,
};

esp_err_t smm_webserver_start(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 12;
    config.task_priority = tskIDLE_PRIORITY + 12;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    ESP_RETURN_ON_ERROR(httpd_start(&server, &config), TAG, "failed");

    ESP_LOGI(TAG, "Registering URI handlers");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_ws), TAG, "failed (ws)");

    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_channels_get_selected), TAG, "failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_channels_set_selected), TAG, "failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_channels_get_available), TAG, "failed");

    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_sampler_start), TAG, "failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_sampler_stop), TAG, "failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_sampler_is_running), TAG, "failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_sampler_get_info), TAG, "failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_sampler_get_calibration), TAG, "failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_sampler_set_calibration), TAG, "failed");

    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &scope_ep), TAG, "failed");

    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &endpoint_serve_file), TAG, "failed");

    ESP_LOGI(TAG, "URI handlers Registered!");

    return ESP_OK;
}