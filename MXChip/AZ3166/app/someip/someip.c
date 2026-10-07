/*
 * Copyright (c) 2026 Eclipse Foundation
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/license/mit.
 *
 * SPDX-License-Identifier: MIT
 */

#include "someip.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cloud_config.h"
#include "nx_api.h"
#include "sensor.h"
#include "sntp_client.h"
#include "wwd_networking.h"

#define SOMEIP_PUBLISH_INTERVAL_SECONDS 5
#define SOMEIP_PACKET_SIZE              28

typedef char someip_requires_binary32_float[
    (sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128) ? 1 : -1];

static void write_u16_be(UCHAR* output, uint16_t value)
{
    output[0] = (UCHAR)(value >> 8);
    output[1] = (UCHAR)value;
}

static void write_u32_be(UCHAR* output, uint32_t value)
{
    output[0] = (UCHAR)(value >> 24);
    output[1] = (UCHAR)(value >> 16);
    output[2] = (UCHAR)(value >> 8);
    output[3] = (UCHAR)value;
}

static void write_u64_be(UCHAR* output, uint64_t value)
{
    write_u32_be(output, (uint32_t)(value >> 32));
    write_u32_be(output + 4, (uint32_t)value);
}

static void build_notification(UCHAR packet[SOMEIP_PACKET_SIZE],
    uint16_t session_id,
    float temperature_degC,
    uint64_t timestamp_ms)
{
    uint32_t temperature_bits;

    write_u16_be(packet, 0x1234);
    write_u16_be(packet + 2, 0x8001);
    write_u32_be(packet + 4, 0x00000014);
    write_u16_be(packet + 8, 0x0001);
    write_u16_be(packet + 10, session_id);
    packet[12] = 0x01;
    packet[13] = 0x01;
    packet[14] = 0x02;
    packet[15] = 0x00;

    memcpy(&temperature_bits, &temperature_degC, sizeof(temperature_bits));
    write_u32_be(packet + 16, temperature_bits);
    write_u64_be(packet + 20, timestamp_ms);
}

void someip_thread_entry(ULONG parameter)
{
    UINT status;
    NX_UDP_SOCKET socket;
    uint16_t session_id = 0;

    (void)parameter;

    printf("Starting SOME/IP temperature publisher\r\n");

    status = wwd_network_init(WIFI_SSID, WIFI_PASSWORD, WIFI_MODE);
    if (status != NX_SUCCESS)
    {
        printf("ERROR: Failed to initialize the network (0x%08x)\r\n", status);
        return;
    }

    status = wwd_network_connect();
    if (status != NX_SUCCESS)
    {
        printf("ERROR: Failed to connect to the network (0x%08x)\r\n", status);
        return;
    }

    status = nx_udp_socket_create(&nx_ip,
        &socket,
        "SOME/IP UDP socket",
        NX_IP_NORMAL,
        NX_FRAGMENT_OKAY,
        64,
        0);
    if (status != NX_SUCCESS)
    {
        printf("ERROR: Failed to create the SOME/IP UDP socket (0x%08x)\r\n", status);
        return;
    }

    status = nx_udp_socket_bind(&socket, NX_ANY_PORT, NX_NO_WAIT);
    if (status != NX_SUCCESS)
    {
        printf("ERROR: Failed to bind the SOME/IP UDP socket (0x%08x)\r\n", status);
        nx_udp_socket_delete(&socket);
        return;
    }

    printf("Publishing SOME/IP temperature to %u.%u.%u.%u:%u\r\n",
        (unsigned int)((SOMEIP_BRIDGE_IP_ADDRESS >> 24) & 0xff),
        (unsigned int)((SOMEIP_BRIDGE_IP_ADDRESS >> 16) & 0xff),
        (unsigned int)((SOMEIP_BRIDGE_IP_ADDRESS >> 8) & 0xff),
        (unsigned int)(SOMEIP_BRIDGE_IP_ADDRESS & 0xff),
        (unsigned int)SOMEIP_BRIDGE_PORT);

    while (1)
    {
        hts221_data_t reading = hts221_data_read();

        if (!isfinite(reading.temperature_degC))
        {
            printf("ERROR: HTS221 returned an invalid temperature\r\n");
        }
        else
        {
            NX_PACKET* netx_packet = NX_NULL;
            UCHAR packet[SOMEIP_PACKET_SIZE];

            build_notification(packet, (uint16_t)(session_id + 1),
                reading.temperature_degC, sntp_time_ms_get());

            status = nx_packet_allocate(&nx_pool[0], &netx_packet, NX_UDP_PACKET, NX_NO_WAIT);
            if (status != NX_SUCCESS)
            {
                printf("ERROR: Failed to allocate SOME/IP packet (0x%08x)\r\n", status);
            }
            else if ((status = nx_packet_data_append(
                          netx_packet, packet, SOMEIP_PACKET_SIZE, &nx_pool[0], NX_NO_WAIT)) != NX_SUCCESS)
            {
                printf("ERROR: Failed to build SOME/IP packet (0x%08x)\r\n", status);
                nx_packet_release(netx_packet);
            }
            else if ((status = nx_udp_socket_send(
                          &socket, netx_packet, SOMEIP_BRIDGE_IP_ADDRESS, SOMEIP_BRIDGE_PORT)) != NX_SUCCESS)
            {
                printf("ERROR: Failed to send SOME/IP notification (0x%08x)\r\n", status);
                nx_packet_release(netx_packet);
            }
            else
            {
                ++session_id;
                printf("Published temperature %.2f C\r\n", (double)reading.temperature_degC);
            }
        }

        tx_thread_sleep(SOMEIP_PUBLISH_INTERVAL_SECONDS * TX_TIMER_TICKS_PER_SECOND);
    }
}
