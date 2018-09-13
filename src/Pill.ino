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

bool ackedReminder = false;
bool ackedButton = false;

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
  Particle.subscribe("ack_reminder", ackReminder, MY_DEVICES);
  Particle.subscribe("ack_button", ackButton, MY_DEVICES);
  ledOn();
}

void ackReminder(const char *event, const char *data) {
  // only accept the ack from the other mac address
  if (String(data) != macAddress) {
    ackedReminder = true;
  }
}

void ackButton(const char *event, const char *data) {
  if (String(data) != macAddress) {
    ackedButton = true;
  }
}

void onButtonPress(const char *event, const char *data) {
  if (String(data) != macAddress) {
    Particle.publish("ack_button", macAddress, PRIVATE);
    ledOff();
  }
}

void onStartReminder(const char *event, const char *data) {
  // flash all the receivers
  if (String(data) != macAddress) {
    Particle.publish("ack_reminder", macAddress, PRIVATE);
    // no need to blink if already pressed today
    blinkLed = !pressedToday;
  }
}

void sendReminder() {
  ackedReminder = false;
  for (int i = 0; i < 6; i++) {
    Particle.publish("start_reminder", macAddress, PRIVATE);
    delay(5000); // allow 5s for this response
    if (ackedReminder) {
      return;
    }
  }
  ledOn();
}

void sendButton() {
  ackedButton = false;
  for (int i = 0; i < 6; i++) {
    Particle.publish("button_pressed", macAddress, PRIVATE);
    delay(5000); // allow 5s for this response
    if (ackedButton) {
      return;
    }
  }
  ledOn();
}

void loop() {
  // Check for when the button is pressed down
  if (digitalRead(buttonPin) == HIGH) {
    digitalWrite(D7, HIGH);
    long startTime = millis();
    bool isLongClick = false;
    int delayUntilTimeCheck = 250;
    while (digitalRead(buttonPin) == HIGH) {
      if (--delayUntilTimeCheck == 0) {
        delayUntilTimeCheck = 250;
        if (millis() - startTime > holdThreshold) {
            isLongClick = true;
            ledOff();
        }
      }
      delay(1);
    }
    if (isLongClick) {
      sendReminder();
    } else {
      sendButton();
    }
    pressedToday = true;
    blinkLed = false;
  } else {
    digitalWrite(D7, LOW); // onboard LED
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
    // UTC = PST + 8h (7 during daylight savings)
    if (Time.hour() == 19 && alertedToday) {
      alertedToday = false;
      pressedToday = false;
      blinkLed = false;
      ledOff();
    }

    // Turn on the LED at 11PM
    // UTC = PST + 8h
    if (Time.hour() == 6 && !alertedToday) {
      alertedToday = true;

      if (!pressedToday) {
        ledOn();
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
