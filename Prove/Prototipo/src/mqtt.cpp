#include "mqtt.h"
#include "conf.h"
#include <Arduino.h>

// TOPICS

String root_topic; //TODO atrent: diamo un nome definitivo... aggiungendo anche un subtopic col nome del device
String sub_to_apiurl;
String sub_to_filterJSON;
String sub_to_path;
String sub_to_min_value;
String sub_to_max_value;
String sub_to_min_pwm;
String sub_to_max_pwm;
String sub_to_interval_ms;
String sub_to_setFromJSON;

String sub_to_setValue;
String sub_to_setMode;

// Builds topic strings starting with IoTDial/<hostName>
void mqtt_create_topics() {
  root_topic = "IoTDial/" + hostName;
  sub_to_apiurl = root_topic + "/setApiUrl";
  sub_to_filterJSON = root_topic + "/setFilterJson";
  sub_to_path = root_topic + "/setPath";
  sub_to_min_value = root_topic + "/setMinValue";
  sub_to_max_value = root_topic + "/setMaxValue";
  sub_to_min_pwm = root_topic + "/setMinPwm";
  sub_to_max_pwm = root_topic + "/setMaxPwm";
  sub_to_interval_ms = root_topic + "/setRequestIntervalMs";
  sub_to_setFromJSON = root_topic + "/setFromJSON";

  sub_to_setValue = root_topic + "/setValue";
  sub_to_setMode = root_topic + "/setMode";
}

/*
   IoTDial/<hostname>/setApiUrl <valore>   : configura l'url da usare per <hostname>

   IoTDial/<hostname>/myCurrentValue <valore>   : pubblica il valore corrente del dial su <hostname>

   IoTDial/<hostname>/getButton <which?> : chiede valore button nr <which> su <hostname>
   IoTDial/<hostname>/getSwitch <which?> : chiede valore switch nr <which> su <hostname>
   IoTDial/<hostname>/switchVal <which?>,<val> : pubblica valore switch nr <which> su <hostname>

   IoTDial/<hostname>/queryMethods <wildcard?>  : chiede quali API sono disponibili su <hostname>
   IoTDial/<hostname>/queryMethod <whichMethod?>  : chiede i dettagli di <whichMethod> su <hostname>

   IoTDial/<hostname>/listOfMethods <getSwitch,getButton,...>  : pubblica quali API sono disponibili su <hostname>
*/

//TODO atrent: bisogna espandere l'API MQTT, parliamone la prox volta, tipo:
// - aggiungere un config in un colpo solo (minV maxV minP maxP)
// - aggiungere blink e beep
// ecc.
// cioè estendiamo l'alfabeto dei comandi via MQTT che in fondo l'oggetto non deve fare poi tante cose, è meglio se i test e i calcoli li facciamo fare ad altri e poi qui li fisicalizziamo (cmq va benissimo il PoC dell'ATM!)
// l'oggetto inoltre mi sa che è meglio parta in modalità MQTT di default così è più facile integrarlo ad esempio in un sistema di home automation (da citare nella tesi!)
// ***potresti addirittura implementare una sorta di "reflection" MQTT in modo da poter interrogare l'oggetto per capire cosa offre (pulsanti, commutatori, ecc.)***

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

void mqtt_callback_setValue(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]:\n" + message + "\n");
  if (current_mode == MQTT_MODE) {
    scanning = false;
    out_value = message.toFloat();
  }
}

void mqtt_callback_setMode(String topic, String message) {
  Serial.println("Message arrived [" + topic + "]:\n" + message + "\n");
  mode_select(message.toInt());
}

void mqtt_reconnect() {
  Serial.print("Attempting MQTT connection...");
  String clientId = hostName;
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

    mqtt_tools.subscribe(sub_to_setValue, mqtt_callback_setValue);
    mqtt_tools.subscribe(sub_to_setMode, mqtt_callback_setMode);
  } else {
    Serial.print("failed, rc=");
    Serial.println(mqtt_client.state());
  }
}

//TODO atrent: secondo me dovrebbe dare un po' di feedback MQTT oltre che visualizzare col dial, in modo da sapere cosa sta visualizzando