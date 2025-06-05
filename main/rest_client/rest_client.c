#include <string.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"

#define REST_TAG "REST_CLIENT"
#define FIREBASE_URL "Your_Firebase_URL_Here"
#define MAX_RESPONSE_SIZE 512

typedef struct
{
    char *buffer;
    size_t buffer_size;
    size_t offset;
} http_response_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->user_data)
    {
        http_response_t *ctx = (http_response_t *)evt->user_data;
        size_t to_copy = evt->data_len;

        if (ctx->offset + to_copy >= ctx->buffer_size)
        {
            to_copy = ctx->buffer_size - ctx->offset - 1;
            ESP_LOGW(REST_TAG, "Response truncated");
        }

        if (to_copy > 0)
        {
            memcpy(ctx->buffer + ctx->offset, evt->data, to_copy);
            ctx->offset += to_copy;
            ctx->buffer[ctx->offset] = '\0';
        }
    }
    return ESP_OK;
}

bool fetch_firebase_message(char *buffer, size_t buffer_size)
{
    char raw_response[MAX_RESPONSE_SIZE] = {0};
    http_response_t response_ctx = {
        .buffer = raw_response,
        .buffer_size = sizeof(raw_response),
        .offset = 0};

    esp_http_client_config_t config = {
        .url = FIREBASE_URL,
        .event_handler = http_event_handler,
        .user_data = &response_ctx,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = false,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client)
    {
        ESP_LOGE(REST_TAG, "HTTP client init failed");
        return false;
    }

    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "Cache-Control", "no-cache");
    esp_http_client_set_header(client, "Connection", "close");

    ESP_LOGI(REST_TAG, "Starting HTTP request to Firebase");
    esp_err_t err = esp_http_client_perform(client);

    bool success = false;
    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(REST_TAG, "HTTP Status: %d", status);

        if (status == 200 && response_ctx.offset > 0)
        {
            ESP_LOGI(REST_TAG, "Received %d bytes: %.*s",
                     response_ctx.offset, response_ctx.offset, raw_response);

            size_t len = response_ctx.offset;
            if (len >= 2 && raw_response[0] == '"' && raw_response[len - 1] == '"')
            {
                len -= 2;
                memmove(raw_response, raw_response + 1, len);
            }
            raw_response[len] = '\0';

            strncpy(buffer, raw_response, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            success = true;
        }
        else
        {
            ESP_LOGE(REST_TAG, "No valid data received");
        }
    }
    else
    {
        ESP_LOGE(REST_TAG, "Request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return success;
}