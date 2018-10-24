#include "VCommunication.h"
#include "VPinState.h"
#include "VVirtualPins.h"

#define _NUM_DIGITAL_PINS 53
#define _NUM_ANALOG_PINS 16
#define _NUM_VIRTUAL_PINS 16
#define ONE_WIRE_PIN 53

DigitalPinState* digital_pins[_NUM_DIGITAL_PINS] = { NULL };
AnalogPinState* analog_pins[_NUM_ANALOG_PINS] = { NULL };
PinState* virtual_pins[_NUM_VIRTUAL_PINS] = { NULL };

void setup() {
    communication_init(&Serial, pin_states_on_command);
    communication_init_logging(&Serial1);

    TemperatureEngine::initialize(ONE_WIRE_PIN);
    ISREngine::reset();

    init_pin_states(_NUM_DIGITAL_PINS, _NUM_ANALOG_PINS, _NUM_VIRTUAL_PINS);

    LOG_INFO("setup() complete");
}

unsigned long last_time = 0;
void loop() {
    unsigned long cur_time = millis();
    if (cur_time < last_time) {
        pin_states_time_rollover();
        communication_time_rollover();
    }
    last_time = cur_time;

    communication_update(cur_time);

    TemperatureEngine::update(cur_time);

    pin_states_update(cur_time);
}
