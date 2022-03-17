/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
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
#if DEVICE_I2C

#include <string.h>

#include "i2c_api.h"
#include "mbed_error.h"
#include "nrfx_twim.h"

static bool nrfx_twim_in_use[TWIM_COUNT] = {false};

static void i2c_event_handler(const nrfx_twim_evt_t* event, void* context)
{
    i2c_t *obj = (i2c_t *)context;
    if (obj) {
        switch (event->type) {
            case NRFX_TWIM_EVT_DONE:
                obj->transfer_result = event->xfer_desc.primary_length;
                break;
            case NRFX_TWIM_EVT_ADDRESS_NACK:
            case NRFX_TWIM_EVT_DATA_NACK:
                obj->transfer_result = I2C_ERROR_NO_SLAVE;
                break;
            case NRFX_TWIM_EVT_OVERRUN:
            case NRFX_TWIM_EVT_BUS_ERROR:
                obj->transfer_result = I2C_ERROR_BUS_BUSY;
                break;
        }

        obj->transfer_complete = true;
    }
}

void i2c_init(i2c_t *obj, PinName sda, PinName scl)
{
    MBED_ASSERT(obj);
    memset(obj, 0, sizeof(*obj));
    obj->instance.drv_inst_idx = NRFX_TWIM_ENABLED_COUNT;

    for (uint8_t index = 0; index < TWIM_COUNT; ++index) {
        if (!nrfx_twim_in_use[index]) {
            switch (index) {
#if NRFX_CHECK(NRFX_TWIM0_ENABLED)
                case 0: {
                    nrfx_twim_t instance0 = NRFX_TWIM_INSTANCE(0);
                    memcpy(&obj->instance, &instance0, sizeof(obj->instance));
                    break;
                }
#endif
#if NRFX_CHECK(NRFX_TWIM1_ENABLED)
                case 1: {
                    nrfx_twim_t instance1 = NRFX_TWIM_INSTANCE(1);
                    memcpy(&obj->instance, &instance1, sizeof(obj->instance));
                    break;
                }
#endif
#if NRFX_CHECK(NRFX_TWIM2_ENABLED)
                case 2: {
                    nrfx_twim_t instance2 = NRFX_TWIM_INSTANCE(2);
                    memcpy(&obj->instance, &instance2, sizeof(obj->instance));
                    break;
                }
#endif
#if NRFX_CHECK(NRFX_TWIM3_ENABLED)
                case 3: {
                    nrfx_twim_t instance3 = NRFX_TWIM_INSTANCE(3);
                    memcpy(&obj->instance, &instance3, sizeof(obj->instance));
                    break;
                }
#endif
                default:
                    continue;
            }
            nrfx_twim_in_use[index] = true;
            break;
        }
    }
    MBED_ASSERT(obj->instance.drv_inst_idx < NRFX_UARTE_ENABLED_COUNT);

    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(scl, sda);
    memcpy(&obj->config, &config, sizeof(obj->config));
    nrfx_twim_init(&obj->instance, &obj->config, i2c_event_handler, obj);
    nrfx_twim_enable(&obj->instance);
}

void i2c_frequency(i2c_t *obj, int hz)
{
    nrfx_twim_uninit(&obj->instance);

    switch (hz) {
        case 100000:
            obj->config.frequency = NRF_TWIM_FREQ_100K;
            break;
        case 250000:
            obj->config.frequency = NRF_TWIM_FREQ_250K;
            break;
        case 400000:
            obj->config.frequency = NRF_TWIM_FREQ_400K;
            break;
#if NRF_TWIM_HAS_1000_KHZ_FREQ
        case 1000000:
            obj->config.frequency = NRF_TWIM_FREQ_1000K;
            break;
#endif
        default:
            MBED_WARNING1(MBED_MAKE_ERROR(MBED_MODULE_DRIVER_SERIAL, MBED_ERROR_CODE_UNSUPPORTED), "frequency not supported", hz);
            break;
    }

    nrfx_twim_init(&obj->instance, &obj->config, i2c_event_handler, obj);
    nrfx_twim_enable(&obj->instance);
}

int i2c_start(i2c_t *obj)
{
    obj->length = 0;
    obj->address = 0;
    return 0;
}

int i2c_byte_write(i2c_t *obj, int data)
{
    /* The TWIM driver does not support single byte writes so combine the bytes and send on i2c_stop() */
    if (obj->address == 0) {
        obj->address = data;
    } else if (obj->length < sizeof(obj->buffer)) {
        obj->buffer[obj->length++] = data;
    } else {
        return 0;
    }

    return 1;
}

int i2c_byte_read(i2c_t *obj, int last)
{
    return I2C_ERROR_NO_SLAVE;
}

int i2c_stop(i2c_t *obj)
{
    if (obj->length) {
        i2c_write(obj, obj->address, &obj->buffer[0], obj->length, true);
    }

    return 0;
}

void i2c_reset(i2c_t *obj)
{
    uint8_t index = obj->instance.drv_inst_idx;
    nrfx_twim_uninit(&obj->instance);
    nrfx_twim_in_use[index] = false;
}

int i2c_read(i2c_t *obj, int address, char *data, int length, int stop)
{
    while(nrfx_twim_is_busy(&obj->instance));

    obj->transfer_complete = false;
    nrfx_twim_xfer_desc_t descriptor = NRFX_TWIM_XFER_DESC_RX(address >> 1, (uint8_t *)data, length);
    if (nrfx_twim_xfer(&obj->instance, &descriptor, 0)) {
        return I2C_ERROR_BUS_BUSY;
    }

    while(!obj->transfer_complete);
    return obj->transfer_result;
}

int i2c_write(i2c_t *obj, int address, const char *data, int length, int stop)
{
    while(nrfx_twim_is_busy(&obj->instance));

    obj->transfer_complete = false;
    nrfx_twim_xfer_desc_t descriptor = NRFX_TWIM_XFER_DESC_TX(address >> 1, (uint8_t *)data, length);
    if (nrfx_twim_xfer(&obj->instance, &descriptor, stop ? 0 : NRFX_TWIM_FLAG_TX_NO_STOP)) {
        return I2C_ERROR_BUS_BUSY;
    }

    while(!obj->transfer_complete);
    return obj->transfer_result;
}

#endif // DEVICE_I2C
