#include "csro_common.h"
#include "cJSON.h"

bool csro_system_parse_level1_json_object(char *msg, char *object_name)
{
    cJSON *json = cJSON_Parse(msg);
    if (json == NULL)
    {
        goto EXIT;
    }

    cJSON *object = cJSON_GetObjectItem(json, object_name);
    if (object == NULL || (object->type != cJSON_Object))
    {
        goto EXIT;
    }

    cJSON_Delete(json);
    return true;

EXIT:
    cJSON_Delete(json);
    return false;
}

bool csro_system_parse_level1_json_number(char *msg, uint32_t *dest, char *object_name)
{
    cJSON *json = cJSON_Parse(msg);
    if (json == NULL)
    {
        goto EXIT;
    }

    cJSON *object = cJSON_GetObjectItem(json, object_name);
    if (object == NULL || (object->type != cJSON_Number))
    {
        goto EXIT;
    }

    dest[0] = object->valueint;

    cJSON_Delete(json);
    return true;

EXIT:
    cJSON_Delete(json);
    return false;
}

bool csro_system_parse_level2_json_number(char *msg, uint32_t *dest, char *object_name, char *sub_object_name)
{
    cJSON *json = cJSON_Parse(msg);
    if (json == NULL)
    {
        goto EXIT;
    }

    cJSON *object = cJSON_GetObjectItem(json, object_name);
    if (object == NULL)
    {
        goto EXIT;
    }

    cJSON *sub_object = cJSON_GetObjectItem(object, sub_object_name);
    if (sub_object == NULL || (sub_object->type != cJSON_Number))
    {
        goto EXIT;
    }

    dest[0] = sub_object->valueint;

    cJSON_Delete(json);
    return true;

EXIT:
    cJSON_Delete(json);
    return false;
}

bool csro_system_parse_level1_json_string(char *msg, char *dest, char *object_name)
{
    cJSON *json = cJSON_Parse(msg);
    if (json == NULL)
    {
        goto EXIT;
    }

    cJSON *object = cJSON_GetObjectItem(json, object_name);
    if (object == NULL || (object->type != cJSON_String))
    {
        goto EXIT;
    }

    strcpy(dest, object->valuestring);

    cJSON_Delete(json);
    return true;

EXIT:
    cJSON_Delete(json);
    return false;
}

bool csro_system_parse_level2_json_string(char *msg, char *dest, char *object_name, char *sub_object_name)
{
    cJSON *json = cJSON_Parse(msg);
    if (json == NULL)
    {
        goto EXIT;
    }

    cJSON *object = cJSON_GetObjectItem(json, object_name);
    if (object == NULL)
    {
        goto EXIT;
    }

    cJSON *sub_object = cJSON_GetObjectItem(object, sub_object_name);
    if (sub_object == NULL || (sub_object->type != cJSON_String))
    {
        goto EXIT;
    }

    strcpy(dest, sub_object->valuestring);

    cJSON_Delete(json);
    return true;

EXIT:
    cJSON_Delete(json);
    return false;
}

void csro_systen_get_hass_message_sub_topic(MessageData *data, char *sub_topic)
{
    char topic[200];
    bzero(topic, 200);
    strncpy(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
    char *segment;
    segment = strtok(topic, "/");
    while (segment)
    {
        bzero(sub_topic, 50);
        strcpy(sub_topic, segment);
        segment = strtok(NULL, "/");
    }
    debug("subtopic:%s\n", sub_topic);
}
