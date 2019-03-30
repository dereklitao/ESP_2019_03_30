#include "csro_device.h"
#include "cJSON.h"

#ifdef MOTOR

#define MOTOR_MAX_ON_SEC 120

static TimerHandle_t	key_timer = NULL;
static TimerHandle_t	led_timer = NULL;
static TimerHandle_t    mtr_timer = NULL;

static csro_motor       motor[2];

static void motor_do_action(csro_motor *mtr, motor_state action)
{
    if (mtr->index + 1 > MOTOR) { return; }

    if (action == UP)
    {
        mtr->count_down = MOTOR_MAX_ON_SEC * 10;
    }
    else if (action == DOWN)
    {
        mtr->count_down = MOTOR_MAX_ON_SEC * 10;
    }
    else if (action==STOP || action==TOUP || action==TODOWN)
    {
        if (action == STOP) { mtr->count_down = 0; }
        else                { mtr->count_down = 5; }
    }
    mtr->state_now = action;
    csro_mqtt_msg_trigger_state(NULL);
}

static void motor_process_command(csro_motor *mtr, motor_state command, uint8_t source)
{
    if (mtr->index + 1 > MOTOR) { return; }

    if (command == UP || command == DOWN || command == STOP)
    {
        if(mtr->state_now == STOP)          { motor_do_action(mtr, command); }
        else if (mtr->state_now == UP)      { (command == DOWN) ? (motor_do_action(mtr, TODOWN)) : (motor_do_action(mtr, command)); }
        else if (mtr->state_now == DOWN)    { (command == UP) ? (motor_do_action(mtr, TOUP)) : (motor_do_action(mtr, command)); }
        else if (mtr->state_now == TODOWN)  { if (command != DOWN) { motor_do_action(mtr, command); } }
        else if (mtr->state_now == TOUP)    { if (command != TOUP) { motor_do_action(mtr, command); } }
        mtr->command = command;
        mtr->command_tim = datetime.time_now;
        mtr->command_source = source;
    }
}

static void motor_add_channel_json(cJSON *target, cJSON *motor_json, csro_motor *mtr)
{
	char key[20];
	sprintf(key, "motor%d", mtr->index + 1);
	cJSON_AddItemToObject(target, key, motor_json = cJSON_CreateObject());
    cJSON_AddStringToObject(motor_json, "state", mtr->state_now == UP ? "up" : mtr->state_now == DOWN ? "down" : "stop");
    cJSON_AddStringToObject(motor_json, "command", mtr->command == UP ? "up" : mtr->command == DOWN ? "down" : "stop");
    cJSON_AddNumberToObject(motor_json, "command_time", mtr->command_tim);
    cJSON_AddNumberToObject(motor_json, "command_cource", mtr->command_source);
}


void csro_motor_prepare_status_message(void)
{
    cJSON *basic_json=cJSON_CreateObject();
    cJSON *motor0 = NULL;
    cJSON *motor1 = NULL;

    if ( MOTOR >= 1 ) {  motor_add_channel_json(basic_json, motor0, &motor[0]); }
    if ( MOTOR >= 2 ) {  motor_add_channel_json(basic_json, motor1, &motor[1]); }

    char *out = cJSON_PrintUnformatted(basic_json);
	strcpy(mqtt.content, out);
	free(out);
	cJSON_Delete(basic_json);
}


void csro_motor_handle_self_message(MessageData *data)
{
    char        sub_topic[50];
    csro_systen_get_self_message_sub_topic(data, sub_topic);

    if (strcmp(sub_topic, "motor") == 0) 
    {
        char command[50];
        if (MOTOR >= 1){
            if(csro_system_parse_level1_json_string(data->message->payload, command, "motor1")) {
                    if (strcmp(command, "up") == 0)     { motor_process_command(&motor[0], UP, 1); }
                else if (strcmp(command, "stop") == 0)   { motor_process_command(&motor[0], STOP, 1); }
                else if (strcmp(command, "down") == 0)   { motor_process_command(&motor[0], DOWN, 1); }
            }
        }
        if (MOTOR >= 2) {
            if(csro_system_parse_level1_json_string(data->message->payload, command, "motor2")) {
                    if (strcmp(command, "up") == 0)     { motor_process_command(&motor[1], UP, 1); }
                else if (strcmp(command, "stop") == 0)   { motor_process_command(&motor[1], STOP, 1); }
                else if (strcmp(command, "down") == 0)   { motor_process_command(&motor[1], DOWN, 1); }
            }
        }
    }
}

void csro_motor_handle_hass_message(MessageData *data)
{

    
}

void csro_motor_alarm_action(uint16_t action)
{

}

static void key_timer_callback( TimerHandle_t xTimer )
{

}


static void led_timer_callback( TimerHandle_t xTimer )
{

}

static void mtr_timer_callback( TimerHandle_t xTimer )
{
	for(uint8_t i=0; i<MOTOR; i++)
	{
		if(motor[i].count_down > 0)
		{
			motor[i].count_down--;
			if(motor[i].count_down == 0)
			{
				if(motor[i].state_now == TOUP)			{ motor_do_action(&motor[i], UP); }
				else if(motor[i].state_now == TODOWN)	{ motor_do_action(&motor[i], DOWN); }
				else 								    { motor_do_action(&motor[i], STOP); }
			}
		}
	}
}


void csro_motor_init(void)
{
    for(size_t i = 0; i < MOTOR; i++) { motor[i].index = i; }

    key_timer = xTimerCreate("key_timer", 25/portTICK_RATE_MS, pdTRUE, (void *)0, key_timer_callback);
    led_timer = xTimerCreate("led_timer", 25/portTICK_RATE_MS, pdTRUE, (void *)0, led_timer_callback);
    mtr_timer = xTimerCreate("mtr_timer", 100/portTICK_RATE_MS, pdTRUE, (void *)0, mtr_timer_callback);
    if (key_timer != NULL) { xTimerStart(key_timer, 0); }
    if (led_timer != NULL) { xTimerStart(led_timer, 0); }
    if (mtr_timer != NULL) { xTimerStart(mtr_timer, 0); }
}

#endif