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

#ifdef DEVICE_I2C_ASYNCH
#define GET_I2C_S(x) &x->i2c
#else
#define GET_I2C_S(x) x
#endif

#if DEVICE_I2CSLAVE
#include "nrfx_twis.h"

#if (NRFX_TWIS_ENABLED_COUNT < 1)
#error "Cannot use DEVICE_I2CSLAVE without any TWIS enabled"
#endif

static bool nrfx_twis_in_use[NRFX_TWIS_ENABLED_COUNT] = { false };
static struct i2c_s *global_i2c_s = NULL;

static nrfx_err_t twis_buf_req_handler(nrfx_twis_evt_type_t type,
                                nrfx_twis_t *twis_obj,
                                i2c_slave_req_buf_cb_t cb,
                                void *context)
{
    nrfx_err_t err = NRFX_ERROR_NOT_SUPPORTED;
    uint8_t* buf = NULL;
    size_t size = 0;

    if (!(twis_obj && cb))
        return err;

    /* request buffer via callback */
    cb(context, &buf, &size);
    if (!buf)
        return err;

    switch (type) {
    case NRFX_TWIS_EVT_READ_REQ:
        err = nrfx_twis_tx_prepare(twis_obj, buf, size);
        break;
    case NRFX_TWIS_EVT_WRITE_REQ:
        err = nrfx_twis_rx_prepare(twis_obj, buf, size);
        break;
    default:
        break;
    }

    return err;
}

static void twis_event_handler(nrfx_twis_evt_t const *p_event)
{
    bool report_error = false;
    i2c_slave_error error;

    if (!(global_i2c_s && p_event))
        return;

    i2c_slave_callbacks *cb_struct = &global_i2c_s->slave_callbacks;

    switch (p_event->type) {
    case NRFX_TWIS_EVT_READ_REQ:
        error.code.nrfx = twis_buf_req_handler(p_event->type,
                                               &global_i2c_s->slave_instance,
                                               cb_struct->txbuf_req,
                                               cb_struct->context);
        report_error = error.code.nrfx != NRFX_SUCCESS;
        break;
    case NRFX_TWIS_EVT_WRITE_REQ:
        error.code.nrfx = twis_buf_req_handler(p_event->type,
                                               &global_i2c_s->slave_instance,
                                               cb_struct->rxbuf_req,
                                               cb_struct->context);
        report_error = error.code.nrfx != NRFX_SUCCESS;
        break;
    case NRFX_TWIS_EVT_READ_DONE:
        if (cb_struct->tx_done)
            cb_struct->tx_done(cb_struct->context, p_event->data.rx_amount);
        break;
    case NRFX_TWIS_EVT_WRITE_DONE:
        if (cb_struct->rx_done)
            cb_struct->rx_done(cb_struct->context, p_event->data.tx_amount);
        break;
    case NRFX_TWIS_EVT_READ_ERROR:
    case NRFX_TWIS_EVT_WRITE_ERROR:
    case NRFX_TWIS_EVT_GENERAL_ERROR:
        report_error = true;
        error.code.twis = p_event->data.error;
        break;
    }

    if (report_error && cb_struct->error)
        cb_struct->error(cb_struct->context, error);
}

static nrfx_twis_t get_next_free_twis_instance(void)
{
    nrfx_twis_t instance = { .p_reg = NULL, .drv_inst_idx = 0 };

    for (uint8_t index = 0; index < NRFX_TWIS_ENABLED_COUNT; ++index) {
        if (nrfx_twis_in_use[index])
            continue;

        switch (index) {
#if NRFX_CHECK(NRFX_TWIS0_ENABLED)
        case 0: {
            nrfx_twis_t instance0 = NRFX_TWIS_INSTANCE(0);
            instance = instance0;
            break;
        }
#endif
#if NRFX_CHECK(NRFX_TWIS1_ENABLED)
        case 1: {
            nrfx_twis_t instance1 = NRFX_TWIS_INSTANCE(1);
            instance = instance1;
            break;
        }
#endif
#if NRFX_CHECK(NRFX_TWIS2_ENABLED)
        case 2: {
            nrfx_twis_t instance2 = NRFX_TWIS_INSTANCE(2);
            instance = instance2;
            break;
        }
#endif
#if NRFX_CHECK(NRFX_TWIS3_ENABLED)
        case 3: {
            nrfx_twis_t instance3 = NRFX_TWIS_INSTANCE(3);
            instance = instance3;
            break;
        }
#endif
        default:
            continue;
        }

        nrfx_twis_in_use[index] = true;
        break;
    }

    return instance;
}

static bool is_slave_active(i2c_t *obj_)
{
    struct i2c_s *obj = GET_I2C_S(obj_);
    return obj->slave_instance.p_reg && obj->slave_instance.p_reg->ENABLE;
}

