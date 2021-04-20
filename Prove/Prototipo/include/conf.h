#ifndef CONF_H
#define CONF_H

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

extern String apiUrl;
extern String filterJSON;
extern String post_payload;
extern String path;
extern float min_value;
extern float max_value;
extern float min_pwm;
extern float max_pwm;

extern String settings_file;

extern Task request_task;

// Returns config file from flash as a string
String load_conf_from_flash();

//Saves running config to flash as a json file
void running_conf_to_flash();

// Takes a JSON string and sets running config accordingly (only sets specified fields, leaves the rest untouched)
void set_conf_from_json(String json);

// Converts running config to a JSON string
String conf_to_json();

#endif