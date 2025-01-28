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
#include "../calibration/calibration.h"

static const char *TAG = "EP_CAL";

void serialize(const calibration_registers_t *cal, char *output) {
    sprintf(output, "Va=%.7f Vb=%.7f I1a=%.7f I1b=%.7f I2a=%.7f I2b=%.7f H=%.7f",
            cal->voltage.multiplier, cal->voltage.offset,
            cal->current1.multiplier, cal->current1.offset,
            cal->current2.multiplier, cal->current2.offset,
            cal->firstHarmonicFrequency);
}

void deserialize(const char *input, calibration_registers_t *cal) {
    sscanf(input,
           "Va=%f Vb=%f I1a=%f I1b=%f I2a=%f I2b=%f H=%f",
           &cal->voltage.multiplier, &cal->voltage.offset,
           &cal->current1.multiplier, &cal->current1.offset,
           &cal->current2.multiplier, &cal->current2.offset,
           &cal->firstHarmonicFrequency);
}

static esp_err_t get_calibration_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    char buffer[196];
    calibration_registers_t calibration;
    if (smm_calibration_get(&calibration) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "couldnt read calibration");
    }

    serialize(&calibration, buffer);
    buffer[195] = 0;
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, buffer, strnlen(buffer, sizeof(buffer)));
}

static esp_err_t set_calibration_handler(httpd_req_t *req) {
    IGNORE_CORS(req);
    char buffer[196];
    calibration_registers_t calibration;
    esp_err_t ret;

    if (req->content_len >= 196) {
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "too long calibration");
    }

    httpd_req_recv(req, buffer, 195);
    buffer[195] = 0;

    deserialize(buffer, &calibration);
    smm_calibration_set(&calibration);
    return httpd_resp_sendstr(req, "");
}

const httpd_uri_t endpoint_sampler_get_calibration = {
    .uri = "/sampler/get_calibration",
    .method = HTTP_GET,
    .handler = get_calibration_handler,
    .user_ctx = NULL,
};

const httpd_uri_t endpoint_sampler_set_calibration = {
    .uri = "/sampler/set_calibration",
    .method = HTTP_POST,
    .handler = set_calibration_handler,
    .user_ctx = NULL,
};
