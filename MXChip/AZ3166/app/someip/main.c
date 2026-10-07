/*
 * Copyright (c) 2026 Eclipse Foundation
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/license/mit.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "board_init.h"
#include "cmsis_utils.h"
#include "someip.h"
#include "tx_api.h"

#define SOMEIP_THREAD_STACK_SIZE 4096
#define SOMEIP_THREAD_PRIORITY   4

static TX_THREAD someip_thread;
static ULONG someip_thread_stack[SOMEIP_THREAD_STACK_SIZE / sizeof(ULONG)];

void tx_application_define(void* first_unused_memory)
{
    systick_interval_set(TX_TIMER_TICKS_PER_SECOND);

    UINT status = tx_thread_create(&someip_thread,
        "SOME/IP temperature publisher",
        someip_thread_entry,
        0,
        someip_thread_stack,
        SOMEIP_THREAD_STACK_SIZE,
        SOMEIP_THREAD_PRIORITY,
        SOMEIP_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("ERROR: SOME/IP publisher thread creation failed (0x%08x)\r\n", status);
    }
}

int main(void)
{
    board_init();
    tx_kernel_enter();
    return 0;
}
