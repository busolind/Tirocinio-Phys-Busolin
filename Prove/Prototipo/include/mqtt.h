#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <TaskSchedulerDeclarations.h> //TaskScheduler.h throws errors on multiple inclusions

extern String apiUrl;
extern String filterJSON;
extern String post_payload;
extern String path;
extern float min_value;
extern float max_value;
extern float min_pwm;
extern float max_pwm;

extern int current_mode;
extern float out_value;
extern bool scanning;

extern PubSubClient mqtt_client;
extern PubSubClientTools mqtt_tools;

extern Task request_task;

void mqtt_callback_setApiUrl(String topic, String message);
void mqtt_callback_setFilterJson(String topic, String message);
void mqtt_callback_setPath(String topic, String message);
void mqtt_callback_setMinValue(String topic, String message);
void mqtt_callback_setMaxValue(String topic, String message);
void mqtt_callback_setMinPwm(String topic, String message);
void mqtt_callback_setMaxPwm(String topic, String message);
void mqtt_callback_setRequestIntervalMs(String topic, String message);
void mqtt_callback_setFromJSON(String topic, String message);

void mqtt_callback_setValue(String topic, String message);
void mqtt_callback_setMode(String topic, String message);

// (Try to) Connect to MQTT server and subscribe
void mqtt_reconnect();

#endif