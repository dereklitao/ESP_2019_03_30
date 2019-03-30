#include "csro_device.h"
#include "modbus_master.h"

#ifdef AIR_SYSTEM

struct air_sys_register
{
    uint8_t     coil[100];
    bool        coil_flag[100];
    uint16_t    holding[100];
    bool        holding_flag[100];
} airsys_reg;

static SemaphoreHandle_t    master_mutex;
static SemaphoreHandle_t    write_semaphore;


static void modify_coil_reg(uint8_t index, uint8_t value)
{
    if(value == 1 || value == 0) {
        airsys_reg.coil[index] = value;
        airsys_reg.coil_flag[index] = true;
        xSemaphoreGive(write_semaphore);
    }
}

static void modify_holding_reg(uint8_t index, uint8_t value)
{
    airsys_reg.holding[index] = value;
    airsys_reg.holding_flag[index] = true;
    xSemaphoreGive(write_semaphore);
}



void csro_air_system_prepare_status_message(void)
{

}

void csro_air_system_handle_self_message(MessageData* data)
{
    char sub_topic[50];
    char payload[50];
    strncpy(payload, data->message->payload, data->message->payloadlen);
    csro_systen_get_self_message_sub_topic(data, sub_topic);
    if (strcmp(sub_topic, "mode") == 0) 
    {
         if (strcmp(payload, "制热") == 0) { debug("hot\n"); }
         else if (strcmp(payload, "制冷") == 0) { debug("cold\n"); }
         else if (strcmp(payload, "除湿") == 0) { debug("humi\n"); }
         else if (strcmp(payload, "仅通风") == 0) { debug("fan\n"); }
         else if (strcmp(payload, "关机") == 0) { debug("off\n"); }
    }
}

void csro_air_system_handle_hass_message(MessageData* data)
{

}

void csro_air_system_alarm_action(uint16_t action)
{

}

static void write_coil_with_flag_and_range_check(uint8_t index)
{
    if (airsys_reg.coil_flag[index] == false) { return; }
    if (airsys_reg.coil[index] == 1 || airsys_reg.coil[index] == 0) { modbus_master_write_single_coil(&Master, 1, index, airsys_reg.coil[index]); }
    airsys_reg.coil_flag[index] = false;
}


static void write_holding_with_flag_and_range_check(uint8_t index, uint16_t min, uint16_t max)
{    
    if (airsys_reg.holding_flag[index] == false) { return; }
    if (airsys_reg.holding[index] >= min && airsys_reg.holding[index] <= max) { modbus_master_Write_single_holding_reg(&Master, 1, index, airsys_reg.holding[index]); }
    airsys_reg.holding_flag[index] = false;
}


static void modbus_master_write_task(void *pvParameters)
{
    while(true)
    {
        if (xSemaphoreTake(write_semaphore, portMAX_DELAY) == pdTRUE ) 
        {
            if (xSemaphoreTake(master_mutex, portMAX_DELAY) == pdTRUE) 
            {
                write_coil_with_flag_and_range_check(40);
                write_coil_with_flag_and_range_check(41);
                write_holding_with_flag_and_range_check(21, 1, 2);
                xSemaphoreGive(master_mutex);
                csro_mqtt_msg_trigger_state(NULL);
            }
        }
    }
    vTaskDelete(NULL);
}

static void modbus_master_read_task(void *pvParameters)
{
    while(true)
	{
        if ( xSemaphoreTake(master_mutex, portMAX_DELAY) == pdTRUE ) 
        {
            uint8_t temp_coil[100];
            modbus_master_read_coils(&Master, 1, 1, 79, &temp_coil[1]);
            for(size_t i = 1; i < 80; i++) { if (airsys_reg.coil_flag[i] == false) { airsys_reg.coil[i] = temp_coil[i]; } }
            xSemaphoreGive(master_mutex);
        }

        if ( xSemaphoreTake(master_mutex, portMAX_DELAY) == pdTRUE ) {
            uint16_t temp_holding[100];
            modbus_master_read_holding_regs(&Master, 1, 1, 42, &temp_holding[1]);
            for(size_t i = 1; i < 43; i++) { if (airsys_reg.holding_flag[i] == false) { airsys_reg.holding[i] = temp_holding[i]; } }
            xSemaphoreGive(master_mutex);
        }       
        vTaskDelay(200 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}


void csro_air_system_init(void)
{
    master_mutex = xSemaphoreCreateMutex();
    write_semaphore = xSemaphoreCreateBinary();
    modbus_master_init();
    xTaskCreate(modbus_master_read_task, "modbus_master_read_task", 1024, NULL, 5, NULL);
    xTaskCreate(modbus_master_write_task, "modbus_master_write_task", 1024, NULL, 8, NULL);
}



#endif