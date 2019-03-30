#include "csro_device.h"
#include "cJSON.h"


void csro_device_prepare_status_message(void)
{
    #ifdef NLIGHT
        csro_nlight_prepare_status_message();
    #elif defined DLIGHT
        csro_dlight_prepare_status_message();
    #elif defined MOTOR
        csro_motor_prepare_status_message();
    #elif defined AQI_MONITOR
        csro_air_monitor_prepare_status_message();
    #elif defined AIR_SYSTEM
        csro_air_system_prepare_status_message();
    #endif
}

void csro_device_prepare_timer_message(void)
{
    uint8_t timer_count = 0;
    char    key[20];
    cJSON   *alarm_json=cJSON_CreateObject();
    cJSON   *single_alalm[ALARM_NUMBER];

    for(size_t i = 0; i < ALARM_NUMBER; i++)
    {
        if (alarms[i].valid == true) 
        { 
            timer_count++; 
            sprintf(key, "alarm%d", timer_count);
            cJSON_AddItemToObject(alarm_json, key, single_alalm[i] = cJSON_CreateObject());
            cJSON_AddNumberToObject(single_alalm[i], "weekday", alarms[i].weekday);
            cJSON_AddNumberToObject(single_alalm[i], "minutes", alarms[i].minutes);
            cJSON_AddNumberToObject(single_alalm[i], "action",  alarms[i].action);
        }
        else 
        {
            cJSON_AddNumberToObject(alarm_json, "count", timer_count);
            break;
        }
    }
    char *out = cJSON_PrintUnformatted(alarm_json);
	strcpy(mqtt.content, out);
	free(out);
	cJSON_Delete(alarm_json);
}

void csro_device_handle_self_message(MessageData* data)
{
    uint32_t value = 0;
    bool valid_alarm_cmd = false;
    if(csro_system_parse_level1_json_number(data->message->payload, &value, "interval"))    { csro_system_set_interval(value); }

    if(csro_system_parse_level1_json_number(data->message->payload, &value, "sysinfo"))     { if (value == 1) { csro_mqtt_msg_trigger_system(); } }

    if(csro_system_parse_level1_json_number(data->message->payload, &value, "alarminfo"))   { if (value == 1) { csro_mqtt_msg_trigger_alarm(); } }

    if(csro_system_parse_level1_json_number(data->message->payload, &value, "restart"))     { if (value == 1) { esp_restart(); } }

    if(csro_system_parse_level1_json_object(data->message->payload, "alarm_add"))  
    {
        uint32_t weekday, minutes, action, valid_count=0;
        if(csro_system_parse_level2_json_number(data->message->payload, &weekday, "alarm_add", "weekday"))   { valid_count++; }
        if(csro_system_parse_level2_json_number(data->message->payload, &minutes, "alarm_add", "minutes"))   { valid_count++; }
        if(csro_system_parse_level2_json_number(data->message->payload, &action, "alarm_add", "action"))     { valid_count++; }
        if(valid_count == 3) { csro_alarm_add(weekday, minutes, action); valid_alarm_cmd = true; }
    }
    if(csro_system_parse_level1_json_object(data->message->payload, "alarm_modify"))  
    {
        uint32_t index, weekday, minutes, action, valid_count=0;
        if(csro_system_parse_level2_json_number(data->message->payload, &index, "alarm_modify", "index"))        { valid_count++; }
        if(csro_system_parse_level2_json_number(data->message->payload, &weekday, "alarm_modify", "weekday"))    { valid_count++; }
        if(csro_system_parse_level2_json_number(data->message->payload, &minutes, "alarm_modify", "minutes"))    { valid_count++; }
        if(csro_system_parse_level2_json_number(data->message->payload, &action, "alarm_modify", "action"))      { valid_count++; }
        if(valid_count == 4) { csro_alarm_modify(index, weekday, minutes, action); valid_alarm_cmd = true; }
    }
    if(csro_system_parse_level1_json_object(data->message->payload, "alarm_delete"))  
    {
        uint32_t index;
        if(csro_system_parse_level2_json_number(data->message->payload, &index, "alarm_delete", "index"))        { csro_alarm_delete(index-1); valid_alarm_cmd = true; }
    }
    if(csro_system_parse_level1_json_object(data->message->payload, "alarm_clear"))  
    {
        uint32_t clear_value;
        if(csro_system_parse_level2_json_number(data->message->payload, &clear_value, "alarm_clear", "value"))   { if (clear_value == 1) { csro_alarm_clear(); valid_alarm_cmd = true; } }
    }
    if( valid_alarm_cmd == true) { csro_mqtt_msg_trigger_alarm(); }
    
    #ifdef NLIGHT
        csro_nlight_handle_self_message(data);
    #elif defined DLIGHT
        csro_dlight_handle_self_message(data);
    #elif defined MOTOR
        csro_motor_handle_self_message(data);
    #elif defined AQI_MONITOR
        csro_air_monitor_handle_self_message(data);
    #elif defined AIR_SYSTEM
        csro_air_system_handle_self_message(data);
    #endif
}


void csro_device_handle_hass_message(MessageData* data)
{
         #ifdef NLIGHT
            csro_nlight_handle_hass_message(data);
        #elif defined DLIGHT
            csro_dlight_handle_hass_message(data);
        #elif defined MOTOR
            csro_motor_handle_hass_message(data);
        #elif defined AQI_MONITOR
            csro_air_monitor_handle_hass_message(data);
        #elif defined AIR_SYSTEM
            csro_air_system_handle_hass_message(data);
        #endif   
}

void csro_device_handle_group_message(MessageData* data)
{
    uint32_t value = 0;
    if(csro_system_parse_level1_json_number(data->message->payload, &value, "update")) 
    {
        if(value == 1) { csro_mqtt_msg_trigger_status(NULL); }
    }
}

void csro_device_alarm_action(uint16_t action)
{
    #ifdef NLIGHT
        csro_nlight_alarm_action(action);
    #elif defined DLIGHT
        csro_dlight_alarm_action(action);
    #elif defined MOTOR
        csro_motor_alarm_action(action);
    #elif defined AQI_MONITOR
        csro_air_monitor_alarm_action(action);
    #elif defined AIR_SYSTEM
        csro_air_system_alarm_action(action);
    #endif
}

void csro_device_init(void)
{
    #ifdef NLIGHT
        csro_nlight_init();
    #elif defined DLIGHT
        csro_dlight_init();
    #elif defined MOTOR
        csro_motor_init();
    #elif defined AQI_MONITOR
        csro_air_monitor_init();
    #elif defined AIR_SYSTEM
        csro_air_system_init();
    #endif
}