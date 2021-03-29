#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <StreamUtils.h>

const char *ssid = "IOT_TEST";
const char *password = "IOT_TEST";

#define LED_PIN D3

int min_num = 0;
int max_num = 80000;
//Costante PWMRANGE

//Come prova faccio una richiesta a http://www.randomnumberapi.com/api/v1.0/random?min=0&max=100
//String apiUrl = "http://www.randomnumberapi.com/api/v1.0/random?min=" + String(min_num) + "&max=" + String(max_num);
String apiUrl = "https://api.blockchain.com/v3/exchange/tickers/BTC-USD";

//Provo a creare un filtro a partire da un input esterno
//String filterJSON = "[true]";
String filterJSON = "{last_trade_price: true}"; //PER API BLOCKCHAIN

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
  BearSSL::WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
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
        //Serial.println(http.getString());
        //WiFiClient stream = http.getStream();
        callback(client);
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
  LoggingStream ls(stream, Serial);

  StaticJsonDocument<50> filter;
  deserializeJson(filter, filterJSON);

  //Necessario perché utilizzando lo stream del client WiFi sicuro la prima linea contiene la lunghezza dello stream in HEX (almeno così sembra)
  while (char(ls.peek()) != '{') {
    ls.read();
  }
  StaticJsonDocument<50> doc;
  deserializeJson(doc, ls, DeserializationOption::Filter(filter));

  Serial.println();
  Serial.println("Documento filtrato:");
  serializeJson(doc, Serial);

  out_pwm = map(doc["last_trade_price"].as<float>(), min_num, max_num, 0, PWMRANGE);

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