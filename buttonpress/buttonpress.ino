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

extern "C" {
#include "spi_flash.h"
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

void setup() {
  long startTime = millis();
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
  Serial.println(String("connecting to ") + ssid);
  WiFi.begin(ssid, password);
  populateMacString();

  unsigned long button_status = 0;
  int NC_counts = 0;
  int NO_counts = 0;
  
  if (digitalRead(NC_sense) == HIGH) {
    button_status += 1;
  }

  if (digitalRead(NO_sense) == HIGH) {
    button_status += 2;
  }

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

  Serial.print("NO counts:");
  Serial.print(NO_counts);
  Serial.print(" NC counts:");
  Serial.print(NC_counts);
  Serial.println();

  if (digitalRead(NC_sense) == HIGH) {
    button_status += 10;
  }

  if (digitalRead(NO_sense) == HIGH) {
    button_status += 20;
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  long wifiTime = millis();
  Serial.print("Wifi connected in: ");
  Serial.println(wifiTime-startTime);

  if (digitalRead(NC_sense) == HIGH) {
    button_status += 100;
  }

  if (digitalRead(NO_sense) == HIGH) {
    button_status += 200;
  }

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

  bool heartbeatOrPowerOn = (NO_counts == 0 && NC_counts == 250) || (NO_counts == 250 && NC_counts == 0);
  
  String reqPath = String("/prod/");
  String postBody = String("{ \"buttonclosed\": ") + (NC_counts>NO_counts?"false":"true") + 
                    ", \"heartbeatOrPowerOn\": "+(heartbeatOrPowerOn?"true":"false")+
                    ", \"macAddress\": \""+MAC_string+"\" }";

  String httpRequest = String("POST ") + reqPath + " HTTP/1.1\r\n" +
               "Host: " + requestHost + "\r\n" +
               "Content-Length: " + String(postBody.length()) + "\r\n" +
               "x-api-key: "+AWS_API_GATEWAY_KEY+"\r\n" +
               "User-Agent: SmartButtonESP8266\r\n" +
               "Connection: close\r\n\r\n"+postBody;
  
  Serial.print("Sending request:");
  Serial.println(httpRequest);
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

  Serial.print("SPI_FLASH_SEC_SIZE: ");
  Serial.println(SPI_FLASH_SEC_SIZE);

  digitalWrite(led, LED_ON);
  delay(50);
  digitalWrite(led, LED_OFF);

  long endTime = millis();
  Serial.print("end-to-end time: ");
  Serial.println(endTime-startTime);

  uint32_t micros_in_a_second = 1000000;
  ESP.deepSleep(60*60*micros_in_a_second, RF_DEFAULT);
}

void populateMacString() {
  uint8_t MAC_array[6];
  WiFi.macAddress(MAC_array);
  assert(sizeof(MAC_string) > 2*sizeof(MAC_array)+1);
  for (int i = 0; i < sizeof(MAC_array); ++i){
    sprintf(&MAC_string[2*i],"%02X",MAC_array[i]);
  }
}

void loop() {
  /*digitalWrite(led, HIGH);
  delay(2000);
  digitalWrite(led, LOW);
  testFlash();*/
  // Unreachable - sleep in setup code.
}
