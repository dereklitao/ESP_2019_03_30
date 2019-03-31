#include "csro_common.h"
#include "ssl/ssl_crypto.h"
#include "cJSON.h"

#define MACSTR_FORMAT "%02x%02x%02x%02x%02x%02x"
#define PASSSTR_FORMAT "%02x%02x%02x%02x%02x%02x%02x%02x"

static void system_print_device_type(void)
{
#ifdef NLIGHT
    sprintf(sysinfo.dev_type, "nlight%d", NLIGHT);
#elif defined DLIGHT
    sprintf(sysinfo.dev_type, "dlight%d", DLIGHT);
#elif defined MOTOR
    sprintf(sysinfo.dev_type, "motor%d", MOTOR);
#elif defined AQI_MONITOR
    sprintf(sysinfo.dev_type, "air_monitor");
#elif defined AIR_SYSTEM
    sprintf(sysinfo.dev_type, "air_system");
#endif
}

void csro_system_get_info(void)
{
    nvs_handle handle;
    nvs_open("system", NVS_READWRITE, &handle);
    nvs_get_u8(handle, "sc_flag", &sysinfo.sc_flag);
    nvs_get_u32(handle, "power_count", &sysinfo.power_on_count);
    nvs_set_u32(handle, "power_count", (sysinfo.power_on_count + 1));
    nvs_get_u16(handle, "interval", &mqtt.interval);
    if (mqtt.interval < MIN_INTERVAL || mqtt.interval > MAX_INTERVAL)
    {
        mqtt.interval = 5;
        nvs_set_u16(handle, "interval", mqtt.interval);
    }
    nvs_commit(handle);
    nvs_close(handle);

    system_print_device_type();

    esp_wifi_get_mac(WIFI_MODE_STA, sysinfo.mac);
    sprintf(sysinfo.mac_str, MACSTR_FORMAT, sysinfo.mac[0], sysinfo.mac[1], sysinfo.mac[2], sysinfo.mac[3], sysinfo.mac[4], sysinfo.mac[5]);
    sprintf(sysinfo.host_name, "csro_%s", sysinfo.mac_str);
    sprintf(mqtt.id, "csro/%s", sysinfo.mac_str);
    sprintf(mqtt.name, "csro/%s/%s", sysinfo.mac_str, sysinfo.dev_type);

    uint8_t sha[20];
    SHA1_CTX *pass_ctx = (SHA1_CTX *)malloc(sizeof(SHA1_CTX));
    SHA1_Init(pass_ctx);
    SHA1_Update(pass_ctx, (const uint8_t *)sysinfo.mac_str, strlen(sysinfo.mac_str));
    SHA1_Final(sha, pass_ctx);
    free(pass_ctx);
    sprintf(mqtt.pass, PASSSTR_FORMAT, sha[0], sha[2], sha[4], sha[6], sha[8], sha[10], sha[12], sha[14]);
    debug("id = %s.\nname = %s.\npass = %s.\n", mqtt.id, mqtt.name, mqtt.pass);
}

void csro_system_set_status(system_status status)
{
    if (sysinfo.status == RESET_PENDING)
    {
        return;
    }
    if (sysinfo.status != status)
    {
        sysinfo.status = status;
    }
}

bool csro_system_get_wifi_info(void)
{
    bool result = false;
    uint8_t router_flag = 0;
    nvs_handle handle;

    nvs_open("system", NVS_READONLY, &handle);
    nvs_get_u8(handle, "router", &router_flag);
    if (router_flag == 1)
    {
        size_t ssid_len = 0;
        size_t pass_len = 0;
        nvs_get_str(handle, "ssid", NULL, &ssid_len);
        nvs_get_str(handle, "ssid", sysinfo.router_ssid, &ssid_len);
        nvs_get_str(handle, "pass", NULL, &pass_len);
        nvs_get_str(handle, "pass", sysinfo.router_pass, &pass_len);
        result = true;
    }
    nvs_close(handle);
    return result;
}

bool csro_system_get_wifi_status(void)
{
    static bool is_connect = false;
    tcpip_adapter_ip_info_t info;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &info);
    if (info.ip.addr != 0)
    {
        if (is_connect == false)
        {
            is_connect = true;
            csro_system_set_status(NORMAL_START_NOSERVER);
            sysinfo.wifi_conn_count++;
            sysinfo.ip[0] = info.ip.addr & 0xFF;
            sysinfo.ip[1] = (info.ip.addr & 0xFF00) >> 8;
            sysinfo.ip[2] = (info.ip.addr & 0xFF0000) >> 16;
            sysinfo.ip[3] = (info.ip.addr & 0xFF000000) >> 24;
            sprintf(sysinfo.ip_str, "%d.%d.%d.%d", sysinfo.ip[0], sysinfo.ip[1], sysinfo.ip[2], sysinfo.ip[3]);
        }
    }
    else
    {
        csro_system_set_status(NORMAL_START_NOWIFI);
        is_connect = false;
    }
    return is_connect;
}

void csro_system_set_interval(uint16_t interval)
{
    if (interval >= 2 && interval <= 120)
    {
        nvs_handle handle;
        nvs_open("system", NVS_READWRITE, &handle);
        nvs_set_u16(handle, "interval", interval);
        nvs_commit(handle);
        nvs_close(handle);
        mqtt.interval = interval;
        csro_mqtt_change_status_msg_timer();
    }
}

void csro_system_prepare_message(void)
{
    struct tm ontime;
    localtime_r(&datetime.time_on, &ontime);
    strftime(datetime.time_str, sizeof(datetime.time_str), "%Y/%m/%d %H:%M:%S", &ontime);

    cJSON *sys_json = cJSON_CreateObject();

    cJSON_AddNumberToObject(sys_json, "power", sysinfo.power_on_count);
    cJSON_AddNumberToObject(sys_json, "router", sysinfo.wifi_conn_count);
    cJSON_AddNumberToObject(sys_json, "broker", sysinfo.serv_conn_count);
    cJSON_AddNumberToObject(sys_json, "heap", esp_get_free_heap_size());

    cJSON_AddStringToObject(sys_json, "on", datetime.time_str);
    cJSON_AddStringToObject(sys_json, "ip", sysinfo.ip_str);
    cJSON_AddStringToObject(sys_json, "mac", sysinfo.mac_str);
    cJSON_AddStringToObject(sys_json, "name", sysinfo.host_name);
    cJSON_AddStringToObject(sys_json, "type", sysinfo.dev_type);
    cJSON_AddNumberToObject(sys_json, "interval", mqtt.interval);

    char *out = cJSON_PrintUnformatted(sys_json);
    strcpy(mqtt.content, out);
    free(out);
    cJSON_Delete(sys_json);
}
