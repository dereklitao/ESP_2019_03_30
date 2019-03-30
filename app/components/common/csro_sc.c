#include "csro_common.h"
#include "esp_smartconfig.h"

static EventGroupHandle_t   wifi_event_group;
static const EventBits_t    CONNECTED_BIT       = BIT0;
static const EventBits_t    ESPTOUCH_DONE_BIT   = BIT1;


static void smartconfig_callback(smartconfig_status_t status, void *pdata)
{
    if (status == SC_STATUS_LINK) 
    {            
        wifi_config_t *wifi_config = pdata;
        strcpy(sysinfo.router_ssid, "Jupiter");
        strcpy(sysinfo.router_pass, "150933205");
        esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config);
        esp_wifi_connect();
    }
    else if (status == SC_STATUS_LINK_OVER) 
    {
        xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}


static void smartconfig_task(void * pvParameters)
{
    EventBits_t uxBits;
    esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS);
    esp_smartconfig_start(smartconfig_callback);
    while (true) 
    {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if(uxBits & ESPTOUCH_DONE_BIT) 
        {
            esp_smartconfig_stop();
            nvs_handle handle;
            nvs_open("system", NVS_READWRITE, &handle);
            nvs_set_str(handle, "ssid", sysinfo.router_ssid);
            nvs_set_str(handle, "pass", sysinfo.router_pass);
            nvs_set_u8(handle, "router", 1);
            nvs_set_u8(handle, "sc_flag", 1);
            nvs_commit(handle);
            nvs_close(handle);
            esp_restart();
        }
    }
    vTaskDelete(NULL);
}


static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    if (event->event_id == SYSTEM_EVENT_STA_START) {
        xTaskCreate(smartconfig_task, "smartconfig_task", 2048, NULL, 5, NULL);
    }
    else if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
    else if (event->event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    }
    return ESP_OK;
}


void csro_sc_task(void *pvParameters)
{
    csro_system_set_status(SMARTCONFIG);
    wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    esp_event_loop_init(wifi_event_handler, NULL);
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    while(true) 
    { 
        vTaskDelay(1000 / portTICK_RATE_MS); 
        debug("SC TASK!. FREE HEAP = %d.\n", esp_get_free_heap_size());  
    }
    vTaskDelete(NULL);
}