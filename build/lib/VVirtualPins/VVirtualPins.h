#pragma once

#include <Arduino.h>

#include "VPinState.h"

#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

#include "Multiplexer.h"

#define ONE_WIRE_PIN 53

#define VIRTUAL_PIN_CENTRAL_AC              0
#define VIRTUAL_PIN_ISR_LIGHT               1
#define VIRTUAL_PIN_ISR_LIGHT2              2
#define VIRTUAL_PIN_MULTIPLEXED             3

#define MAX_ISR_LIGHTS 16

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
friend class ISRLights2PinState;
friend class CustomArduinoService;
    static int m_light_ports[MAX_ISR_LIGHTS];
    static int m_light_intensities[MAX_ISR_LIGHTS];
    static int m_light_target_clocks[MAX_ISR_LIGHTS]; // mapped to from m_light_intensities to make the curve linear
    static int m_light_intensities_copies[MAX_ISR_LIGHTS]; // used at the beginning of the interrupt cycle to prevent double signals
    static bool m_light_only_wave[MAX_ISR_LIGHTS]; // true means a dimmer only needs a wave at the right time, false means it needs a constant HIGH until next zero crossing

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
 * ISR Lighting array virtual pin (old dimmer: uses a pulse in the AC wave) (only one pin is necessary)
 */
class ISRLightsPinState : public PinState {
protected:
    int m_my_index;

public:
    ISRLightsPinState(uint8_t frequency, uint8_t sync_port, uint8_t out_port);

    virtual void update(unsigned long cur_time);

    virtual void setOutput(uint8_t output);

    virtual uint8_t readInput();
};

/**
 * ISR Lighting array virtual pin (newer dimmer: doesn't use pulse, just turns on the wave) (only one pin is necessary)
 */
class ISRLights2PinState : public ISRLightsPinState {
public:
    ISRLights2PinState(uint8_t frequency, uint8_t sync_port, uint8_t out_port);
};

