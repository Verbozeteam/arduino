#pragma once

// Messages sent by the Arduino
#define COMMAND_PIN_READING             0

// Messages received by the Ardunio
#define COMMAND_SET_PIN_MODE            1
#define COMMAND_SET_PIN_OUTPUT          2
#define COMMAND_READ_PIN_INPUT          3
#define COMMAND_REGISTER_PIN_LISTENER   4

#define PIN_TYPE_DIGITAL    0
#define PIN_TYPE_ANALOG     1
#define PIN_TYPE_VIRTUAL    2

#define PIN_MODE_INPUT  0
#define PIN_MODE_OUTPUT 1
#define PIN_MODE_PWM    2


class PinState {
public: // Fuck it.
    char m_index;
    char m_mode;
    char m_type;
    unsigned long m_next_report;
    unsigned long m_read_interval; // interval (in ms) at which readings should happen

public:
    PinState(int index = 0, int mode = PIN_MODE_OUTPUT, int type = PIN_TYPE_DIGITAL);

    virtual void update(unsigned long cur_time);

    virtual void setMode(int mode);

    virtual void markForReading();

    virtual void setReadingInterval(unsigned long interval);

    virtual void setOutput(int output) = 0;

    virtual int readInput() = 0;
};

class DigitalPinState : public PinState {
public:
    DigitalPinState(int index = 0, int mode = PIN_MODE_OUTPUT);

    virtual void setOutput(int output);

    virtual int readInput();
};

class AnalogPinState : public PinState {
public:
    AnalogPinState(int index = 0, int mode = PIN_MODE_INPUT);

    virtual void setOutput(int output);

    virtual int readInput();
};

// virtual pin
class CentralACPinState : public PinState {
public:
    CentralACPinState();

    virtual void setOutput(int output);

    virtual int readInput();
};

void init_pin_states(int num_digital, int num_analog, int num_virtual);
int on_command(int msg_type, int msg_len, char* command_buffer);
void pin_states_update(unsigned long cur_time);
