# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0


target_include_directories(mbed-core
    INTERFACE
        drivers_nrf/nrf_soc_nosd
        libraries
        libraries/atomic
        libraries/atomic_fifo
        libraries/balloc
        libraries/delay
        libraries/experimental_log
        libraries/experimental_log/src
        libraries/experimental_memobj
        libraries/experimental_section_vars
        libraries/fds
        libraries/fstorage
        libraries/queue
        libraries/spi_mngr
        libraries/strerror
        libraries/twi_mngr
        libraries/util
)

target_sources(mbed-core
    INTERFACE
        drivers_nrf/nrf_soc_nosd/nrf_nvic.c
        drivers_nrf/nrf_soc_nosd/nrf_soc.c
        libraries/atomic/nrf_atomic.c
        libraries/atomic_fifo/nrf_atfifo.c
        libraries/balloc/nrf_balloc.c
        libraries/memobj/nrf_memobj.c
        libraries/experimental_section_vars/nrf_section_iter.c
        libraries/fds/fds.c
        libraries/fstorage/nrf_fstorage.c
        libraries/queue/nrf_queue.c
        libraries/spi_mngr/nrf_spi_mngr.c
        libraries/strerror/nrf_strerror.c
        libraries/twi_mngr/nrf_twi_mngr.c
        libraries/util/app_error.c
        libraries/util/app_error_handler_gcc.c
        libraries/util/app_error_handler_iar.c
        libraries/util/app_error_handler_keil.c
        libraries/util/app_error_weak.c
        libraries/util/app_util_platform.c
        libraries/util/nrf_assert.c
        libraries/util/sdk_mapped_flags.c
)
