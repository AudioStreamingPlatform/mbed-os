/* mbed Microcontroller Library
 * Copyright (c) 2016-2018 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if DEVICE_ANALOGIN

#include "hal/analogin_api.h"

#include "pinmap.h"
#include "PeripheralPins.h"
#include "nrfx_saadc.h"

#define ADC_12BIT_RANGE 0x0FFF
#define ADC_16BIT_RANGE 0xFFFF

static uint8_t channel_index = 0;

#if STATIC_PINMAP_READY
#define ANALOGIN_INIT_DIRECT analogin_init_direct
void analogin_init_direct(analogin_t *obj, const PinMap *pinmap)
#else
#define ANALOGIN_INIT_DIRECT _analogin_init_direct
static void _analogin_init_direct(analogin_t *obj, const PinMap *pinmap)
#endif
{
    nrfx_err_t result;
    static bool first_init = true;

    MBED_ASSERT(obj);
    MBED_ASSERT(pinmap->pin != NC);

    if (first_init) {
        result = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);
        MBED_ASSERT(result == NRFX_SUCCESS);
        result = nrfx_saadc_offset_calibrate(NULL);
        MBED_ASSERT(result == NRFX_SUCCESS);
        first_init = false;
    }

    nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(pinmap->function, channel_index);
    /* The 1/4 gain and VDD/4 makes the reference voltage VDD */
    channel.channel_config.gain = NRF_SAADC_GAIN1_4,
    channel.channel_config.reference = NRF_SAADC_REFERENCE_VDD4;

    result = nrfx_saadc_channel_config(&channel);
    MBED_ASSERT(result == NRFX_SUCCESS);

    obj->channel_index = channel_index;
    channel_index++;
}

void analogin_init(analogin_t *obj, PinName pin)
{
    const PinMap static_pinmap = {pin, pinmap_peripheral(pin, PinMap_ADC), pinmap_find_function(pin, PinMap_ADC)};
    ANALOGIN_INIT_DIRECT(obj, &static_pinmap);
}

uint16_t analogin_read_u16(analogin_t *obj)
{
    nrfx_err_t result;
    nrf_saadc_value_t value = 0;

    MBED_ASSERT(obj);

    /* Use simple mode (single sample conversion) in blocking mode */
    result = nrfx_saadc_simple_mode_set((1 << obj->channel_index), NRF_SAADC_RESOLUTION_12BIT, NRF_SAADC_OVERSAMPLE_DISABLED, NULL);
    MBED_ASSERT(result == NRFX_SUCCESS);

    result = nrfx_saadc_buffer_set(&value, 1);
    MBED_ASSERT(result == NRFX_SUCCESS);

    result = nrfx_saadc_mode_trigger();
    MBED_ASSERT(result == NRFX_SUCCESS);

    /* Convert the value from 12 to 16-bit as used by Mbed */
    return (((uint32_t)value) * ADC_16BIT_RANGE) / ADC_12BIT_RANGE;
}

float analogin_read(analogin_t *obj)
{
    MBED_ASSERT(obj);

    /* Read 16 bit ADC value (using Mbed API) and convert to [0;1] range float */
    return ((float)analogin_read_u16(obj) / (float)ADC_16BIT_RANGE);
}

const PinMap *analogin_pinmap()
{
    return PinMap_ADC;
}

#endif // DEVICE_ANALOGIN
