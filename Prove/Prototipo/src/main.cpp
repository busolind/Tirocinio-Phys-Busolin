#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWiFiManager.h>
#include <FS.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <StreamUtils.h>
#include <TaskScheduler.h>

const char *hostName = "esp-async";
const char *mqtt_server = "192.168.178.5";

#define LED_PIN D3

#define REQUEST_DELAY_MS 10000
#define MQTT_RECONNECT_DELAY 5000

//Possibilmente successivamente impostati "da fuori"

float min_value = 0;
float max_value = 80000;
float min_pwm = 0;
float max_pwm = PWMRANGE;

String root_topic = "Phys";
String sub_to_apiurl = root_topic + "/setApiUrl";
String sub_to_filterJSON = root_topic + "/setFilterJson";
String sub_to_path = root_topic + "/setPath";
String sub_to_min_value = root_topic + "/setMinValue";
String sub_to_max_value = root_topic + "/setMaxValue";
String sub_to_min_pwm = root_topic + "/setMinPwm";
String sub_to_max_pwm = root_topic + "/setMaxPwm";
String sub_to_interval_ms = root_topic + "/setRequestIntervalMs";
String sub_to_setFromJSON = root_topic + "/setFromJSON";

String settings_file = "/settings.json";

//Come prova faccio una richiesta a http://www.randomnumberapi.com/api/v1.0/random?min=0&max=100
//String apiUrl = "http://www.randomnumberapi.com/api/v1.0/random?min=" + String(int(min_value)) + "&max=" + String(int(max_value)); //http_random
//String apiUrl = "https://api.blockchain.com/v3/exchange/tickers/BTC-USD";
//String apiUrl = "https://api.ratesapi.io/api/latest";
//String apiUrl = "https://api.zippopotam.us/us/90210"; //Beverly Hills (scelta perché ha un mix di oggetti e array)
String apiUrl = "https://csrng.net/csrng/csrng.php?min=" + String(int(min_value)) + "&max=" + String(int(max_value)); //https_random

//Provo a creare un filtro a partire da un input esterno
//String filterJSON = "[true]"; //http_random
//String filterJSON = "{last_trade_price: true}"; //PER API BLOCKCHAIN
//String filterJSON = "{\"places\": [{\"latitude\": true}]}"; //Beverly Hills
//String filterJSON = "{rates: {MXN: true}}"; //PER API RATES
String filterJSON = "[{random: true}]"; //https_random

//String path = "0"; //http_random
//String path = "last_trade_price"; //PER API BLOCKCHAIN
//String path = "places/0/latitude"; //Beverly Hills
String path = "0/random"; //https_random

int out_pwm;

Scheduler ts;
WiFiClient espClient;
PubSubClient mqtt_client(mqtt_server, 1883, espClient);
PubSubClientTools mqtt_tools(mqtt_client);

AsyncWebServer server(80);
DNSServer dns;

void set_conf_from_json(String json);
String conf_to_json();

