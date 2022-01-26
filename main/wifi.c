//Based on ESP-IDF example for wifi provisioning

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "qrcode.h"

static const char *TAG_WIFI = "wifi-mgr";

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

#define PROV_QR_VERSION "v1"
#define PROV_TRANSPORT_BLE "ble"
#define QRCODE_BASE_URL "https://espressif.github.io/esp-jumpstart/qrcode.html"

/* Event handler for catching system events */
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static int retries;

    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG_WIFI, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG_WIFI, "Received Wi-Fi credentials for network %s", (const char *)wifi_sta_cfg->ssid);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG_WIFI, "Provisioning failed!\n\tReason : %s"
                          "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            retries++;
            if (retries >= CONFIG_EXAMPLE_PROV_MGR_MAX_RETRY_CNT) {
                ESP_LOGI(TAG_WIFI, "Failed to connect with provisioned AP, reseting provisioned credentials");
                wifi_prov_mgr_reset_sm_state_on_failure();
                retries = 0;
            }
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG_WIFI, "Provisioning successful");
            retries = 0;
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG_WIFI, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
    }
}

static void wifi_init_sta(void) {
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void get_device_service_name(char *service_name, size_t max) {
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

static void wifi_prov_print_qr(const char *name, const char *pop, const char *transport) {
    if (!name || !transport) {
        ESP_LOGW(TAG_WIFI, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150] = {0};
    if (pop) {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\""",\"pop\":\"%s\",\"transport\":\"%s\"}", PROV_QR_VERSION, name, pop, transport);
    } else {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\""",\"transport\":\"%s\"}", PROV_QR_VERSION, name, transport);
    }
#ifdef CONFIG_EXAMPLE_PROV_SHOW_QR
    ESP_LOGI(TAG_WIFI, "Scan this QR code from the provisioning application for Provisioning.");
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&cfg, payload);
#endif /* CONFIG_APP_WIFI_PROV_SHOW_QR */
    ESP_LOGI(TAG_WIFI, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}

static void do_provision() {
    ESP_LOGI(TAG_WIFI, "Starting provisioning");

    char service_name[12];
    get_device_service_name(service_name, sizeof(service_name));

    /* What is the security level that we want (0 or 1):
        *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
        *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
        *          using X25519 key exchange and proof of possession (pop) and AES-CTR
        *          for encryption/decryption of messages.
        */
    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

    /* Do we want a proof-of-possession (ignored if Security 0 is selected):
        *      - this should be a string with length > 0
        *      - NULL if not used
        */
    const char *pop = "GnwcnUt5qJGKAN";

    /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
        * set a custom 128 bit UUID which will be included in the BLE advertisement
        * and will correspond to the primary GATT service that provides provisioning
        * endpoints as GATT characteristics. Each GATT characteristic will be
        * formed using the primary service UUID as base, with different auto assigned
        * 12th and 13th bytes (assume counting starts from 0th byte). The client side
        * applications must identify the endpoints by reading the User Characteristic
        * Description descriptor (0x2901) for each characteristic, which contains the
        * endpoint name of the characteristic */
    uint8_t custom_service_uuid[] = {
        /* LSB <---------------------------------------
            * ---------------------------------------> MSB */
        0xb4,
        0xdf,
        0x5a,
        0x1c,
        0x3f,
        0x6b,
        0xf4,
        0xbf,
        0xea,
        0x4a,
        0x82,
        0x03,
        0x04,
        0x90,
        0x1a,
        0x02,
    };
    wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

    /* Start provisioning service */
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, NULL));

    /* Print QR code for provisioning */
    wifi_prov_print_qr(service_name, pop, PROV_TRANSPORT_BLE);
}