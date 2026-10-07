/*
 * Copyright (c) 2026 Eclipse Foundation
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/license/mit.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _CLOUD_CONFIG_H
#define _CLOUD_CONFIG_H

typedef enum
{
    None         = 0,
    WEP          = 1,
    WPA_PSK_TKIP = 2,
    WPA2_PSK_AES = 3
} WiFi_Mode;

#ifdef SOMEIP_LOCAL_CONFIG
#include "cloud_config.local.h"
#endif

#ifndef HOSTNAME
#define HOSTNAME "eclipse-threadx"
#endif
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif
#ifndef WIFI_MODE
#define WIFI_MODE WPA2_PSK_AES
#endif
#ifndef SOMEIP_BRIDGE_IP_ADDRESS
#define SOMEIP_BRIDGE_IP_ADDRESS IP_ADDRESS(192, 168, 88, 255)
#endif
#ifndef SOMEIP_BRIDGE_PORT
#define SOMEIP_BRIDGE_PORT 30511
#endif

#endif
