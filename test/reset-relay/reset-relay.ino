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
  
  bool success = awaitLedFeedback();
  unsigned long awakeDuration = millis()-timeMillis;
  
  Serial.println(String("Summary,") + thisEpoch + "," + timeMillis 
        + "," + analogBefore + "," + analogAfter + "," + awakeDuration
        + "," + success + "\n");
}

bool awaitLedFeedback() {
  Serial.print("Awaiting feedback led toggle...");
  bool sawToggles = (awaitLedValueOrTime(LOW)
      && awaitLedValueOrTime(HIGH)
      && awaitLedValueOrTime(LOW)
      && awaitLedValueOrTime(HIGH));
  if (sawToggles) {
    Serial.println(" OK");
    return true;
  } else {
    Serial.println(" not seen. Continuing.");
    return false;
  }
}

bool awaitLedValueOrTime(bool waitFor) {
  uint32_t i=0;
  while (true) {
    if (digitalRead(feedbackFromEsp) == waitFor) {
      return true;
    }
    if (i++ >= 60000) {
      return false;
    }
    delay(1);
  }
}

