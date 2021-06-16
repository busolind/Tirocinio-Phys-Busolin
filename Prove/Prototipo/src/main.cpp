#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "conf.h"
#include "mqtt.h"
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

//TODO atrent: rinomina e sposta dir progetto, non più Prove e non più Prototipo ;)

//TODO atrent: ripensare/aggiungere, deve diventare un sistema modulare che permetta di "attivare" (con dei #define) varie funzionalità tipo commutatori ecc.

// MAIN CONFIG
const char *hostName = "prototipo-phys"; //TODO atrent: deve diventare autogenerato, magari aggiungendo il macaddress, altrimenti tutti sono uguali, pensiamo ad un main topic fisso e poi subtopic identificativi
#define METER D2

// atrent
#define RGBLED_DATA D3
#define RGBLED_CLOCK D4
#define BUZZER D5
#define LED D0
#define PUSHBTN D7
#define SWITCH A0

//TODO atrent: cominciare a usare anche almeno LED e BUZZER (ad esempio per segnalare variazioni molto grandi o estremi dei range)
//TODO atrent: definire due funzioni (tipo bing e blink) per attivare genericamente LED e BUZZER

//TODO atrent: fornire anche un meccanismo di taratura del range tensioni? magari basta solo uno sketch a parte, minimale

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

int current_mode = API_MODE;

String settings_file = "/settings.json";

int out_pwm;
float out_value;

// SPECIFIC TO SCANNING MODE
bool scanning = true;
bool rising = true;

// MQTT CONFIG

const char *mqtt_server = "mqtt.atrent.it";
#define MQTT_RECONNECT_DELAY 30000

WiFiClient espClient;
PubSubClient mqtt_client(mqtt_server, 1883, espClient);
PubSubClientTools mqtt_tools(mqtt_client);

AsyncWebServer server(80);
DNSServer dns;

Scheduler ts;

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

// Maps out_value to out_pwm and writes to "analog" output, handles scanning animation
void write_output() {
  if (!scanning) {
    //out_pwm = map(value, min_value, max_value, min_pwm, max_pwm);
    out_pwm = (out_value - min_value) * (max_pwm - min_pwm) / (max_value - min_value) + min_pwm;
  } else {
    if ((rising && out_pwm >= max_pwm) || (!rising && out_pwm <= min_pwm)) {
      rising = !rising;
    }
    if (rising) {
      out_pwm += max_pwm / 25;
    } else {
      out_pwm -= max_pwm / 25;
    }
  }
  if (out_pwm < 0)
    out_pwm = 0;
  if (out_pwm > PWMRANGE)
    out_pwm = PWMRANGE;
  analogWrite(METER, out_pwm);
}
Task write_output_task(100 * TASK_MILLISECOND, TASK_FOREVER, write_output);

// HTTP(S) request. On successful request returns the response stream to the callback
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
      if (current_mode == FANTOZZI_MODE) {
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      }
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

// HTTP(S) callback. Takes a stream as a parameter, parses the json within it, extracts the required value as specified in config (filter + path)
void stream_callback(Stream &stream) {
  //LoggingStream ls(stream, Serial);
  ReadBufferingStream rbs(stream, 64);

  StaticJsonDocument<200> filter;
  deserializeJson(filter, filterJSON);

  Serial.println();
  Serial.print("Filter: ");
  serializeJson(filter, Serial);
  Serial.println();

  // Necessary because the secure wifi client stream sometimes contains (apparently) the stream length in HEX before the response
  Serial.println("--- Start of text excluded from stream ---");
  while (rbs.available() && char(rbs.peek()) != '{' && char(rbs.peek()) != '[') {
    Serial.print(char(rbs.read()));
  }
  Serial.println("---  End of text excluded from stream  ---");
  Serial.println();

  DynamicJsonDocument doc(2000);
  deserializeJson(doc, rbs, DeserializationOption::Filter(filter));
  //deserializeJson(doc, ls);

  Serial.println();
  Serial.println("Filtered Document:");
  serializeJson(doc, Serial);

  //Path parsing to extract desired value

  JsonVariant jv = doc.as<JsonVariant>();

  int path_buf_len = 50;
  char path_buf[path_buf_len];
  char *token;

  path.toCharArray(path_buf, path_buf_len);
  token = strtok(path_buf, "/");
  Serial.println();
  while (token != NULL) {
    //checks if current token is completely numeric (index in array) or not (key in object)
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

  String value_string = jv.as<String>();

  if (value_string == "ricalcolo" || value_string == "*") {
    scanning = true;
  } else {
    scanning = false;
    out_value = value_string.toFloat(); //Advantage over jv.as<float>: String::toFloat converts numbers even when the string contains
                                        //characters other than digits (e.g. "3 min" -> 3.00)
  }

  Serial.println();
  Serial.println("Extracted value: " + String(out_value));
}

// Function called by request task. It makes the HTTP or HTTPS request and prints some debug information
void request_task_callback() {
  http_s_request(stream_callback, post_payload);

  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());

  Serial.print("Millis: ");
  Serial.println(millis());
}
Task request_task(REQUEST_DELAY_MS *TASK_MILLISECOND, TASK_FOREVER, request_task_callback);

Task mqtt_reconnect_task(MQTT_RECONNECT_DELAY *TASK_MILLISECOND, TASK_FOREVER, mqtt_reconnect);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  setup_ota();
  //setup_ws();
  pinMode(METER, OUTPUT);
  LittleFS.begin();

  ts.addTask(mqtt_reconnect_task);
  ts.addTask(write_output_task);
  ts.addTask(request_task);
  mode_select(current_mode);

  Serial.println("Started with hardcoded config, trying to load from file...");
  set_conf_from_json(load_conf_from_flash());

  Serial.println(conf_to_json());
  delay(3000);

  write_output_task.enable();
}

void loop() {
  //TODO atrent: si possono mettere *tutte* le attività nei task ;)

  if (!mqtt_client.connected()) {
    mqtt_reconnect_task.enableIfNot();
  } else {
    mqtt_reconnect_task.disable();
  }

  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
  }

  mqtt_client.loop();
  ts.execute();
}