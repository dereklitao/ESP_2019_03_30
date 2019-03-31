#include "csro_common.h"
#include "csro_device.h"

static SemaphoreHandle_t status_msg_semaphore;
static SemaphoreHandle_t system_msg_semaphore;
static SemaphoreHandle_t alarm_msg_semaphore;
static TimerHandle_t status_msg_timer = NULL;

static bool mqtt_pub_messgae(char *suffix, enum QoS qos, bool retained)
{
    sprintf(mqtt.pub_topic, "%s/%s/%s/%s", mqtt.prefix, sysinfo.mac_str, sysinfo.dev_type, suffix);
    mqtt.message.payload = mqtt.content;
    mqtt.message.payloadlen = strlen(mqtt.message.payload);
    mqtt.message.qos = qos;
    mqtt.message.retained = retained;
    if (MQTTPublish(&mqtt.client, mqtt.pub_topic, &mqtt.message) != SUCCESS)
    {
        mqtt.client.isconnected = 0;
        return false;
    }
    return true;
}

static bool broker_is_connected(void)
{
    if (mqtt.client.isconnected == 1)
    {
        return true;
    }

    if (strlen((char *)mqtt.broker) == 0 || strlen((char *)mqtt.prefix) == 0)
    {
        return false;
    }

    close(mqtt.network.my_socket);
    NetworkInit(&mqtt.network);
    if (NetworkConnect(&mqtt.network, mqtt.broker, 1883) != SUCCESS)
    {
        return false;
    }

    sprintf(mqtt.sub_topic_group, "%s/group", mqtt.prefix);
    sprintf(mqtt.sub_topic_self, "%s/%s/%s/command", mqtt.prefix, sysinfo.mac_str, sysinfo.dev_type);
    sprintf(mqtt.sub_topic_hass, "%s/%s/%s/hass/#", mqtt.prefix, sysinfo.mac_str, sysinfo.dev_type);
    sprintf(mqtt.pub_topic, "%s/%s/%s/available", mqtt.prefix, sysinfo.mac_str, sysinfo.dev_type);

    MQTTClientInit(&mqtt.client, &mqtt.network, 5000, mqtt.send_buf, MQTT_BUFFER_LENGTH, mqtt.recv_buf, MQTT_BUFFER_LENGTH);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.clientID.cstring = mqtt.id;
    data.username.cstring = mqtt.name;
    data.password.cstring = mqtt.pass;
    data.willFlag = true;
    data.will.retained = true;
    data.will.topicName.cstring = mqtt.pub_topic;
    data.will.message.cstring = "offline";

    if (MQTTConnect(&mqtt.client, &data) != SUCCESS)
    {
        mqtt.client.isconnected = 0;
        return false;
    }
    if (MQTTSubscribe(&mqtt.client, mqtt.sub_topic_self, QOS1, csro_device_handle_self_message) != SUCCESS)
    {
        mqtt.client.isconnected = 0;
        return false;
    }
    if (MQTTSubscribe(&mqtt.client, mqtt.sub_topic_hass, QOS1, csro_device_handle_hass_message) != SUCCESS)
    {
        mqtt.client.isconnected = 0;
        return false;
    }
    if (MQTTSubscribe(&mqtt.client, mqtt.sub_topic_group, QOS1, csro_device_handle_group_message) != SUCCESS)
    {
        mqtt.client.isconnected = 0;
        return false;
    }

    sysinfo.serv_conn_count++;
    strcpy(mqtt.content, "online");
    return mqtt_pub_messgae("available", QOS1, true);
}

void csro_mqtt_change_status_msg_timer(void)
{
    xTimerChangePeriod(status_msg_timer, (mqtt.interval * 1000) / portTICK_RATE_MS, 0);
    xTimerReset(status_msg_timer, 0);
}

void csro_mqtt_msg_trigger_status(TimerHandle_t xTimer)
{
    xSemaphoreGive(status_msg_semaphore);
    xTimerReset(status_msg_timer, 0);
}

void csro_mqtt_msg_trigger_system(void) { xSemaphoreGive(system_msg_semaphore); }

void csro_mqtt_msg_trigger_alarm(void) { xSemaphoreGive(alarm_msg_semaphore); }

void csro_mqtt_task(void *pvParameters)
{
    static uint16_t wifi_retry = 0;
    xTaskCreate(csro_udp_receive_task, "csro_udp_receive_task", 2048, NULL, 5, NULL);

    status_msg_semaphore = xSemaphoreCreateBinary();
    xSemaphoreTake(status_msg_semaphore, 1);
    system_msg_semaphore = xSemaphoreCreateBinary();
    xSemaphoreTake(system_msg_semaphore, 1);
    alarm_msg_semaphore = xSemaphoreCreateBinary();
    xSemaphoreTake(alarm_msg_semaphore, 1);

    status_msg_timer = xTimerCreate("status_msg_timer", (mqtt.interval * 1000) / portTICK_RATE_MS, pdTRUE, (void *)0, csro_mqtt_msg_trigger_status);
    xTimerStart(status_msg_timer, 0);

    while (true)
    {
        if (csro_system_get_wifi_status())
        {
            if (broker_is_connected())
            {
                csro_system_set_status(NORMAL_START_OK);
                if (xSemaphoreTake(status_msg_semaphore, 1) == pdTRUE)
                {
                    csro_device_prepare_status_message();
                    mqtt_pub_messgae("state", QOS1, false);
                }
                if (xSemaphoreTake(system_msg_semaphore, 1) == pdTRUE)
                {
                    csro_system_prepare_message();
                    mqtt_pub_messgae("system", QOS1, false);
                }
                if (xSemaphoreTake(alarm_msg_semaphore, 1) == pdTRUE)
                {
                    csro_device_prepare_timer_message();
                    mqtt_pub_messgae("timer", QOS1, false);
                }
                if (MQTTYield(&mqtt.client, 50) != SUCCESS)
                {
                    mqtt.client.isconnected = 0;
                }
            }
            else
            {
                debug("NO BROKER.\n");
                csro_system_set_status(NORMAL_START_NOSERVER);
                vTaskDelay(1000 / portTICK_RATE_MS);
            }
        }
        else
        {
            debug("NO WIFI.\n");
            wifi_retry++;
            if (sysinfo.sc_flag == 1 && wifi_retry >= 10)
            {
                nvs_handle handle;
                nvs_open("system", NVS_READWRITE, &handle);
                nvs_set_u8(handle, "sc_flag", 0);
                nvs_commit(handle);
                nvs_close(handle);
                esp_restart();
            }
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    }
    vTaskDelete(NULL);
}