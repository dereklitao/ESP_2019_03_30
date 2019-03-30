#include "csro_common.h"
#include "csro_device.h"

system_struct        sysinfo;
datetime_struct      datetime;
mqtt_struct          mqtt;

void csro_init(void)
{
    nvs_flash_init();
    csro_datetime_init();
    csro_system_get_info();
    csro_device_init();

    if (csro_system_get_wifi_info()) { xTaskCreate(csro_mqtt_task, "csro_mqtt_task", 4096, NULL, 10, NULL); }
    else                             { xTaskCreate(csro_sc_task, "csro_sc_task", 4096, NULL, 10, NULL); }
}