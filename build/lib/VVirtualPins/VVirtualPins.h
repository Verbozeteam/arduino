#pragma once

#include <Arduino.h>

#include "VPinState.h"

#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

#define VIRTUAL_PIN_CENTRAL_AC 0
#define VIRTUAL_PIN_ISR_LIGHT 1

#define MAX_ISR_LIGHTS 8

PinState* create_virtual_pin(uint8_t type, uint8_t data_len, char* data);

class TemperatureEngine {
    friend class CentralACPinState;

    static OneWire m_one_wire;
    static DallasTemperature m_sensors;
    static uint8_t m_num_sensors;
    static float m_cur_temp;
    static unsigned long m_next_read;

    static void discoverOneWireDevices();

public:

    static void initialize(uint8_t one_wire_pin);

    static void update(unsigned long cur_time);
};

class ISREngine {
friend class ISRLightsPinState;
friend class CustomArduinoService;
    static int m_light_ports[MAX_ISR_LIGHTS];
    static int m_light_intensities[MAX_ISR_LIGHTS];
    static int m_light_target_clocks[MAX_ISR_LIGHTS]; // mapped to from m_light_intensities to make the curve linear
    static int m_light_intensities_copies[MAX_ISR_LIGHTS]; // used at the beginning of the interrupt cycle to prevent double signals

    static int m_sync_port;
    static int m_sync_full_period;
    static int m_sync_wavelength;
    static int m_clock_tick;

    /**
     * Converts a linear curve of input to a curve that counters the output of the dimmer to make it linear
     * @param  input input (linear) curve point
     * @return       corresponding point on the countering curve
     */
    static int LinearizationFunction(int input);

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
    CentralACPinState(uint8_t temp_index);

    virtual void setOutput(uint8_t output);

    virtual uint8_t readInput();
};

/**
 * ISR Lighting array virtual pin (only one pin is necessary)
 */
class ISRLightsPinState : public PinState {
    int m_my_index;

public:
    ISRLightsPinState(uint8_t frequency, uint8_t sync_port, uint8_t out_port);

    virtual void update(unsigned long cur_time);

    virtual void setOutput(uint8_t output);

    virtual uint8_t readInput();
};

