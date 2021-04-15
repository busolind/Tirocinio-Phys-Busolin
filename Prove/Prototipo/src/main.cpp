#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWiFiManager.h>
#include <FS.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <StreamUtils.h>
#include <TaskScheduler.h>

// MAIN CONFIG
const char *hostName = "prototipo-phys";
#define OUT_PIN D3

// Default hardcoded values. If a config file is present on flash, its settings will be used instead.
String apiUrl = "https://csrng.net/csrng/csrng.php?min=0&max=150"; //https_random
String filterJSON = "[{random: true}]";                            //https_random
String post_payload;
String path = "0/random"; //https_random
float min_value = 0;
float max_value = 150;
float min_pwm = 0;
float max_pwm = PWMRANGE;
#define REQUEST_DELAY_MS 10000 //Request interval. Can be edited in config

String settings_file = "/settings.json";

int out_pwm;

// MQTT CONFIG

const char *mqtt_server = "192.168.178.5";
#define MQTT_RECONNECT_DELAY 30000

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

WiFiClient espClient;
PubSubClient mqtt_client(mqtt_server, 1883, espClient);
PubSubClientTools mqtt_tools(mqtt_client);

AsyncWebServer server(80);
DNSServer dns;

Scheduler ts;

void set_conf_from_json(String json);
String conf_to_json();

// Returns config file from flash as a string
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

//Saves running config to flash as a json file
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

// Web server setup
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

        Serial.println("Message received via POST: ");
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

  server.on("/get/saved-conf", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", load_conf_from_flash());
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
  server.begin();
}

//OTA setup
void setup_ota() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  //ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  ArduinoOTA.setPassword("prototipo");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
      LittleFS.end();
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("[OTA] Ready");
  Serial.println(WiFi.localIP());
}

// WiFi setup
void setup_wifi() {
  AsyncWiFiManager wifiManager(&server, &dns);

  //WiFi.mode(WIFI_AP_STA);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostName);

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

// HTTP(S) request. On successful request returns a stream to the callback
void http_s_request(void (*callback)(Stream &), String post_payload = (const char *)nullptr) {
  WiFiClient client;
  BearSSL::WiFiClientSecure ssl_client;
  ssl_client.setInsecure();

  HTTPClient http;

  bool use_https = apiUrl.startsWith("https://");

  bool begin;
  if (use_https) {
    begin = http.begin(ssl_client, apiUrl);
    Serial.print("[HTTPS] ");
  } else {
    begin = http.begin(client, apiUrl);
    Serial.print("[HTTP] ");
  }

  Serial.print("begin... ");
  Serial.println("URL: " + apiUrl);
  if (begin) {
    // start connection and send HTTP header
    int httpCode;
    if (post_payload == nullptr || post_payload == "" || post_payload == NULL) {
      httpCode = http.GET();
      Serial.print("GET... ");
    } else {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //TEMPORARY
      httpCode = http.POST(post_payload);
      Serial.print("POST... ");
    }

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        if (use_https) {
          callback(ssl_client);
        } else {
          callback(http.getStream());
        }
      }
    } else {
      Serial.printf("failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("ERROR: Unable to connect\n");
  }
}

// HTTP(S) callback. Takes a stream as a parameter, parses the json within it, extracts the required value as specified in config (filter + path) and maps the out_pwm value accordingly
void stream_callback(Stream &stream) {
  LoggingStream ls(stream, Serial);

  StaticJsonDocument<200> filter;
  deserializeJson(filter, filterJSON);

  Serial.println();
  Serial.print("Filter: ");
  serializeJson(filter, Serial);
  Serial.println();

  // Necessary because the secure wifi client stream sometimes contains (apparently) the stream length in HEX before the response
  Serial.println("--- Start of text excluded from stream ---");
  while (ls.available() && char(ls.peek()) != '{' && char(ls.peek()) != '[') {
    ls.read();
  }
  Serial.println("---  End of text excluded from stream  ---");
  Serial.println();

  DynamicJsonDocument doc(2000);
  deserializeJson(doc, ls, DeserializationOption::Filter(filter));
  //deserializeJson(doc, ls);

  Serial.println();
  Serial.println("Filtered Document:");
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

  float value = jv.as<String>().toFloat(); //Advantage over as<float>: String::toFloat converts numbers even when the string contains characters other than digits
                                           // e.g. "3 min" -> 3.00
  //FINE CANTIERE

  /*
  JsonVariant jv = doc.as<JsonVariant>();
  jv = jv["rates"];
  serializeJson(jv, Serial);

  float value = doc["rates"]["MXN"].as<float>();
  */

  Serial.println();
  Serial.println("Extracted value: " + String(value));
  Serial.println("Min value: " + String(min_value));
  Serial.println("Max value: " + String(max_value));

  //out_pwm = map(value, min_value, max_value, min_pwm, max_pwm);
  out_pwm = (value - min_value) * (max_pwm - min_pwm) / (max_value - min_value) + min_pwm;

  Serial.println();
  Serial.println("Mapped to: " + String(out_pwm));
}

// Function called by request task. It makes the HTTP or HTTPS request and sets the PWM output according to out_pwm
void request_task_callback() {
  http_s_request(stream_callback, post_payload);

  analogWrite(OUT_PIN, out_pwm);
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
}
Task request_task(REQUEST_DELAY_MS *TASK_MILLISECOND, TASK_FOREVER, request_task_callback);

// MQTT FUNCTIONS:
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

// (Try to) Connect to MQTT server and subscribe
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

void setup() {
  Serial.begin(115200);
  setup_wifi();
  setup_ota();
  setup_ws();
  pinMode(OUT_PIN, OUTPUT);
  LittleFS.begin();

  Serial.println("Started with hardcoded config, trying to load from file...");
  set_conf_from_json(load_conf_from_flash());

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
    ArduinoOTA.handle();
  } else {
    request_task.disable();
  }

  mqtt_client.loop();

  ts.execute();
}