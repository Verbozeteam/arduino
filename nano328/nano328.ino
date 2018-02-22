#include "VCommunication.h"
#include "VPinState.h"
#include "VVirtualPins.h"
#include "VZigbee.h"

#define _NUM_DIGITAL_PINS 12
#define _NUM_ANALOG_PINS 8
#define _NUM_VIRTUAL_PINS 8
#define ONE_WIRE_PIN 13

DigitalPinState* digital_pins[_NUM_DIGITAL_PINS] = { NULL };
AnalogPinState* analog_pins[_NUM_ANALOG_PINS] = { NULL };
PinState* virtual_pins[_NUM_VIRTUAL_PINS] = { NULL };

void setup() {
    communication_init(&Serial, pin_states_on_command);

    // nanos connect to zigbees on the serial port!
    zigbeeInit(&Serial, NULL, "2", "C", "3332");

    TemperatureEngine::initialize(ONE_WIRE_PIN);
    ISREngine::reset();

    init_pin_states(_NUM_DIGITAL_PINS, _NUM_ANALOG_PINS, _NUM_VIRTUAL_PINS);
}

void loop() {
    unsigned long cur_time = millis();

    communication_update(cur_time);

    TemperatureEngine::update(cur_time);

    pin_states_update(cur_time);
}


