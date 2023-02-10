# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

if("NRF52" IN_LIST MBED_TARGET_LABELS)
    add_subdirectory(TARGET_NRF52)
elseif("NRF53" IN_LIST MBED_TARGET_LABELS)
    add_subdirectory(TARGET_NRF53)
endif()

if("SDK_15_0" IN_LIST MBED_TARGET_LABELS)
    add_subdirectory(TARGET_SDK_15_0)
elseif("NRFX" IN_LIST MBED_TARGET_LABELS)
    target_include_directories(mbed-core
        INTERFACE
            nrfx
            nrfx/drivers
            nrfx/drivers/include
            nrfx/drivers/src
            nrfx/drivers/src/prs
            nrfx/hal
            nrfx/helpers
            nrfx/mdk
            nrfx/soc
    )

    target_sources(mbed-core
        INTERFACE
            nrfx/drivers/src/nrfx_adc.c
            nrfx/drivers/src/nrfx_clock.c
            nrfx/drivers/src/nrfx_comp.c
            nrfx/drivers/src/nrfx_dppi.c
            nrfx/drivers/src/nrfx_gpiote.c
            nrfx/drivers/src/nrfx_i2s.c
            nrfx/drivers/src/nrfx_lpcomp.c
            nrfx/drivers/src/nrfx_nfct.c
            nrfx/drivers/src/nrfx_nvmc.c
            nrfx/drivers/src/nrfx_pdm.c
            nrfx/drivers/src/nrfx_power.c
            nrfx/drivers/src/nrfx_ppi.c
            nrfx/drivers/src/nrfx_pwm.c
            nrfx/drivers/src/nrfx_qdec.c
            nrfx/drivers/src/nrfx_qspi.c
            nrfx/drivers/src/nrfx_rng.c
            nrfx/drivers/src/nrfx_rtc.c
            nrfx/drivers/src/nrfx_saadc.c
            nrfx/drivers/src/nrfx_spi.c
            nrfx/drivers/src/nrfx_spim.c
            nrfx/drivers/src/nrfx_spis.c
            nrfx/drivers/src/nrfx_systick.c
            nrfx/drivers/src/nrfx_temp.c
            nrfx/drivers/src/nrfx_timer.c
            nrfx/drivers/src/nrfx_twi_twim.c
            nrfx/drivers/src/nrfx_twi.c
            nrfx/drivers/src/nrfx_twim.c
            nrfx/drivers/src/nrfx_twis.c
            nrfx/drivers/src/nrfx_uart.c
            nrfx/drivers/src/nrfx_uarte.c
            nrfx/drivers/src/nrfx_usbd.c
            nrfx/drivers/src/nrfx_wdt.c
            nrfx/drivers/src/prs/nrfx_prs.c
            nrfx/helpers/nrfx_flag32_allocator.c
            nrfx/soc/nrfx_atomic.c
    )
endif()

target_include_directories(mbed-core
    INTERFACE
        .
)

target_sources(mbed-core
    INTERFACE
        lp_ticker.c
        pinmap.c
        port_api.c
        qspi_api.c
        reset_reason.c
        rtc_api.c
)