static void deinit_slave(i2c_t *obj_)
{
    struct i2c_s *obj = GET_I2C_S(obj_);
    if (!obj->slave_instance.p_reg)
        return;

    nrfx_twis_disable(&obj->slave_instance);
    nrfx_twis_uninit(&obj->slave_instance);
    nrfx_twis_in_use[obj->slave_instance.drv_inst_idx] = false;
}

#endif // DEVICE_I2CSLAVE

static bool nrfx_twim_in_use[TWIM_COUNT] = { false };

static bool is_master_active(i2c_t *obj_)
{
    struct i2c_s *obj = GET_I2C_S(obj_);
    return obj->instance.p_twim && obj->instance.p_twim->ENABLE;
}

static void i2c_event_handler(const nrfx_twim_evt_t *event, void *context)
{
    i2c_t *obj_ = (i2c_t *)context;
    struct i2c_s* obj = GET_I2C_S(obj_);

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

static void deinit_master(i2c_t *obj_)
{
    struct i2c_s *obj = GET_I2C_S(obj_);
    if (!obj->instance.p_twim)
        return;

    nrfx_twim_disable(&obj->instance);
    nrfx_twim_uninit(&obj->instance);
    nrfx_twim_in_use[obj->instance.drv_inst_idx] = false;
}

void i2c_init(i2c_t *obj_, PinName sda, PinName scl)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    MBED_ASSERT(obj);
    memset(obj, 0, sizeof(*obj));
    obj->instance.drv_inst_idx = NRFX_TWIM_ENABLED_COUNT;
    obj->instance.p_twim = NULL;

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

#if DEVICE_I2CSLAVE
    obj->is_slave = false;
    obj->slave_addr = 0;
    obj->slave_instance.p_reg = NULL;
    obj->slave_callbacks.rxbuf_req = NULL;
    obj->slave_callbacks.txbuf_req = NULL;
    obj->slave_callbacks.rx_done = NULL;
    obj->slave_callbacks.tx_done = NULL;
    obj->slave_callbacks.error = NULL;
#endif // DEVICE_I2CSLAVE

    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(scl, sda);
    memcpy(&obj->config, &config, sizeof(obj->config));
}

void i2c_frequency(i2c_t *obj_, int hz)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

#if DEVICE_I2CSLAVE
    /* master instance invalid -> device is slave -> freq can't be configured */
    if (!obj->instance.p_twim)
        return;
#endif

    /* master will re-init on next use */
    if (is_master_active(obj_))
        deinit_master(obj_);

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
}

/**
 * @brief      Reconfigure driver.
 *
 *             If the peripheral is enabled, it will be disabled first. All
 *             registers are cleared to their default values unless replaced
 *             by new configuration.
 *
 * @param      obj           The object
 */
static void i2c_configure_driver_instance(i2c_t *obj_)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    /* If the peripheral is running, disable and uninitialize it.*/
    if (is_master_active(obj_))
        deinit_master(obj_);

#if DEVICE_I2CSLAVE
    if (is_slave_active(obj_))
        deinit_slave(obj_);

    /* Configure slave driver if device is slave */
    if (obj->is_slave) {
        nrfx_twis_config_t twis_config =
            NRFX_TWIS_DEFAULT_CONFIG(obj->config.scl, obj->config.sda,
                                     obj->slave_addr >> 1);

        /* FIXME: add error handling */
        nrfx_twis_init(&obj->slave_instance, &twis_config, twis_event_handler);
        nrfx_twis_enable(&obj->slave_instance);
        nrfx_twis_in_use[obj->slave_instance.drv_inst_idx] = true;

        /* FIXME: use array of i2c_s ptr instead? */
        global_i2c_s = obj;
        return;
    }
#endif // DEVICE_I2CSLAVE

    /* Configure driver as master if device is master */
    if (obj->instance.p_twim) {
        nrfx_twim_init(&obj->instance, &obj->config, i2c_event_handler, obj_);
        nrfx_twim_enable(&obj->instance);
        nrfx_twim_in_use[obj->instance.drv_inst_idx] = true;
    }
}

int i2c_start(i2c_t *obj_)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    obj->length = 0;
    obj->address = 0;

    return 0;
}

int i2c_byte_write(i2c_t *obj_, int data)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    /* return if master instance is invalid */
    if (!obj->instance.p_twim)
        return I2C_ERROR_NO_SLAVE;

    if (!is_master_active(obj_))
        i2c_configure_driver_instance(obj_);

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

int i2c_stop(i2c_t *obj_)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    if (obj->length) {
        i2c_write(obj_, obj->address, &obj->buffer[0], obj->length, true);
    }

    return 0;
}

void i2c_reset(i2c_t *obj)
{
    if (is_master_active(obj))
        deinit_master(obj);
#if DEVICE_I2CSLAVE
    if (is_slave_active(obj))
        deinit_slave(obj);
#endif // DEVICE_I2CSLAVE
}

