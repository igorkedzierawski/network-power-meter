#ifndef _SMM_WEBSERVER_ENDPOINTS_H_
#define _SMM_WEBSERVER_ENDPOINTS_H_

#include <esp_http_server.h>

#define IGNORE_CORS(req)                                              \
    do {                                                              \
                                                                      \
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "*"); \
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "*"); \
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");  \
    } while (0)

extern const httpd_uri_t endpoint_ws;

extern const httpd_uri_t endpoint_serve_file;

extern const httpd_uri_t endpoint_channels_get_selected;
extern const httpd_uri_t endpoint_channels_set_selected;
extern const httpd_uri_t endpoint_channels_get_available;

extern const httpd_uri_t endpoint_sampler_start;
extern const httpd_uri_t endpoint_sampler_stop;
extern const httpd_uri_t endpoint_sampler_is_running;
extern const httpd_uri_t endpoint_sampler_get_info;
extern const httpd_uri_t endpoint_sampler_get_calibration;
extern const httpd_uri_t endpoint_sampler_set_calibration;

#endif