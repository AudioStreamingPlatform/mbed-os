/*
 * Copyright (c) 2015-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hal/critical_section_api.h"

#include <cmsis.h>
#include <stdint.h>
#include <mbed_critical.h>

static uint8_t outside_critical_region = 1;
static uint8_t m_in_critical_region = 0;

uint32_t sd_nvic_critical_region_enter(uint8_t * p_is_nested_critical_region)
{
    __disable_irq();

    *p_is_nested_critical_region = (m_in_critical_region != 0);
    m_in_critical_region++;

    return 0;
}

uint32_t sd_nvic_critical_region_exit(uint8_t is_nested_critical_region)
{
    m_in_critical_region--;

    if (is_nested_critical_region == 0)
    {
        m_in_critical_region = 0;
        __enable_irq();
    }
    return 0;
}

void hal_critical_section_enter()
{
    /* expect 1 or N calls but only lock on the first one */
    if (outside_critical_region) {
        sd_nvic_critical_region_enter(&outside_critical_region);
    }
}

void hal_critical_section_exit()
{
    /* unlock on first call */
    outside_critical_region = 1;
    sd_nvic_critical_region_exit(0);
}

bool hal_in_critical_section(void)
{
    return !outside_critical_region;
}
