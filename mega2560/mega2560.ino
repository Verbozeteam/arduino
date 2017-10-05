#include <VCommunication.h>
#include <VPinState.h>
#include <VVirtualPins.h>

#define NUM_DIGITAL_PINS 53
#define NUM_ANALOG_PINS 16
#define NUM_VIRTUAL_PINS 8
#define ONE_WIRE_PIN 53

DigitalPinState* digital_pins[NUM_DIGITAL_PINS] = { NULL };
AnalogPinState* analog_pins[NUM_ANALOG_PINS] = { NULL };
PinState* virtual_pins[NUM_VIRTUAL_PINS] = { NULL };

void setup() {
    serial_init(&Serial);

    TemperatureEngine::initialize(ONE_WIRE_PIN);
    ISREngine::reset();

    init_pin_states(NUM_DIGITAL_PINS, NUM_ANALOG_PINS, NUM_VIRTUAL_PINS);
}

void loop() {
    unsigned long cur_time = millis();

    serial_update(cur_time);

    TemperatureEngine::update(cur_time);

    pin_states_update(cur_time);
}


