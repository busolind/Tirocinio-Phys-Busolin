#include "mqtt.h"
#include "conf.h"
#include <Arduino.h>

// TOPICS

String root_topic = "PrototipoPhys";
String sub_to_apiurl = root_topic + "/setApiUrl";
String sub_to_filterJSON = root_topic + "/setFilterJson";
String sub_to_path = root_topic + "/setPath";
String sub_to_min_value = root_topic + "/setMinValue";
String sub_to_max_value = root_topic + "/setMaxValue";
String sub_to_min_pwm = root_topic + "/setMinPwm";
String sub_to_max_pwm = root_topic + "/setMaxPwm";
String sub_to_interval_ms = root_topic + "/setRequestIntervalMs";
String sub_to_setFromJSON = root_topic + "/setFromJSON";

// Callbacks:

void mqtt_callback_setApiUrl(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]: " + message);
  apiUrl = message;
}

void mqtt_callback_setFilterJson(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]: " + message);
  filterJSON = message;
}

void mqtt_callback_setPath(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]: " + message);
  path = message;
}

void mqtt_callback_setMinValue(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]: " + message);
  min_value = message.toFloat();
}

void mqtt_callback_setMaxValue(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]: " + message);
  max_value = message.toFloat();
}

void mqtt_callback_setMinPwm(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]: " + message);
  min_pwm = message.toInt();
}

void mqtt_callback_setMaxPwm(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]: " + message);
  max_pwm = message.toInt();
}

void mqtt_callback_setRequestIntervalMs(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]: " + message);
  request_task.setInterval(message.toInt() * TASK_MILLISECOND);
}

void mqtt_callback_setFromJSON(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]:\n" + message + "\n");
  set_conf_from_json(message);
}

void mqtt_reconnect() {
  Serial.print("Attempting MQTT connection...");
  String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
  // Attempt to connect
  if (mqtt_client.connect(clientId.c_str())) {
    Serial.println("connected");
    mqtt_tools.subscribe(sub_to_apiurl, mqtt_callback_setApiUrl);
    mqtt_tools.subscribe(sub_to_filterJSON, mqtt_callback_setFilterJson);
    mqtt_tools.subscribe(sub_to_path, mqtt_callback_setPath);
    mqtt_tools.subscribe(sub_to_setFromJSON, mqtt_callback_setFromJSON);
    mqtt_tools.subscribe(sub_to_min_value, mqtt_callback_setMinValue);
    mqtt_tools.subscribe(sub_to_max_value, mqtt_callback_setMaxValue);
    mqtt_tools.subscribe(sub_to_min_pwm, mqtt_callback_setMinPwm);
    mqtt_tools.subscribe(sub_to_max_pwm, mqtt_callback_setMaxPwm);
    mqtt_tools.subscribe(sub_to_interval_ms, mqtt_callback_setRequestIntervalMs);
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqtt_client.state());
  }
}