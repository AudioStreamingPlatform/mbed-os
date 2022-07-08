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
#if DEVICE_SERIAL

#include <string.h>

#include "serial_api.h"
#include "nrfx_uarte.h"
#include "mbed_error.h"

static uint32_t serial_irq_ids[UARTE_COUNT] = {0};
static uart_irq_handler irq_handler = NULL;
static bool nrfx_uarte_in_use[UARTE_COUNT] = {false};

int stdio_uart_inited = 0;
serial_t stdio_uart;

static void serial_event_handler(const nrfx_uarte_event_t* event, void* context)
{
    serial_t *obj = (serial_t *)context;

    switch (event->type) {
        case NRFX_UARTE_EVT_TX_DONE:
            if (obj && irq_handler) {
                irq_handler(serial_irq_ids[obj->instance.drv_inst_idx], TxIrq);
            }
            break;
        case NRFX_UARTE_EVT_RX_DONE:
            if (obj && obj->rx_irq_enable) {
                if (obj->rx_buffer_secondary_in_use) {
                    memcpy(&obj->rx_buffer_secondary[0], event->data.rxtx.p_data, sizeof(obj->rx_buffer_secondary));
                    obj->rx_buffer_secondary_set = true;
                    obj->rx_buffer_secondary_in_use = false;
                    if (!obj->rx_buffer_primary_set) {
                        nrfx_uarte_rx(&obj->instance, &obj->rx_buffer_primary[0], sizeof(obj->rx_buffer_primary));
                    }
                } else {
                    memcpy(&obj->rx_buffer_primary[0], event->data.rxtx.p_data, sizeof(obj->rx_buffer_primary));
                    obj->rx_buffer_primary_set = true;
                    obj->rx_buffer_secondary_in_use = true;
                    if (!obj->rx_buffer_secondary_set) {
                        nrfx_uarte_rx(&obj->instance, &obj->rx_buffer_secondary[0], sizeof(obj->rx_buffer_secondary));
                    }
                }
                if (irq_handler) {
                    irq_handler(serial_irq_ids[obj->instance.drv_inst_idx], RxIrq);
                }
            }
            break;
        case NRFX_UARTE_EVT_ERROR:
            break;
    }
}

void serial_init(serial_t *obj, PinName tx, PinName rx) {
    MBED_ASSERT(obj);
    memset(obj, 0, sizeof(*obj));
    obj->instance.drv_inst_idx = NRFX_UARTE_ENABLED_COUNT;

    for (uint8_t index = 0; index < UARTE_COUNT; ++index) {
        if (!nrfx_uarte_in_use[index]) {
            switch (index) {
#if NRFX_CHECK(NRFX_UARTE0_ENABLED)
                case 0: {
                    nrfx_uarte_t instance0 = NRFX_UARTE_INSTANCE(0);
                    memcpy(&obj->instance, &instance0, sizeof(obj->instance));
                    break;
                }
#endif
#if NRFX_CHECK(NRFX_UARTE1_ENABLED)
                case 1: {
                    nrfx_uarte_t instance1 = NRFX_UARTE_INSTANCE(1);
                    memcpy(&obj->instance, &instance1, sizeof(obj->instance));
                    break;
                }
#endif
#if NRFX_CHECK(NRFX_UARTE2_ENABLED)
                case 2: {
                    nrfx_uarte_t instance2 = NRFX_UARTE_INSTANCE(2);
                    memcpy(&obj->instance, &instance2, sizeof(obj->instance));
                    break;
                }
#endif
#if NRFX_CHECK(NRFX_UARTE3_ENABLED)
                case 3: {
                    nrfx_uarte_t instance3 = NRFX_UARTE_INSTANCE(3);
                    memcpy(&obj->instance, &instance3, sizeof(obj->instance));
                    break;
                }
#endif
                default:
                    continue;
            }
            nrfx_uarte_in_use[index] = true;
            break;
        }
    }
    MBED_ASSERT(obj->instance.drv_inst_idx < NRFX_UARTE_ENABLED_COUNT);

    nrfx_uarte_config_t config = NRFX_UARTE_DEFAULT_CONFIG(tx, rx);
    memcpy(&obj->config, &config, sizeof(obj->config));
    obj->config.p_context = (serial_t *)obj;

    if (obj == &stdio_uart) {
        stdio_uart_inited = 1;
        memcpy(&stdio_uart, obj, sizeof(serial_t));
    }

    nrfx_uarte_init(&obj->instance, &obj->config, serial_event_handler);
}

