#pragma once

#include <Arduino.h>

// Messages sent by the Arduino
#define COMMAND_PIN_READING             0

// Messages received by the Ardunio
#define COMMAND_SET_PIN_MODE            1
#define COMMAND_SET_VIRTUAL_PIN_MODE    2
#define COMMAND_SET_PIN_OUTPUT          3
#define COMMAND_READ_PIN_INPUT          4
#define COMMAND_REGISTER_PIN_LISTENER   5
#define COMMAND_RESET_BOARD             6

#define PIN_TYPE_DIGITAL    0
#define PIN_TYPE_ANALOG     1
#define PIN_TYPE_VIRTUAL    2

#define PIN_MODE_INPUT        0
#define PIN_MODE_OUTPUT       1
#define PIN_MODE_PWM          2
#define PIN_MODE_INPUT_PULLUP 3
#define PIN_MODE_PWM_SMOOTH   4

#define VIRTUAL_PIN_TYPE_CENTRAL_AC 0
#define VIRTUAL_PIN_TYPE_ISR_LIGHT  1

#define TEMPERATURE_READ_INTERVAL 2000


class PinState {
protected:
    uint8_t m_index; // pin index on the Arduino board
    uint8_t m_index_in_middleware; // pin index in the middleware (for analog pins, m_index is e.g. 3+A0=56, but on middleware it is pin 3 (a3))
    uint8_t m_mode; // pin mode (input, output, ...)
    uint8_t m_type; // pin type (analog, digital, ...)
    uint8_t m_last_reading_sent; // last value sent to the controller. if mode is PWM_SMOOTH, this is used to indicate the last value written to the PWM
    uint8_t m_is_first_send; // whether or not this is the first time to send (using on-change mode (m_read_interval=0))
    uint8_t m_target_pwm_value; // target value while using mode PWM_SMOOTH
    unsigned long m_next_report; // time to do the next sending. If mode is PWM_SMOOTH, this is the next time the PWM value moves towards target
    unsigned long m_read_interval; // interval (in ms) at which readings should happen

public:
    PinState(uint8_t index = 0, uint8_t mode = PIN_MODE_OUTPUT, uint8_t type = PIN_TYPE_DIGITAL);
    virtual ~PinState() {}

    virtual void update(unsigned long cur_time);

    virtual void setMode(uint8_t mode);

    virtual void markForReading();

    virtual void setReadingInterval(unsigned long interval);

    virtual void setOutput(uint8_t output) = 0;

    virtual uint8_t readInput() = 0;
};

class DigitalPinState : public PinState {
public:
    DigitalPinState(uint8_t index = 0, uint8_t mode = PIN_MODE_OUTPUT);

    virtual void setOutput(uint8_t output);

    virtual uint8_t readInput();
};

class AnalogPinState : public PinState {
public:
    AnalogPinState(uint8_t index = 0, uint8_t mode = PIN_MODE_INPUT);

    virtual void setOutput(uint8_t output);

    virtual uint8_t readInput();
};

void init_pin_states(uint8_t num_digital, uint8_t num_analog, uint8_t num_virtual);
uint8_t pin_states_on_command(uint8_t msg_type, uint8_t msg_len, char* command_buffer);
void pin_states_update(unsigned long cur_time);
