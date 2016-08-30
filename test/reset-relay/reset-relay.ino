/**
 * Target: Arduino Uno
 * 
 * Tests our smart button battery performance by simulating lots of presses.
 * 
 * A relay module replaces the pushbutton (on pin 13)
 * the program below cycles between short and long press and release.
 * 
 * Meanwhile, the voltage on pin A0  and the ESP8266 module's LED on pin 12
 * are monitored. These are logged to the serial port.
 * 
 * Over the course of a few hours (or days) we expect to see the battery
 * voltage drop until the module stops functioning.
 * 
 * Results can be logged from a PC with minicom, e.g.
 *     minicom -C acm0log.txt -D /dev/ttyACM0 -b 115200
 */

const int relay = 13;
const int feedbackFromEsp = 12;
const int targetVoltagePin = 0;

long lastRunEpoch = -1;
const long epochLengthMillis = 10*1000;

void setup() {
  pinMode(relay, OUTPUT);
  pinMode(feedbackFromEsp, INPUT);
  
  Serial.begin(115200);
  Serial.println();
}

void loop() {
  unsigned long startTime = millis();
  int thisEpoch = startTime/epochLengthMillis;
  
  if (thisEpoch > lastRunEpoch) {
    performTest(thisEpoch, startTime);
    lastRunEpoch = thisEpoch;
  }
}

void performTest(int thisEpoch, unsigned long timeMillis) {
  float analogBefore = analogRead(targetVoltagePin) * 5.0 / 1024.0;
  
  Serial.println(String("Epoch ") + thisEpoch + " voltage before " + analogBefore);
  
  switch (thisEpoch % 4) {
    case 0:
      Serial.println("Switching high");
      digitalWrite(relay, HIGH);
      break;
    case 1:
      Serial.println("Pulsing low");
      digitalWrite(relay, LOW);
      delay(100);
      digitalWrite(relay, HIGH);
      break;
    case 2:
      Serial.println("Switching low");
      digitalWrite(relay, LOW);
      break;
    case 3:
      Serial.println("Pulsing high");
      digitalWrite(relay, HIGH);
      delay(100);
      digitalWrite(relay, LOW);
      break;
  }
  
  delay(100); // Allow time to boot & really put a load on the power supply.
  float analogAfter = analogRead(targetVoltagePin) * 5.0 / 1024.0;
  Serial.println(String("Voltage after ") + analogAfter);

  Serial.print("Awaiting feedback led toggle...");
  while (digitalRead(feedbackFromEsp) == HIGH) {}
  while (digitalRead(feedbackFromEsp) == LOW) {}
  while (digitalRead(feedbackFromEsp) == HIGH) {}
  while (digitalRead(feedbackFromEsp) == LOW) {}
  Serial.println(" OK");

  unsigned long awakeDuration = millis()-timeMillis;
  
  Serial.println(String("Summary,") + thisEpoch + "," + timeMillis 
        + "," + analogBefore + "," + analogAfter + "," + awakeDuration + "\n");
}


