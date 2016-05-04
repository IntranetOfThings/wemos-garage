#include <Homie.h>

#define FW_NAME "wemos-garage"
#define FW_VERSION "0.0.2"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

#define PIN_RELAY       D1
#define PIN_SENSOR      D5
#define OPENER_EVENT_MS 1000
#define DEBOUNCER_MS    50

// keep track of when the opener is pressed so we can un-set the
// relay after a short time to simulate a button press
unsigned long openerEvent = 0;

// bounce is built into Homie, so you can use it without including it first
Bounce debouncer = Bounce();
int lastSensorValue = -1;

HomieNode openerNode("opener", "relay");
HomieNode doorNode("door", "sensor");

bool openerHandler(String value) {
  if (value == "1" || value == "on" || value == "true") {
    digitalWrite(PIN_RELAY, HIGH);
    openerEvent = millis();
  }
  return true;
}

void setupHandler() {
  // initialise our hardware pins
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_SENSOR, INPUT_PULLUP);

  // make sure the relay is off/open
  digitalWrite(PIN_RELAY, LOW);  

  // initialise the debouncer
  debouncer.attach(PIN_SENSOR);
  debouncer.interval(DEBOUNCER_MS);
}

void loopHandler() {
  int sensorValue = debouncer.read();

  if (sensorValue != lastSensorValue) {
    if (Homie.setNodeProperty(doorNode, "state", sensorValue ? "open" : "closed", true)) {
      lastSensorValue = sensorValue;
    }
  }
}

void setup() {
  Homie.setFirmware(FW_NAME, FW_VERSION);

  // publish '1' to /opener/press/set
  openerNode.subscribe("press", openerHandler);  

  Homie.registerNode(openerNode);
  Homie.registerNode(doorNode);

  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);
  
  Homie.setup();
}

void loop() {
  // if the opener has been pressed, then un-set after a short time
  // this code has preference to ensure the relay is released even if
  // we lose WIFI or MQTT connectivity and homie has to reconnect
  if (openerEvent && (millis() - openerEvent >= OPENER_EVENT_MS)) {
    digitalWrite(PIN_RELAY, LOW);
    openerEvent = 0;
  }

  // let homie do its processing loop
  Homie.loop();

  // let the debouncer do its processing loop
  debouncer.update();
}