//Restituisce il file di configurazione salvato nella flash sotto forma di stringa
String load_conf_from_flash() {
  String conf = "";
  if (!LittleFS.exists(settings_file)) {
    Serial.println("ERRORE: file di configurazione non trovato");
  } else {
    File file = LittleFS.open(settings_file, "r");
    if (!file) {
      Serial.println("ERRORE: apertura file di configurazione non riuscita");
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
    Serial.println("ERRORE: apertura file di configurazione non riuscita");
  } else {
    file.print(conf_to_json());
    file.close();
    Serial.println("Configurazione salvata su flash");
  }
}

void setup_ws() {
  server.serveStatic("/", LittleFS, "/static/").setDefaultFile("index.html");

  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");

    // POST setFromJSON value on <ESP_IP>/post
    if (request->params() < 1) {
      response->print("Nothing to do!");
    } else {
      if (request->hasParam("setFromJSON", true)) {
        String inputMessage = request->getParam("setFromJSON", true)->value();
        String inputParam = "setFromJSON";

        Serial.println("Messaggio ricevuto via POST: ");
        Serial.println(inputMessage);

        set_conf_from_json(inputMessage);
        response->print("HTTP POST request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage + "<br><br>");
      }
      if (request->hasParam("saveToFlash", true)) {
        running_conf_to_flash();
        response->print("Settings saved to flash.<br>");
      }
    }
    response->print("<a href=\"/\">Return to Home Page</a>");
    response->setCode(200);
    request->send(response);
  });

  server.on("/get/running-conf", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", conf_to_json());
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
  server.begin();
}

void setup_wifi() {
  AsyncWiFiManager wifiManager(&server, &dns);

  //WiFi.mode(WIFI_AP_STA);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("prototipo-phys");

  while (WiFi.status() != WL_CONNECTED) {
    if (!wifiManager.autoConnect("AutoConnectAP")) {
      Serial.println("Failed to connect");
      // ESP.restart();
    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //WiFi.softAP(hostName);
}

void http_request(void (*callback)(Stream &)) {
  WiFiClient client;
  HTTPClient http;

  Serial.print("[HTTP] begin... ");
  Serial.println("URL: " + apiUrl);
  if (http.begin(client, apiUrl)) { // HTTP

    //Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    //http.addHeader("Content-Type", "application/json");
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        WiFiClient stream = http.getStream();
        callback(stream);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("[HTTP] Unable to connect\n");
  }
}

void https_request(void (*callback)(Stream &)) {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  Serial.print("[HTTPS] begin... ");
  Serial.println("URL: " + apiUrl);
  if (http.begin(client, apiUrl)) { // HTTP

    //Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    //http.addHeader("Content-Type", "application/json");
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        callback(client);
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
}

void stream_callback(Stream &stream) {
  LoggingStream ls(stream, Serial);

  StaticJsonDocument<200> filter;
  deserializeJson(filter, filterJSON);

  Serial.println();
  Serial.print("Filter: ");
  serializeJson(filter, Serial);
  Serial.println();

  //Necessario perché utilizzando lo stream del client WiFi sicuro la prima linea contiene la lunghezza dello stream in HEX (almeno così sembra)
  Serial.println("--- Inizio parte esclusa da JSON ---");
  while (char(ls.peek()) != '{' && char(ls.peek()) != '[') {
    ls.read();
  }
  Serial.println("---  Fine parte esclusa da JSON  ---");
  Serial.println();

  DynamicJsonDocument doc(2000);
  deserializeJson(doc, ls, DeserializationOption::Filter(filter));
  //deserializeJson(doc, ls);

  Serial.println();
  Serial.println("Documento filtrato:");
  serializeJson(doc, Serial);

  //INIZIO CANTIERE TEST PARSING PATH
  JsonVariant jv = doc.as<JsonVariant>();

  int path_buf_len = 50;
  char path_buf[path_buf_len];
  char *token;

  path.toCharArray(path_buf, path_buf_len);
  token = strtok(path_buf, "/");
  Serial.println();
  while (token != NULL) {
    //controlla se è numero o no
    bool object = false;
    char *p = token;
    while (*p != 0) {
      if (!isDigit(*p)) {
        object = true;
      }
      p++;
    }

    if (object) {
      jv = jv[token];
    } else {
      jv = jv[atoi(token)];
    }
    Serial.print("Token: " + String(token) + " ");
    Serial.println(object ? "[OBJ]" : "[ARRAY]");
    token = strtok(NULL, "/");
  }

  float value = jv.as<float>();
  //FINE CANTIERE

  /*
  JsonVariant jv = doc.as<JsonVariant>();
  jv = jv["rates"];
  serializeJson(jv, Serial);

  float value = doc["rates"]["MXN"].as<float>();
  */

  Serial.println();
  Serial.println("Valore estratto: " + String(value));
  Serial.println("Minimo: " + String(min_value));
  Serial.println("Massimo: " + String(max_value));

  //out_pwm = map(value, min_value, max_value, min_pwm, max_pwm);
  out_pwm = (value - min_value) * (max_pwm - min_pwm) / (max_value - min_value) + min_pwm;

  Serial.println();
  Serial.println("Mappato a : " + String(out_pwm));
}

void request_task_callback() {
  if (apiUrl.startsWith("https://")) {
    https_request(stream_callback);
  } else {
    http_request(stream_callback);
  }

  analogWrite(LED_PIN, out_pwm);
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
}
Task request_task(REQUEST_DELAY_MS *TASK_MILLISECOND, TASK_FOREVER, request_task_callback);

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
Task mqtt_reconnect_task(MQTT_RECONNECT_DELAY *TASK_MILLISECOND, TASK_FOREVER, mqtt_reconnect);

void set_conf_from_json(String json) {
  DynamicJsonDocument doc(2000);
  deserializeJson(doc, json);

  if (doc.containsKey("apiUrl")) {
    apiUrl = doc["apiUrl"].as<String>();
  }
  if (doc.containsKey("filterJSON")) {
    filterJSON = doc["filterJSON"].as<String>();
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

String conf_to_json() {
  DynamicJsonDocument doc(2000);

  doc["apiUrl"] = apiUrl;
  doc["filterJSON"] = filterJSON;
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

void setup() {
  Serial.begin(115200);
  setup_wifi();
  setup_ws();
  pinMode(LED_PIN, OUTPUT);
  LittleFS.begin();

  Serial.println("Avvio con configurazione hardcoded, provo a caricare da file...");
  set_conf_from_json(load_conf_from_flash());
  delay(3000);

  Serial.println(conf_to_json());
  delay(3000);

  ts.addTask(mqtt_reconnect_task);
  ts.addTask(request_task);
}

void loop() {
  if (!mqtt_client.connected()) {
    mqtt_reconnect_task.enableIfNot();
  } else {
    mqtt_reconnect_task.disable();
  }

  if (WiFi.status() == WL_CONNECTED) {
    request_task.enableIfNot();
  } else {
    request_task.disable();
  }

  mqtt_client.loop();

  ts.execute();
}