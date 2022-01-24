#include <stdio.h>
#include <string.h>
#include "esp_console.h"
#include "esp_wifi.h"

#define DEFAULT_SCAN_LIST_SIZE 20

bool wifi_initialized = false;

static int wifiinit(int argc, char** argv) {
    if (wifi_initialized) {
        printf("Wifi already initialized\n");
        return 1;
    }
    
    printf("Initializing wifi...\n");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_initialized = true;
    return 0;
}

char* getAuthMode(int authmode) {
    switch (authmode) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2 PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/WPA2 PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2 Enterprise";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3 PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WPA2/WPA3 PSK";
        default:
            return "Unknown";
    }
}

static int wifisearch(int argc, char** argv) {
    if (!wifi_initialized) {
        printf("Wifi not initialized\n");
        return 1;
    }

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    printf("Total APs scanned = %u\n", ap_count);
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        printf("%d %s Auth: %s\n", ap_info[i].rssi, ap_info[i].ssid, getAuthMode(ap_info[i].authmode));
    }

    return 0;
}

void register_wifisearch(void) {
    esp_console_cmd_t wifisearch_cmd = {
        .command = "wifisearch",
        .help = "Search for wifi networks",
        .hint = NULL,
        .func = &wifisearch
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifisearch_cmd));

    esp_console_cmd_t wifiinit_cmd = {
        .command = "wifiinit",
        .help = "Initialize wifi",
        .hint = NULL,
        .func = &wifiinit
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifiinit_cmd));
}