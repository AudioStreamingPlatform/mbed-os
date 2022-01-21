# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

target_include_directories(mbed-core
    INTERFACE
        .
        #nrfx/legacy
)

target_sources(mbed-core
    INTERFACE
        #nrfx/legacy/nrf_drv_clock.c
        #nrfx/legacy/nrf_drv_power.c
        #nrfx/legacy/nrf_drv_ppi.c
        #nrfx/legacy/nrf_drv_rng.c
        #nrfx/legacy/nrf_drv_spi.c
        #nrfx/legacy/nrf_drv_spis.c
        #nrfx/legacy/nrf_drv_swi.c
        #nrfx/legacy/nrf_drv_twi.c
        #nrfx/legacy/nrf_drv_uart.c
)
