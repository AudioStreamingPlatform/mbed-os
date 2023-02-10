/* mbed Microcontroller Library
 * Copyright (c) 2006-2017 ARM Limited
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
#include "reset_reason_api.h"
#include "nrf.h"

#ifdef DEVICE_RESET_REASON

#include "device.h"

#define RETURN_IF_RESET_REASON(mask, result) \
    if (NRF_RESET_S->RESETREAS & (mask)) return result;

reset_reason_t hal_reset_reason_get(void)
{
    RETURN_IF_RESET_REASON(RESET_RESETREAS_DOG0_Msk, RESET_REASON_WATCHDOG)
    RETURN_IF_RESET_REASON(RESET_RESETREAS_DOG1_Msk, RESET_REASON_WATCHDOG)
    RETURN_IF_RESET_REASON(RESET_RESETREAS_SREQ_Msk, RESET_REASON_SOFTWARE)
    RETURN_IF_RESET_REASON(RESET_RESETREAS_LOCKUP_Msk, RESET_REASON_LOCKUP)
    RETURN_IF_RESET_REASON(RESET_RESETREAS_VBUS_Msk, RESET_REASON_POWER_ON)
    RETURN_IF_RESET_REASON(RESET_RESETREAS_RESETPIN_Msk, RESET_REASON_PIN_RESET)

    RETURN_IF_RESET_REASON(RESET_RESETREAS_DIF_Msk, RESET_REASON_PLATFORM)
    RETURN_IF_RESET_REASON(RESET_RESETREAS_CTRLAP_Msk, RESET_REASON_PLATFORM)
    RETURN_IF_RESET_REASON(RESET_RESETREAS_OFF_Msk, RESET_REASON_PLATFORM)
    RETURN_IF_RESET_REASON(RESET_RESETREAS_LPCOMP_Msk, RESET_REASON_PLATFORM)
    RETURN_IF_RESET_REASON(RESET_RESETREAS_NFC_Msk, RESET_REASON_PLATFORM)

    return RESET_REASON_UNKNOWN;
}

uint32_t hal_reset_reason_get_raw(void)
{
    return NRF_RESET_S->RESETREAS;
}

void hal_reset_reason_clear(void)
{
    NRF_RESET_S->RESETREAS = ~0;
}

#endif // DEVICE_RESET_REASON