int i2c_read(i2c_t *obj_, int address, char *data, int length, int stop)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    /* return if master instance is invalid */
    if (!obj->instance.p_twim)
        return I2C_ERROR_NO_SLAVE;

    if (!is_master_active(obj_))
        i2c_configure_driver_instance(obj_);

    while (nrfx_twim_is_busy(&obj->instance));

    obj->transfer_complete = false;
    nrfx_twim_xfer_desc_t descriptor = NRFX_TWIM_XFER_DESC_RX(address >> 1, (uint8_t *)data, length);
    if (nrfx_twim_xfer(&obj->instance, &descriptor, 0)) {
        return I2C_ERROR_BUS_BUSY;
    }

    while(!obj->transfer_complete);
    return obj->transfer_result;
}

int i2c_write(i2c_t *obj_, int address, const char *data, int length, int stop)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    /* return if master instance is invalid */
    if (!obj->instance.p_twim)
        return 0;

    if (!is_master_active(obj_))
        i2c_configure_driver_instance(obj_);

    while (nrfx_twim_is_busy(&obj->instance));

    obj->transfer_complete = false;
    nrfx_twim_xfer_desc_t descriptor = NRFX_TWIM_XFER_DESC_TX(address >> 1, (uint8_t *)data, length);
    if (nrfx_twim_xfer(&obj->instance, &descriptor, stop ? 0 : NRFX_TWIM_FLAG_TX_NO_STOP)) {
        return I2C_ERROR_BUS_BUSY;
    }

    while(!obj->transfer_complete);
    return obj->transfer_result;
}

#if DEVICE_I2CSLAVE

/** Configure I2C as slave or master.
 *  @param obj_ The I2C object
 *  @param enable_slave Enable i2c hardware in slave mode
 *  @return non-zero if a value is available
 */
void i2c_slave_mode(i2c_t *obj_, int enable_slave)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    /* Reconfigure this instance as an I2C slave */
    obj->is_slave = enable_slave;

    if (obj->is_slave && !obj->slave_instance.p_reg) {
        obj->slave_instance = get_next_free_twis_instance();
    }

    i2c_configure_driver_instance(obj_);
}

/** Check to see if the I2C slave has been addressed.
 *  @param obj The I2C object
 *  @return The status - 1 - read addresses, 2 - write to all slaves,
 *         3 write addressed, 0 - the slave has not been addressed
 */
int i2c_slave_receive(i2c_t *obj_)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    /* return if slave instance is invalid */
    if (!obj->slave_instance.p_reg)
        return I2C_ERROR_NO_SLAVE;

    if (nrfx_twis_is_waiting_rx_buff(&obj->slave_instance)) {
        return 3;
    }

    if (nrfx_twis_is_waiting_tx_buff(&obj->slave_instance)) {
        return 1;
    }

    return 0;
}

/** Configure I2C as slave or master.
 *  @param obj The I2C object
 *  @param data    The buffer for receiving
 *  @param length  Number of bytes to read
 *  @return non-zero if a value is available
 */
int i2c_slave_read(i2c_t *obj_, char *data, int length)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    /* return if slave instance is invalid */
    if (!obj->slave_instance.p_reg)
        return 0;

    const nrfx_twis_t *twis_instance = &obj->slave_instance;

    /* Wait until the master is trying to write data */
    while (!nrfx_twis_is_waiting_rx_buff(twis_instance)) { }

    /* Master is attempting to write, now we prepare the rx buffer */
    nrfx_twis_rx_prepare(twis_instance, data, length);

    /* Wait until the transaction is over */
    while (nrfx_twis_is_pending_rx(twis_instance)) { }

    return nrfx_twis_rx_amount(twis_instance);
}

/** Configure I2C as slave or master.
 *  @param obj The I2C object
 *  @param data    The buffer for sending
 *  @param length  Number of bytes to write
 *  @return non-zero if a value is available
 */
int i2c_slave_write(i2c_t *obj_, const char* data, int length)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    /* return if slave instance is invalid */
    if (!obj->slave_instance.p_reg)
        return 0;

    const nrfx_twis_t *twis_instance = &obj->slave_instance;

    /* Wait until the master is trying to read data */
    while (!nrfx_twis_is_waiting_tx_buff(twis_instance)) { }

    /* Master is attempting to read, now we prepare the tx buffer */
    nrfx_twis_tx_prepare(twis_instance, data, length);

    /* Wait until the transaction is over */
    while (nrfx_twis_is_pending_tx(twis_instance)) { }

    return nrfx_twis_tx_amount(twis_instance);
}

/** Configure I2C address.
 *  @param obj     The I2C object
 *  @param idx     Currently not used
 *  @param address The address to be set
 *  @param mask    Currently not used
 */
void i2c_slave_address(i2c_t *obj_, int idx, uint32_t address, uint32_t mask)
{
    struct i2c_s *obj = GET_I2C_S(obj_);

    /* Reconfigure this instance as an I2C slave with given address */
    obj->slave_addr = (uint8_t)address;

    i2c_configure_driver_instance(obj_);
}

#endif // DEVICE_I2CSLAVE

#endif // DEVICE_I2C
