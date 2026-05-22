#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

#include "esp_wifi.h"
#include "esp_http_server.h"


#include <stdio.h>

#include "flash.h"
#include "stm_pro_mode.h"

static const char *WS_TAG = "ws_server";
static const char *WIFI_TAG = "wifi_server";
static const char *SERVER_TAG = "http_server";


static FILE *file_handle = NULL;            // file handle for where we want to save the binary in SPIFFS
static bool receiving_binary = false;           // indicates that we are streaming binary chunks over the websocket 

// initialise the ESP as a soft access point (basically emit wifi)
static void init_wifi_softap(void)
{
    ESP_LOGI(WIFI_TAG, "starting wifi access point");
	esp_netif_create_default_wifi_ap(); // create the network interface object (assigns ip)

	// configure the wifi driver internally to default and initialise it
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

	// configure how we want the wifi to broadcast
	wifi_config_t wifi_ap_config = {
		.ap = {
			.ssid = "car_go_zoom",
			.ssid_len = 0, // TODO - think this is fine but check?
			.channel = 1, 									// a post-brexit european channel
			.password = "freddie_is_a_good_boy",	// TODO - put this in a config file...
			.max_connection = 4, // number of client connections we allow
			.authmode = WIFI_AUTH_WPA2_PSK,
			.pmf_cfg = {
				.required = false,
			},
        },
    };

	esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config);
    esp_wifi_start();
    ESP_LOGI(WIFI_TAG, "wifi access point initialised");
}

// define the handler to deal with data that gets sent to the ESP over the webnsocket
static esp_err_t ws_handler(httpd_req_t *req){
    ESP_LOGI(WS_TAG, "handler called");
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt));

    // get frame length
    ESP_ERROR_CHECK(httpd_ws_recv_frame(req, &ws_pkt, 0));

    if (ws_pkt.len == 0) { // nothing to do
        return ESP_OK;
    }

    // allocate space for the buffrew 
    uint8_t *buf = calloc(1, ws_pkt.len + 1);
    if (!buf) return ESP_ERR_NO_MEM;

    ws_pkt.payload = buf;

    // Now read payload into buf
    ESP_ERROR_CHECK(httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len));

    // check if this is the start of a transfer
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && ws_pkt.len == 5 &&  memcmp(buf, "START", 5) == 0){
        ESP_LOGI(WS_TAG, "Binary Transfer Initiated");

        if (file_handle != NULL){
            ESP_LOGW(WS_TAG, "Closing file handle that was left open");
            fclose(file_handle);
        }

        clear_binary();
        file_handle = fopen(FILE_LOCATION, "wb");

        if (!file_handle){
            ESP_LOGE(WS_TAG, "Failed to open file for writing");
            free(buf);
            return ESP_FAIL;
        }

        receiving_binary = true;
        free(buf);
        return ESP_OK;
    }

    // check if a transfer has ended
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && ws_pkt.len == 3 &&  memcmp(buf, "END", 3) == 0){
        ESP_LOGI(WS_TAG, "Binary Transfer Fimished");

        if (file_handle) {
            fclose(file_handle);
            file_handle = NULL;
        }

        receiving_binary = false;
        free(buf);

        // TODO - put the logic here to initiate the STM flashing - uncomment when ready
        // flash_stm();

        return ESP_OK;
    }

    // received a binary chunk to save
    if (ws_pkt.type == HTTPD_WS_TYPE_BINARY && receiving_binary){
        ESP_LOGI(WS_TAG, "Binary Chunk Received");

        if (save_binary_chunk(file_handle, buf, ws_pkt.len) == 0){
            ESP_LOGI(WS_TAG, "Chunk succesfully saved");
        } 
        else{
            ESP_LOGE(WS_TAG, "Failed to save a chunk");
        }
    }

    // httpd_ws_frame_t resp;
    // memset(&resp, 0, sizeof(resp));

    // resp.payload = buf;
    // resp.len = ws_pkt.len;
    // resp.type = HTTPD_WS_TYPE_TEXT;
    // resp.final = true;

    // httpd_ws_send_frame(req, &resp);

    free(buf);
    return ESP_OK;
}

// start the HTTP server
static httpd_handle_t start_server(void)
{
    ESP_LOGI(SERVER_TAG, "starting server");
	// reserve space for server and use default configuration - NOTE - we run on port
	httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(SERVER_TAG, "Starting server on port: '%d'\n", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        printf("server handle = %p\n", server);

        // define the websocket
		static const httpd_uri_t ws = {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = ws_handler, // function to handle incoming
			.user_ctx   = NULL,
			.is_websocket = true,
        #ifdef CONFIG_EXAMPLE_ENABLE_WS_PRE_HANDSHAKE_CB
                .ws_pre_handshake_cb = ws_pre_handshake_cb,
        #endif /* CONFIG_EXAMPLE_ENABLE_WS_PRE_HANDSHAKE_CB */
        #ifdef CONFIG_EXAMPLE_ENABLE_WS_POST_HANDSHAKE_CB
                .ws_post_handshake_cb = ws_post_handshake_cb,
        #endif /* CONFIG_EXAMPLE_ENABLE_WS_POST_HANDSHAKE_CB */
        };        

        // register the ws handler
        ESP_LOGI(SERVER_TAG, "attempting to register handler");
        httpd_register_uri_handler(server, &ws);
        ESP_LOGI(SERVER_TAG, "handler has been registered");
        ESP_LOGI(SERVER_TAG,"server has been started\n");
        return server;
    }
    ESP_LOGE(SERVER_TAG,"Error starting server!");
    return NULL;
}

void app_main(void)
{
	nvs_flash_init(); // init non-volatile storage in memory so wifi initialisation persists
	esp_netif_init(); // init the network interface abstraciton layer
	esp_event_loop_create_default(); // create the global event loop
	init_wifi_softap(); // init the access point
    initSPIFFS(); // mount the file system so that we can write to it
	start_server();
}
