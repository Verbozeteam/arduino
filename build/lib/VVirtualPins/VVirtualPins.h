#pragma once

#include <VPinState.h>

#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

#define VIRTUAL_PIN_CENTRAL_AC 0
#define VIRTUAL_PIN_ISR_LIGHT 1

#define MAX_ISR_LIGHTS 8

PinState* create_virtual_pin(uchar type, uchar data_len, char* data);

class TemperatureEngine {
    friend class CentralACPinState;

    static OneWire m_one_wire;
    static DallasTemperature m_sensors;
    static uchar m_num_sensors;
    static float m_cur_temp;
    static unsigned long m_next_read;

    static void discoverOneWireDevices();

public:

    static void initialize(uchar one_wire_pin);

    static void update(unsigned long cur_time);
};

class ISREngine {
friend class ISRLightsPinState;
friend class CustomArduinoService;
    static int m_light_ports[MAX_ISR_LIGHTS];
    static int m_light_intensities[MAX_ISR_LIGHTS];

    static int m_sync_port;
    static int m_sync_full_period;
    static int m_sync_wavelength;
    static int m_clock_tick;


    /**
     * function to be fired at the zero crossing to dim the light
     */
    static void zero_cross_interrupt();
    /**
     * function to be fired at some frequency to trigger the wave for the light
     */
    static void timer_interrupt();

public:

    static void initialize(int frequency=50, int sync_port=3);
    static void reset();
};

/**
 * Central AC Virtual pin
 */
class CentralACPinState : public PinState {
public:
    CentralACPinState(uchar temp_index);

    virtual void setOutput(uchar output);

    virtual uchar readInput();
};

/**
 * ISR Lighting array virtual pin (only one pin is necessary)
 */
class ISRLightsPinState : public PinState {
    int m_my_index;

public:
    ISRLightsPinState(uchar frequency, uchar sync_port, uchar out_port);

    virtual void update(unsigned long cur_time);

    virtual void setOutput(uchar output);

    virtual uchar readInput();
};

