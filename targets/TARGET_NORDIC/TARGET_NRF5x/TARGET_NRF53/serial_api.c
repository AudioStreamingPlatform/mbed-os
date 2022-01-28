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
#include <string.h>

#include "serial_api.h"
#include "pinmap.h"
#include "nrfx_uarte.h"

int stdio_uart_inited = 0;
serial_t stdio_uart = { 0 };

uint32_t nrfx_tx_done = 0;
uint32_t nrfx_rx_done = 0;
uint32_t nrfx_error = 0;

static void serial_event_handler(const nrfx_uarte_event_t* event, void* context)
{
    switch (event->type) {
        case NRFX_UARTE_EVT_TX_DONE:
            nrfx_tx_done++;
            break;
        case NRFX_UARTE_EVT_RX_DONE:
            nrfx_rx_done++;
            break;
        case NRFX_UARTE_EVT_ERROR:
            nrfx_error++;
            break;
    }
}

void serial_init(serial_t *obj, PinName tx, PinName rx) {
#if NRFX_CHECK(NRFX_UARTE0_ENABLED)
    nrfx_uarte_t instance = NRFX_UARTE_INSTANCE(0);
#elif NRFX_CHECK(NRFX_UARTE1_ENABLED)
    nrfx_uarte_t instance = NRFX_UARTE_INSTANCE(1);
#else
#error No NRFX_UARTE instance enabled
#endif
    MBED_ASSERT(obj);
    memcpy(&obj->instance, &instance, sizeof(obj->instance));

    nrfx_uarte_config_t config = NRFX_UARTE_DEFAULT_CONFIG(tx, rx);
    memcpy(&obj->config, &config, sizeof(obj->config));

    if (obj == &stdio_uart) {
        stdio_uart_inited = 1;
    }

    nrfx_uarte_init(&obj->instance, &obj->config, serial_event_handler);
}

void serial_free(serial_t *obj) {
    nrfx_uarte_uninit(&obj->instance);
}

void serial_baud(serial_t *obj, int baudrate) {
    // FIXME
}

void serial_format(serial_t *obj, int data_bits, SerialParity parity, int stop_bits) {
    // FIXME
}

void serial_irq_handler(serial_t *obj, uart_irq_handler handler, uint32_t id) {
    // FIXME
}

void serial_irq_set(serial_t *obj, SerialIrq irq, uint32_t enable) {
    // FIXME
}

int serial_getc(serial_t *obj) {
    while (!serial_readable(obj));
    return 0;
}

void serial_putc(serial_t *obj, int c) {
    while (!serial_writable(obj));
    const uint8_t byte = (uint8_t)c;
    nrfx_uarte_tx(&obj->instance, &byte, sizeof(byte));
}

int serial_readable(serial_t *obj) {
    return nrfx_uarte_rx_ready(&obj->instance);
}

int serial_writable(serial_t *obj) {
    return !nrfx_uarte_tx_in_progress(&obj->instance);
}

void serial_clear(serial_t *obj) {
}

void serial_break_clear(serial_t *obj) {
}

void serial_break_set(serial_t *obj) {
}
