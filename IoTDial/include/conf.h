#ifndef CONF_H
#define CONF_H

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h> //TaskScheduler.h throws errors on multiple inclusions

#define API_MODE 0
#define MQTT_MODE 1
#define FANTOZZI_MODE 2

extern String apiUrl;
extern String filterJSON;
extern String post_payload;
extern String path;
extern float min_value;
extern float max_value;
extern float min_pwm;
extern float max_pwm;

extern int current_mode;

extern String settings_file;

extern Scheduler ts;
extern Task request_task;

// Sets the config filename and stores it in flash as a plaintextfile named "confselect"
void set_config_file(String config_filename);

// Updates config file variable as the content of "confselect" file from flash
void set_config_file_from_flash();

// Returns config file from flash as a string
String load_conf_from_flash();

//Saves running config to flash as a json file
void running_conf_to_flash();

// Takes a JSON string and sets running config accordingly (only sets specified fields, leaves the rest untouched)
void set_conf_from_json(String json);

// Converts running config to a JSON string
String conf_to_json();

void mode_select(int mode);

#endif