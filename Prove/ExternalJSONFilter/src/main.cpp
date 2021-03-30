#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <StreamUtils.h>

const char *ssid = "IOT_TEST";
const char *password = "IOT_TEST";

#define LED_PIN D3

//Possibilmente successivamente impostati "da fuori"

float min_num = 0;
float max_num = 150;
//Costante PWMRANGE

//Come prova faccio una richiesta a http://www.randomnumberapi.com/api/v1.0/random?min=0&max=100
//String apiUrl = "http://www.randomnumberapi.com/api/v1.0/random?min=" + String(min_num) + "&max=" + String(max_num);
//String apiUrl = "https://api.blockchain.com/v3/exchange/tickers/BTC-USD";
//String apiUrl = "https://api.ratesapi.io/api/latest";
String apiUrl = "https://api.zippopotam.us/us/90210"; //Beverly Hills (scelta perché ha un mix di oggetti e array)

//Provo a creare un filtro a partire da un input esterno
//String filterJSON = "[true]";
//String filterJSON = "{last_trade_price: true}"; //PER API BLOCKCHAIN
String filterJSON = "{\"places\": [{\"latitude\": true}]}";
//String filterJSON = "{rates: {MXN: true}}"; //PER API RATES
//String path = "last_trade_price";
String path = "places/0/latitude";

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

  StaticJsonDocument<200> filter;
  deserializeJson(filter, filterJSON);

  //Necessario perché utilizzando lo stream del client WiFi sicuro la prima linea contiene la lunghezza dello stream in HEX (almeno così sembra)
  while (char(ls.peek()) != '{') {
    ls.read();
  }
  StaticJsonDocument<1000> doc;
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

  //out_pwm = map(value, min_num, max_num, 0, PWMRANGE);
  out_pwm = (value - min_num) * (PWMRANGE - 0) / (max_num - min_num) + 0;

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