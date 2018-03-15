/*
* Project Pill
* Description:
* Author:
* Date:
*/

// all times is in UTC
const int ledPin = D6;
const int buttonPin = D1;
const long msPerDay = 60*60*24*1000;
const long holdThreshold = 3000;
int sleeps = 0;
int lastActiveSecond;
bool alertedToday;
bool pressedToday;
String macAddress = "";
bool blinkLed, ledIsOn;

void setup() {
  alertedToday = false;
  pressedToday = false;
  blinkLed = false;
  ledIsOn = false;
  lastActiveSecond = -1;
  // input pulldown to natually assume LOW when switch is not pressed
  pinMode(buttonPin, INPUT_PULLDOWN);
  pinMode(ledPin, OUTPUT);
  pinMode(D7, OUTPUT);

  byte mac[6];
  WiFi.macAddress(mac);
  for (int i = 0; i < 5; i++) {
    macAddress += String(mac[i], HEX) + ":";
  }
  macAddress += String(mac[5], HEX);
  // Subscribe only to events from my devices
  Particle.subscribe("button_pressed", onButtonPress, MY_DEVICES);
  Particle.subscribe("start_reminder", onStartReminder, MY_DEVICES);
  Particle.subscribe("start_alert", onStartAlert, MY_DEVICES);
  Particle.subscribe("stop_alert", onStopAlert, MY_DEVICES);

  ledOn();
}

void onButtonPress(const char *event, const char *data) {
  pressedToday = true;
  blinkLed = false;
  ledOff();
}

void onStartReminder(const char *event, const char *data) {
  // turn off current button
  // flash the other one
  if (String(data) == macAddress) {
    ledOff();
    blinkLed = false;
  } else {
    // no need to blink if already pressed today
    blinkLed = !pressedToday;
  }
}

void onStartAlert(const char *event, const char *data) {
  ledOn();
}

void onStopAlert(const char *event, const char *data) {
  alertedToday = false;
  pressedToday = false;
  blinkLed = false;
  ledOff();
}

void loop() {
  // Check for when the button is pressed down
  if (digitalRead(buttonPin) == HIGH) {
    digitalWrite(D7, HIGH);
    long startTime = millis();
    while (digitalRead(buttonPin) == HIGH) {
      delay(1);
    }
    long endTime = millis();
    if (endTime - startTime > holdThreshold) {
      Particle.publish("start_reminder", macAddress, PRIVATE);
    } else {
      Particle.publish("button_pressed", macAddress, PRIVATE);
    }
  } else {
    digitalWrite(D7, LOW);
  }

  // Update the internal stats once a second
  int currentSecond = Time.second();
  if (currentSecond != lastActiveSecond) {
    lastActiveSecond = currentSecond;
    if (blinkLed) {
      if (ledIsOn) {
        ledOff();
      } else {
        ledOn();
      }
    }

    // Resync the internal clock once a day
    if (millis() >= msPerDay) {
      Particle.syncTime();
    }

    // Reset the daily flag and turn off the button at 12pm
    // UTC = PST + 8h
    if (Time.hour() == 20 && alertedToday) {
      Particle.publish("stop_alert", PRIVATE);
    }

    // Turn on the LED at 11PM
    // UTC = PST + 8h
    if (Time.hour() == 7 && !alertedToday) {
      alertedToday = true;

      if (!pressedToday) {
        Particle.publish("start_alert", PRIVATE);
      }
    }
  }
  delay(1);
}

void ledOn() {
  digitalWrite(ledPin, HIGH);
  ledIsOn = true;
}

void ledOff() {
  digitalWrite(ledPin, LOW);
  ledIsOn = false;
}
