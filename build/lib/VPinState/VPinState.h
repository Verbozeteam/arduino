#pragma once

#ifndef byte
#define byte unsigned char
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

#define PIN_MODE_INPUT  0
#define PIN_MODE_OUTPUT 1
#define PIN_MODE_PWM    2

#define VIRTUAL_PIN_TYPE_CENTRAL_AC 0
#define VIRTUAL_PIN_TYPE_ISR_LIGHT  1

#define TEMPERATURE_READ_INTERVAL 2000


class PinState {
protected:
    byte m_index;
    byte m_mode;
    byte m_type;
    unsigned long m_next_report;
    unsigned long m_read_interval; // interval (in ms) at which readings should happen

public:
    PinState(byte index = 0, byte mode = PIN_MODE_OUTPUT, byte type = PIN_TYPE_DIGITAL);

    virtual void update(unsigned long cur_time);

    virtual void setMode(byte mode);

    virtual void markForReading();

    virtual void setReadingInterval(unsigned long interval);

    virtual void setOutput(byte output) = 0;

    virtual byte readInput() = 0;
};

class DigitalPinState : public PinState {
public:
    DigitalPinState(byte index = 0, byte mode = PIN_MODE_OUTPUT);

    virtual void setOutput(byte output);

    virtual byte readInput();
};

class AnalogPinState : public PinState {
public:
    AnalogPinState(byte index = 0, byte mode = PIN_MODE_INPUT);

    virtual void setOutput(byte output);

    virtual byte readInput();
};

void init_pin_states(byte num_digital, byte num_analog, byte num_virtual);
byte on_command(byte msg_type, byte msg_len, char* command_buffer);
void pin_states_update(unsigned long cur_time);
