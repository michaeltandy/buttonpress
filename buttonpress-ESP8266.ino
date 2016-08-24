/* In the Arduino IDE and for my hardware, I choose board
 * "WeMos D1 R2 & mini" and flash size "4M (1M SPIFFS)"
 */

#include <passwords.h>
/* Put your wifi and other credentials as #defines in 
 * ~/Arduino/libraries/passwords/passwords.h
 * this lets us version control this file without 
 * sharing our passwords with the world.
 */

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const int led = 2;

const int NC_sense = 14;
const int NC_pull = 5;
const int NO_sense = 12;
const int NO_pull = 4;

extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;
extern "C" uint32_t _SPIFFS_page;
extern "C" uint32_t _SPIFFS_block;

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

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

  unsigned long button_status = 0;
  int NC_counts = 0;
  int NO_counts = 0;
  
  if (digitalRead(NC_sense) == HIGH) {
    button_status += 1;
  }

  if (digitalRead(NO_sense) == HIGH) {
    button_status += 2;
  }

  for (int i=0 ; i<50 ; i++) {
    if (digitalRead(NC_sense) == HIGH) {
      NC_counts++;
    }
    if (digitalRead(NO_sense) == HIGH) {
      NO_counts++;
    }
    delay(1);
  }

  EEPROM.begin(10);
  byte countBefore = EEPROM.read(1);
  EEPROM.write(1, countBefore+1);
  EEPROM.commit();

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
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

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

  bool heartbeatOrPowerOn = (NO_counts == 0 && NC_counts == 50) || (NO_counts == 50 && NC_counts == 0);
  
  String reqPath = String("/prod/");
  String postBody = String("{ \"buttonclosed\": ") + (NC_counts>NO_counts?"false":"true") + 
                    ", \"heartbeatOrPowerOn\": "+(heartbeatOrPowerOn?"true":"false")+"}";

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

  

  //////////////////////////////////////////////////////////

  Serial.print("Flash chip ID: ");
  Serial.println(ESP.getFlashChipId());

  Serial.print("_SPIFFS_start:");
  Serial.print(_SPIFFS_start);
  Serial.print(" @ ");
  Serial.println((uint32_t)(&_SPIFFS_start));
  Serial.print("_SPIFFS_end:");
  Serial.print(_SPIFFS_end);
  Serial.print(" @ ");
  Serial.println((uint32_t)(&_SPIFFS_end));

  uint32_t micros_in_a_second = 1000000;

  ESP.deepSleep(60*60*micros_in_a_second, RF_DEFAULT);
}

void loop() {
  // Unreachable - sleep in setup code.
}
