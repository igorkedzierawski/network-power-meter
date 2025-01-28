#include "endpoints.h"

#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include <esp_err.h>
#include <esp_check.h>
#include <esp_log.h>

#include <esp_http_server.h>

static const char *TAG = "EP_SF";

static void set_mime_type(httpd_req_t *req, const char *filename);

static esp_err_t serve_file_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "/spiffs%s", req->uri);
    FILE *f = fopen(buffer, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open %s", buffer);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }
    set_mime_type(req, buffer);

    size_t chunksize;
    while ((chunksize = fread(buffer, 1, sizeof(buffer), f)) != 0) {
        if (httpd_resp_send_chunk(req, buffer, chunksize) != ESP_OK) {
            fclose(f);
            ESP_LOGE(TAG, "File sending failed!");
            httpd_resp_sendstr_chunk(req, NULL);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }
    }

    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static void set_mime_type(httpd_req_t *req, const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0) {
            httpd_resp_set_type(req, "text/html");
        } else if (strcmp(ext, ".js") == 0) {
            httpd_resp_set_type(req, "application/javascript");
        } else if (strcmp(ext, ".css") == 0) {
            httpd_resp_set_type(req, "text/css");
        } else if (strcmp(ext, ".png") == 0) {
            httpd_resp_set_type(req, "image/png");
        } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
            httpd_resp_set_type(req, "image/jpeg");
        } else if (strcmp(ext, ".gif") == 0) {
            httpd_resp_set_type(req, "image/gif");
        } else if (strcmp(ext, ".ico") == 0) {
            httpd_resp_set_type(req, "image/x-icon");
        } else if (strcmp(ext, ".txt") == 0) {
            httpd_resp_set_type(req, "text/plain");
        } else if (strcmp(ext, ".json") == 0) {
            httpd_resp_set_type(req, "application/json");
        } else {
            httpd_resp_set_type(req, "application/octet-stream");
        }
    } else {
        httpd_resp_set_type(req, "application/octet-stream");
    }
}

const httpd_uri_t endpoint_serve_file = {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = serve_file_handler,
    .user_ctx = NULL,
};
