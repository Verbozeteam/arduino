#include "VVirtualPins.h"
// #include "Tlc5940.h"

bool multiplexing_initialized = false;

MultiplexedPin::MultiplexedPin(uint8_t base_index)
        : PinState(2, PIN_MODE_OUTPUT, PIN_TYPE_VIRTUAL) {
    // if (!multiplexing_initialized) {
    //     multiplexing_initialized = true;
    //     Tlc.init(0);
    // }

    // m_base_index = base_index;

    // setOutput(0);
}

void MultiplexedPin::setOutput(uint8_t output) {
    // for (int i = 0; i < 8; i++) {
    //     int pinOutput = ((output >> i) & 0x1) ? 4095 : 0;
    //     Tlc.set(m_base_index + i, pinOutput);
    // }
    // Tlc.update();
}
void MultiplexedPin::update(unsigned long cur_time) {
}

uint8_t MultiplexedPin::readInput() {
    // return 0;
}
