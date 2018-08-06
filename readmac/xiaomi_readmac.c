/*
 * Copyright (C) 2016 The CyanogenMod Project
 *               2017-2018 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "xiaomi_readmac"
#define LOG_NDEBUG 0

#include <log/log.h>

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAC_SIZE 6
#define WLAN_MAC_FILE "/persist/wlan_mac.bin"

#define LIB_QMINVAPI "libqminvapi.so"

static const char xiaomi_mac_prefix[] = { 0x34, 0x80, 0xb3 };

typedef int (*qmi_nv_read_mac_t)(char** mac);

static void fill_random_mac(unsigned char mac[]) {
    int i;
    memcpy(mac, xiaomi_mac_prefix, sizeof(xiaomi_mac_prefix));

    /*
     * We don't need strong randomness, and if the NV is corrupted
     * any hardware values are suspect, so just seed it with the
     * current time.
     */
    srand(time(NULL));

    for (i = sizeof(xiaomi_mac_prefix) / sizeof(xiaomi_mac_prefix[0]);
            i < MAC_SIZE; i++) {
        mac[i] = rand() % 255;
    }
}

static void load_mac_with(unsigned char mac[],
        qmi_nv_read_mac_t qmi_nv_read_mac) {
    char *nv_mac = NULL;
    int i;

    if (!qmi_nv_read_mac) {
        ALOGE("failed to find qmi_nv_read_mac, error: %s", dlerror());
        goto fill_random;
    }

    qmi_nv_read_mac(&nv_mac);
    if (!nv_mac) {
        ALOGE("failed to read mac from nv");
        goto fill_random;
    }

    for (i = 0; i < MAC_SIZE; i++) {
        mac[i] = nv_mac[i];
    }

    return;

fill_random:
    fill_random_mac(mac);
}

static int is_valid_wlan_mac_file() {
    char content[1024];
    FILE *fp;

    fp = fopen(WLAN_MAC_FILE, "r");
    if (fp == NULL)
        return 0;

    memset(content, 0, sizeof(content));
    fread(content, 1, sizeof(content) - 1, fp);
    fclose(fp);

    if (strstr(content, "Intf0MacAddress") == NULL) {
        ALOGV("%s is missing Intf0MacAddress entry value", WLAN_MAC_FILE);
        return 0;
    }

    if (strstr(content, "Intf1MacAddress") == NULL) {
        ALOGV("%s is missing Intf1MacAddress entry value", WLAN_MAC_FILE);
        return 0;
    }

    return 1;
}

static int write_wlan_mac_file(unsigned char wlan_mac[]) {
    FILE *fp;

    fp = fopen(WLAN_MAC_FILE, "w");
    if (fp == NULL)
        return -1;

    fprintf(fp, "Intf0MacAddress=%02X%02X%02X%02X%02X%02X\n", wlan_mac[0],
            wlan_mac[1], wlan_mac[2], wlan_mac[3], wlan_mac[4], wlan_mac[5]);
    fprintf(fp, "Intf1MacAddress=%02X%02X%02X%02X%02X%02X\n", wlan_mac[0],
            wlan_mac[1], wlan_mac[2], wlan_mac[3], wlan_mac[4],
            (unsigned char)(wlan_mac[5] + 1));
    fprintf(fp, "END\n");
    fclose(fp);

    return 0;
}

static void set_wlan_mac_with(qmi_nv_read_mac_t qmi_nv_read_wlan_mac) {
    unsigned char wlan_mac[MAC_SIZE];
    int rc;

    if (is_valid_wlan_mac_file()) {
        ALOGV("%s is valid", WLAN_MAC_FILE);
        return;
    }

    load_mac_with(wlan_mac, qmi_nv_read_wlan_mac);

    rc = write_wlan_mac_file(wlan_mac);
    if (rc) {
        ALOGE("failed to write %s", WLAN_MAC_FILE);
    }
}

int main() {
    qmi_nv_read_mac_t qmi_nv_read_wlan_mac = NULL;
    void *handle;

    handle = dlopen(LIB_QMINVAPI, RTLD_NOW);
    if (!handle) {
        ALOGE("%s", dlerror());
        goto set_macs;
    }

    qmi_nv_read_wlan_mac =
        (qmi_nv_read_mac_t)dlsym(handle, "qmi_nv_read_wlan_mac");

set_macs:
    set_wlan_mac_with(qmi_nv_read_wlan_mac);

    if (handle)
        dlclose(handle);

    return 0;
}
