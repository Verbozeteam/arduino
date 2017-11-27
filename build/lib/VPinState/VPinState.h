#pragma once

#ifndef uchar
#define uchar unsigned char
#endif

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
    uchar m_index; // pin index on the Arduino board
    uchar m_mode; // pin mode (input, output, ...)
    uchar m_type; // pin type (analog, digital, ...)
    uchar m_last_reading_sent; // last value sent to the controller. if mode is PWM_SMOOTH, this is used to indicate the last value written to the PWM
    uchar m_is_first_send; // whether or not this is the first time to send (using on-change mode (m_read_interval=0))
    uchar m_target_pwm_value; // target value while using mode PWM_SMOOTH
    unsigned long m_next_report; // time to do the next sending. If mode is PWM_SMOOTH, this is the next time the PWM value moves towards target
    unsigned long m_read_interval; // interval (in ms) at which readings should happen

public:
    PinState(uchar index = 0, uchar mode = PIN_MODE_OUTPUT, uchar type = PIN_TYPE_DIGITAL);
    virtual ~PinState() {}

    virtual void update(unsigned long cur_time);

    virtual void setMode(uchar mode);

    virtual void markForReading();

    virtual void setReadingInterval(unsigned long interval);

    virtual void setOutput(uchar output) = 0;

    virtual uchar readInput() = 0;
};

class DigitalPinState : public PinState {
public:
    DigitalPinState(uchar index = 0, uchar mode = PIN_MODE_OUTPUT);

    virtual void setOutput(uchar output);

    virtual uchar readInput();
};

class AnalogPinState : public PinState {
public:
    AnalogPinState(uchar index = 0, uchar mode = PIN_MODE_INPUT);

    virtual void setOutput(uchar output);

    virtual uchar readInput();
};

void init_pin_states(uchar num_digital, uchar num_analog, uchar num_virtual);
uchar on_command(uchar msg_type, uchar msg_len, char* command_buffer);
void pin_states_update(unsigned long cur_time);
