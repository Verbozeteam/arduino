#pragma once

#include "VPinState.h"

class MultiplexedPin : public PinState {
    uint8_t m_base_index;

public:
    MultiplexedPin(uint8_t base_index);

    virtual void setOutput(uint8_t output);

    virtual void update(unsigned long cur_time);

    virtual uint8_t readInput();
};

