#include "conf.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

void mode_select(int mode) {
  switch (mode) {
  case API_MODE:
  case FANTOZZI_MODE:
    ts.addTask(request_task);
    request_task.enableIfNot();
    current_mode = mode;

    break;

  case MQTT_MODE:
    request_task.disable();
    ts.deleteTask(request_task);
    current_mode = mode;

    break;
  default:
    Serial.println("Mode " + String(mode) + " not found");
  }
}

String load_conf_from_flash() {
  String conf = "";
  if (!LittleFS.exists(settings_file)) {
    Serial.println("ERROR: config file not found");
  } else {
    File file = LittleFS.open(settings_file, "r");
    if (!file) {
      Serial.println("ERROR: could not open config file");
    } else {
      conf = file.readString();
      file.close();
    }
  }
  return conf;
}

void running_conf_to_flash() {
  File file = LittleFS.open(settings_file, "w");
  if (!file) {
    Serial.println("ERROR: could not open config file");
  } else {
    file.print(conf_to_json());
    file.close();
    Serial.println("Running config saved to flash");
  }
}

// Takes a JSON string and sets running config accordingly (only sets specified fields, leaves the rest untouched)
void set_conf_from_json(String json) {
  DynamicJsonDocument doc(2000);
  deserializeJson(doc, json);

  if (doc.containsKey("apiUrl")) {
    apiUrl = doc["apiUrl"].as<String>();
  }
  if (doc.containsKey("filterJSON")) {
    filterJSON = doc["filterJSON"].as<String>();
  }
  JsonVariant post_payload_variant = doc["post_payload"];
  if (!post_payload_variant.isNull()) {
    post_payload = post_payload_variant.as<String>();
  } else {
    post_payload = (const char *)nullptr;
  }
  if (doc.containsKey("path")) {
    path = doc["path"].as<String>();
  }
  if (doc.containsKey("min_value")) {
    min_value = doc["min_value"].as<float>();
  }
  if (doc.containsKey("max_value")) {
    max_value = doc["max_value"].as<float>();
  }
  if (doc.containsKey("min_pwm")) {
    min_pwm = doc["min_pwm"].as<int>();
  }
  if (doc.containsKey("max_pwm")) {
    max_pwm = doc["max_pwm"].as<int>();
  }
  if (doc.containsKey("request_interval_ms")) {
    request_task.setInterval(doc["request_interval_ms"].as<int>() * TASK_MILLISECOND);
  }
}

// Converts running config to a JSON string
String conf_to_json() {
  DynamicJsonDocument doc(2000);

  doc["apiUrl"] = apiUrl;
  doc["filterJSON"] = filterJSON;
  doc["post_payload"] = post_payload;
  doc["path"] = path;
  doc["min_value"] = min_value;
  doc["max_value"] = max_value;
  doc["min_pwm"] = min_pwm;
  doc["max_pwm"] = max_pwm;
  doc["request_interval_ms"] = request_task.getInterval();

  String out;
  serializeJsonPretty(doc, out);
  return out;
}