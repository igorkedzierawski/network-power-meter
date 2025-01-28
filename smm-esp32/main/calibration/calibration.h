#ifndef _SMM_CALIBRATION_H_
#define _SMM_CALIBRATION_H_

#include "esp_err.h"

typedef struct {
    float multiplier;
    float offset;
} channel_scaling_t;

typedef struct {
    channel_scaling_t voltage;
    channel_scaling_t current1;
    channel_scaling_t current2;
    float firstHarmonicFrequency;
} calibration_registers_t;

esp_err_t smm_calibration_get(calibration_registers_t *registers);

esp_err_t smm_calibration_set(calibration_registers_t *registers);

#endif