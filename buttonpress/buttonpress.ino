/* In the Arduino IDE and for my hardware, I choose board
 * "WeMos D1 R2 & mini" and flash size "4M (1M SPIFFS)"
 */

#include <passwords.h>
/* Put your wifi and other credentials as #defines in 
 * ~/Arduino/libraries/passwords/passwords.h
 * this lets us version control this file without 
 * sharing our passwords with the world.
 */
#include <assert.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

ADC_MODE(ADC_VCC);

extern "C" {
#include "user_interface.h"
}

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

char MAC_string[19];

const int led = 2;
const int LED_ON = LOW;
const int LED_OFF = HIGH;

const int NC_sense = 14;
const int NC_pull = 5;
const int NO_sense = 12;
const int NO_pull = 4;

const uint32_t micros_in_a_second = 1000000;
const int maxRunTimeMillis = 30*1000;
os_timer_t timerCfg;

void setup() {
  long startTime = millis();

  {
    // Set a timer so if there's a failure
    // (e.g. wifi unavailable, server unavailable, whatever)
    // we don't stay awake forever and run down the battery.
    os_timer_disarm(&timerCfg);
    os_timer_setfn(&timerCfg, timeout, NULL);
    os_timer_arm(&timerCfg, maxRunTimeMillis, true);
  }
  
  pinMode(led, OUTPUT);
  digitalWrite(led, LED_ON);

  pinMode(NO_sense, INPUT);
  pinMode(NC_sense, INPUT);
  
  pinMode(NO_pull, OUTPUT);
  pinMode(NC_pull, OUTPUT);
  digitalWrite(NO_pull, LOW);
  digitalWrite(NC_pull, LOW);
  
  Serial.begin(115200);
  Serial.println();

  Serial.println("Connecting to WiFi.");
  WiFi.begin(ssid, password);
  populateMacString();

  bool heartbeatOrPowerOn;
  bool buttonClosed;

  {
    int NC_counts = 0;
    int NO_counts = 0;
    for (int i=0 ; i<250 ; i++) {
      if (digitalRead(NC_sense) == HIGH) {
        NC_counts++;
      }
      if (digitalRead(NO_sense) == HIGH) {
        NO_counts++;
      }
      if (i==50) {
        digitalWrite(led, LED_OFF);
      }
      delay(1);
    }
    Serial.println(String("NO counts:") + NO_counts + " NC counts:" + NC_counts);
    heartbeatOrPowerOn = (NO_counts == 0 && NC_counts == 250) || (NO_counts == 250 && NC_counts == 0);
    buttonClosed = NC_counts<NO_counts;
  }

  int espVcc = ESP.getVcc();

  {
    int i=0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(50);
      Serial.print(".");
      if (++i % 50 == 0) Serial.println("");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  long wifiTime = millis();
  Serial.print("Wifi connected in: ");
  Serial.println(wifiTime-startTime);

  const char* requestHost = AWS_API_GATEWAY_URL;
  const int requestPort = 443;

  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(requestHost);
  if (!client.connect(requestHost, requestPort)) {
    Serial.println("connection failed");
    return;
  }

  long connOpenedTime = millis();
  Serial.print("http connection opened in: ");
  Serial.println(connOpenedTime-wifiTime);

  String reqPath = String("/prod/");
  String postBody = String("{ \"buttonclosed\": ") + (buttonClosed?"true":"false") + 
                    ", \"heartbeatOrPowerOn\": "+(heartbeatOrPowerOn?"true":"false")+
                    ", \"macAddress\": \""+MAC_string+"\""+
                    ", \"wifiConnectTime\": \""+(wifiTime-startTime)+"\""+
                    ", \"httpsConnectTime\": \""+(connOpenedTime-wifiTime)+"\""+
                    ", \"espVcc\": "+espVcc+" }";

  String httpRequest = String("POST ") + reqPath + " HTTP/1.1\r\n" +
               "Host: " + requestHost + "\r\n" +
               "Content-Length: " + String(postBody.length()) + "\r\n" +
               "x-api-key: "+AWS_API_GATEWAY_KEY+"\r\n" +
               "User-Agent: SmartButtonESP8266\r\n" +
               "Connection: close\r\n\r\n"+postBody;
  
  Serial.print("Sending request with body:");
  Serial.println(postBody);
  client.print(httpRequest);
  Serial.println("request sent");

  long httpSendTime = millis();
  Serial.print("http post made in: ");
  Serial.println(httpSendTime-connOpenedTime);
  
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readString();
  Serial.println("Body: "+line);
  Serial.println("==========");
  Serial.println("closing connection");

  long httpReadTime = millis();
  Serial.print("http read in: ");
  Serial.println(httpReadTime-httpSendTime);

  digitalWrite(led, LED_ON);
  delay(50);
  digitalWrite(led, LED_OFF);

  long endTime = millis();
  Serial.print("end-to-end time: ");
  Serial.println(endTime-startTime);

  ESP.deepSleep(4*60*60*micros_in_a_second, RF_DEFAULT);
}

void populateMacString() {
  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);
  assert(sizeof(MAC_string) > 2*sizeof(MAC_array)+1);
  for (int i = 0; i < sizeof(MAC_array); ++i){
    sprintf(&MAC_string[2*i],"%02X",MAC_array[i]);
  }
}

void timeout(void *unused) {
  ESP.deepSleep(10*60*micros_in_a_second, RF_DEFAULT);
}

void loop() {
  // Unreachable - sleep in setup code.
}
