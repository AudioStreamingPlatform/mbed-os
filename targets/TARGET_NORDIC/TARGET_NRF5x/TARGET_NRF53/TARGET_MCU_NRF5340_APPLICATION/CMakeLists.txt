# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

if("NRF5340_DK" IN_LIST MBED_TARGET_LABELS)
    target_include_directories(mbed-core
        INTERFACE
            TARGET_NRF5340_DK
    )
endif()

if(${MBED_TOOLCHAIN} STREQUAL "GCC_ARM")
    set(LINKER_FILE device/TOOLCHAIN_GCC_ARM/nrf5340_application.ld)
    set(STARTUP_FILE device/TOOLCHAIN_GCC_ARM/startup_nrf5340_application.S)
endif()

set_property(GLOBAL PROPERTY MBED_TARGET_LINKER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_FILE})

target_include_directories(mbed-core
    INTERFACE
        .
        config
        device
)

target_sources(mbed-core
    INTERFACE
        PeripheralPins.c
        device/cmsis_nvic.c
        device/system_nrf5340_application.c
        ${STARTUP_FILE}
)
