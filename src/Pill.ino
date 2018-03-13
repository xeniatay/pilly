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
int sleeps = 0;
int lastActiveSecond;
bool alertedToday;

void setup() {
  alertedToday = false;
  pressedToday = false;
  lastActiveSecond = -1;
  // input pulldown to natually assume LOW when switch is not pressed
  pinMode(buttonPin, INPUT_PULLDOWN);
  pinMode(ledPin, OUTPUT);
  pinMode(D7, OUTPUT);

  // Subscribe only to events from my devices
  Particle.subscribe("button_pressed", onButtonPress, MY_DEVICES);
  Particle.subscribe("start_reminder", onStartReminder, MY_DEVICES);
  Particle.subscribe("start_alert", onStartAlert, MY_DEVICES);
  Particle.subscribe("stop_alert", onStopAlert, MY_DEVICES);
}

void onButtonpress() {
  pressedToday = true;
  ledOff();
  digitalWrite(D7, HIGH);
}

void onStartReminder() {
  // turn off current button
  // flash the other one
}

void onStartAlert() {
  ledOn();
}

void onStopAlert() {
  alertedToday = false;
  pressedToday = false;
  ledOff();
}

void loop() {
  // Check for when the button is pressed down
  if (digitalRead(buttonPin) == HIGH) {
    Particle.publish("button_pressed", PRIVATE);
  } else {
    digitalWrite(D7, LOW);
  }

  // Update the internal stats once a second
  int currentSecond = Time.second();
  if (currentSecond != lastActiveSecond) {
    lastActiveSecond = currentSecond;

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
}

void ledOff() {
  digitalWrite(ledPin, LOW);
}
