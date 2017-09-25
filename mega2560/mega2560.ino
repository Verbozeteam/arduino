
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

#include <VSerialCommunication.h>
#include <VPinState.h>

#define NUM_DIGITAL_PINS 54
#define NUM_ANALOG_PINS 16
#define NUM_VIRTUAL_PINS 1

DigitalPinState* digital_pins[NUM_DIGITAL_PINS];
AnalogPinState* analog_pins[NUM_ANALOG_PINS];
PinState* virtual_pins[NUM_VIRTUAL_PINS];

void setup() {
    for (int i = 0; i < NUM_DIGITAL_PINS; i++)
        digital_pins[i] = new DigitalPinState(i);
    for (int i = 0; i < NUM_ANALOG_PINS; i++)
        analog_pins[i] = new AnalogPinState(i);

    virtual_pins[0] = new CentralACPinState();

    serial_init();
}

void loop() {
    unsigned long cur_time = millis();

    serial_update(cur_time);
}