void serial_free(serial_t *obj) {
    uint8_t index = obj->instance.drv_inst_idx;
    nrfx_uarte_uninit(&obj->instance);
    nrfx_uarte_in_use[index] = false;
}

void serial_baud(serial_t *obj, int baudrate) {
    nrfx_uarte_uninit(&obj->instance);

    /* Support the most common used baud rates */
    switch (baudrate) {
        case 9600:
            obj->config.baudrate = NRF_UARTE_BAUDRATE_9600;
            break;
        case 115200:
            obj->config.baudrate = NRF_UARTE_BAUDRATE_115200;
            break;
        case 230400:
            obj->config.baudrate = NRF_UARTE_BAUDRATE_230400;
            break;
        case 460800:
            obj->config.baudrate = NRF_UARTE_BAUDRATE_460800;
            break;
        case 921600:
            obj->config.baudrate = NRF_UARTE_BAUDRATE_921600;
            break;
        case 1000000:
            obj->config.baudrate = NRF_UARTE_BAUDRATE_1000000;
            break;
        default:
            MBED_WARNING1(MBED_MAKE_ERROR(MBED_MODULE_DRIVER_SERIAL, MBED_ERROR_CODE_UNSUPPORTED), "baudrate not supported", baudrate);
            break;
    }

    nrfx_uarte_init(&obj->instance, &obj->config, serial_event_handler);
}

void serial_format(serial_t *obj, int data_bits, SerialParity parity, int stop_bits) {
    MBED_WARNING(MBED_MAKE_ERROR(MBED_MODULE_DRIVER_SERIAL, MBED_ERROR_CODE_UNSUPPORTED), "serial_format");
}

void serial_irq_handler(serial_t *obj, uart_irq_handler handler, uint32_t id) {
    irq_handler = handler;
    serial_irq_ids[obj->instance.drv_inst_idx] = id;
}

void serial_irq_set(serial_t *obj, SerialIrq irq, uint32_t enable) {
    switch (irq) {
        case RxIrq:
            obj->rx_irq_enable = (bool)enable;
            if (enable) {
                nrfx_uarte_rx(&obj->instance, &obj->rx_buffer_primary[0], sizeof(obj->rx_buffer_primary));
            }
            break;
        case TxIrq:
            obj->tx_irq_enable = (bool)enable;
            break;
    }
}

int serial_getc(serial_t *obj) {
    uint8_t character = 0;

    while (!(obj->rx_buffer_primary_set || obj->rx_buffer_secondary_set));

    if (obj->rx_buffer_primary_set) {
        character = obj->rx_buffer_primary[0];
        obj->rx_buffer_primary_set = false;

        /* Start receiving again if all buffers were set */
        if (obj->rx_buffer_secondary_set) {
            nrfx_uarte_rx(&obj->instance, &obj->rx_buffer_primary[0], sizeof(obj->rx_buffer_primary));
        }
    } else {
        character = obj->rx_buffer_secondary[0];
        obj->rx_buffer_secondary_set = false;
    }

    return character;
}

void serial_putc(serial_t *obj, int c) {
    const uint8_t character = (uint8_t)c;

    while (!serial_writable(obj));
    nrfx_uarte_tx(&obj->instance, &character, sizeof(character));
}

int serial_readable(serial_t *obj) {
    return obj->rx_buffer_primary_set || obj->rx_buffer_secondary_set;
}

int serial_writable(const serial_t *obj) {
    return !nrfx_uarte_tx_in_progress(&obj->instance);
}

void serial_clear(serial_t *obj) {
    serial_putc(obj, 0);
}

void serial_break_clear(serial_t *obj) {
    MBED_WARNING(MBED_MAKE_ERROR(MBED_MODULE_DRIVER_SERIAL, MBED_ERROR_CODE_UNSUPPORTED), "serial_break_clear");
}

void serial_break_set(serial_t *obj) {
    MBED_WARNING(MBED_MAKE_ERROR(MBED_MODULE_DRIVER_SERIAL, MBED_ERROR_CODE_UNSUPPORTED), "serial_break_set");
}

#endif // DEVICE_SERIAL
