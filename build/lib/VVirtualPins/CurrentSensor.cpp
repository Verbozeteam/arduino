#include "VVirtualPins.h"

CurrentSensorPinState::CurrentSensorPinState(int virtual_pin_index, int readingPin): PinState(readingPin + A0, PIN_MODE_INPUT, PIN_TYPE_VIRTUAL)  { // reading pin: 0 for A0, 1 for A1, etc...
    m_index_in_middleware = virtual_pin_index;
    float resistance = 1010.0f;
    m_emon.current(A0 + readingPin, 400);//(30.0f/0.015f)/resistance);
}

void CurrentSensorPinState::setOutput(uint8_t output) {
}

uint8_t CurrentSensorPinState::readInput() {
    double Irms = m_emon.calcIrms(1480);
    //Irms = min(max(0, (Irms * 100.0)), 255);
    return (uint8_t)Irms;
}
