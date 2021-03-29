#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <StreamUtils.h>

const char *ssid = "IOT_TEST";
const char *password = "IOT_TEST";

#define LED_PIN D3

//Come prova faccio una richiesta a http://www.randomnumberapi.com/api/v1.0/random?min=0&max=100

int min_num = 0;
int max_num = 100;
//Costante PWMRANGE

String apiUrl = "http://www.randomnumberapi.com/api/v1.0/random?min=" + String(min_num) + "&max=" + String(max_num);

WiFiClient client;
HTTPClient http;

unsigned long last_action = 0;
int out_pwm;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void http_request(void (*callback)(Stream &)) {
  Serial.print("[HTTP] begin...\n");
  if (http.begin(client, apiUrl)) { // HTTP

    //Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        WiFiClient stream = http.getStream();
        //Serial.println(payload);
        callback(stream);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
  }
}

void http_callback(Stream &stream) {
  //Serial.println(res);
  LoggingStream ls(stream, Serial);

  StaticJsonDocument<50> doc;
  deserializeJson(doc, ls);

  out_pwm = map(doc[0].as<int>(), min_num, max_num, 0, PWMRANGE);

  Serial.println();
  Serial.println("Mappato a : " + String(out_pwm));
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  unsigned long now = millis();
  if (now - last_action > 5000) {
    last_action = now;
    if (WiFi.status() == WL_CONNECTED) {
      http_request(http_callback);
    }
    analogWrite(LED_PIN, out_pwm);
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
  }
}