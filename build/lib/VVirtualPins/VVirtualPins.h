#pragma once

#include <VPinState.h>

#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

PinState* create_virtual_pin(int type, int data_len, char* data);

class TemperatureEngine {
    friend class CentralACPinState;

    static OneWire m_one_wire;
    static DallasTemperature m_sensors;
    static int m_num_sensors;
    static float m_cur_temp;
    static unsigned long m_next_read;

    static void discoverOneWireDevices();

public:

    static void initialize(int one_wire_pin);

    static void update(unsigned long cur_time);
};

/**
 * Central AC Virtual pin
 */
class CentralACPinState : public PinState {
public:
    CentralACPinState(int temp_index);

    virtual void setOutput(int output);

    virtual int readInput();
};
