#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "lcd/lcd.h"
#include "rest_client/rest_client.h"

#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASS "Your_WiFi_Password"
#define UPDATE_INTERVAL_MS 10000

static const char *TAG = "HTTP_DISPLAY";
static char current_message[256] = "Connecting...";
static char last_message[256] = "";

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retrying WiFi connection...");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Got IP address");
    }
}

void wifi_init_sta()
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi...");

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to WiFi");
    }
}

void init_time()
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int max_retry = 10;

    while (timeinfo.tm_year < (2023 - 1900) && ++retry < max_retry)
    {
        ESP_LOGI(TAG, "Waiting for time sync (%d/%d)", retry, max_retry);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (retry == max_retry)
    {
        ESP_LOGW(TAG, "Failed to get time");
    }
    else
    {
        setenv("TZ", "GMT-2", 1);
        tzset();

        time(&now);
        localtime_r(&now, &timeinfo);

        char strftime_buf[64];
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "Local time: %s", strftime_buf);
    }
}

void app_main()
{
    lcd_init();
    lcd_clear_screen(0x0000);

    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set("esp_http_client", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

        lcd_draw_text_fast(current_message, 10, LCD_HEIGHT / 2, 0xFFFF, 0x0000);

    init_time();

    uint32_t last_update = 0;

    char old_message[256] = "";
    int x_pos = 10;
    int y_pos = LCD_HEIGHT / 2;

    while (1)
    {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        uint32_t now_ms = pdTICKS_TO_MS(xTaskGetTickCount());
        if ((now_ms - last_update) > UPDATE_INTERVAL_MS)
        {
            if (fetch_firebase_message(current_message, sizeof(current_message)))
            {
                ESP_LOGI(TAG, "Fetched message: %s", current_message);
            }
            else
            {
                snprintf(current_message, sizeof(current_message), "Fetch error");
                ESP_LOGE(TAG, "Failed to fetch message");
            }
            last_update = now_ms;
        }

        if (strcmp(current_message, old_message) != 0)
        {
            lcd_draw_text_fast(old_message, x_pos, y_pos, 0x0000, 0x0000);

            lcd_draw_text_fast(current_message, x_pos, y_pos, 0xFFFF, 0x0000);

            strcpy(old_message, current_message);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}