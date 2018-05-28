#include "VCommunication.h"
#include "VPinState.h"
#include "VVirtualPins.h"

#define _NUM_DIGITAL_PINS 53
#define _NUM_ANALOG_PINS 16
#define _NUM_VIRTUAL_PINS 16
#define ONE_WIRE_PIN 53

pin_state_t digital_pins[_NUM_DIGITAL_PINS];
pin_state_t analog_pins[_NUM_ANALOG_PINS];
pin_state_t virtual_pins[_NUM_VIRTUAL_PINS];

void setup() {
    communication_init(&Serial, pin_states_on_command);
    communication_init_logging(&Serial1);

    TemperatureEngine::initialize(ONE_WIRE_PIN);
    ISREngine::reset();

    init_pin_states(_NUM_DIGITAL_PINS, _NUM_ANALOG_PINS, _NUM_VIRTUAL_PINS);

    LOG_INFO("setup() complete");
}

void loop() {
    unsigned long cur_time = millis();

    communication_update(cur_time);

    TemperatureEngine::update(cur_time);

    pin_states_update(cur_time);
}
