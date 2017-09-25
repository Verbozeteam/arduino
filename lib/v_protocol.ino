
// Messages sent by the Arduino
#define COMMAND_PIN_READING             0

// Messages received by the Ardunio
#define COMMAND_SET_PIN_MODE           1
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
    char m_index;
    char m_mode;
    unsigned long m_next_report;
    unsigned long m_read_interval; // interval (in ms) at which readings should happen

public:
    PinState(int index = 0, int mode = PIN_MODE_OUTPUT) : m_index(index), m_mode(mode) {
        pinMode(index, mode);
        if (mode == 1)
            digitalWrite(index, mode);
        m_next_report = 0;
        m_read_interval = -1;
    }

    virtual void setMode(int mode) {
        m_mode = mode;
        setPinMode(m_index, m_mode);
    }

    virtual void markForReading() {
        m_next_report = 0; // next cycle must read this pin immediately
    }

    virtual void setOutput(int output) = 0;
    virtual int readInput() = 0;
};

class DigitalPinState : public PinState {
public:
    DigitalPinState(int index = 0, int mode = PIN_MODE_OUTPUT) : PinState(index, mode) {
    }

    virtual void setOutput(int output) {
        if (m_mode == PIN_MODE_OUTPUT)
            digitalWrite(m_index, output);
        else if (m_mode == PIN_MODE_PWM)
            analogWrite(m_index, output);
    }

    virtual int readInput() {
        if (m_mode == PIN_MODE_INPUT)
            return digitalRead(m_index);
        return 0;
    }
};

class AnalogPinState : public PinState {
public:
    AnalogPinState(int index = 0, int mode = PIN_MODE_INPUT) : PinState(index, mode) {
    }

    virtual void setOutput(int output) {}

    virtual int readInput() {
        if (m_mode == PIN_MODE_INPUT)
            return analogRead(m_index);
        return 0;
    }
};

// virtual pin
class CentralACPinState : public PinState {
public:
    CentralACPinState() : PinState() {

    }

    virtual void setOutput(int output) {
    }

    virtual int readInput() {
    }
}

int on_command(int msg_type, int msg_len, char* command_buffer) {
    if (msg_len < 2)
        return 0;

    int pin_type = (int)command_buffer[0];
    int pin_index = (int)command_buffer[1];

    PinState* pin;
    if (pin_type == PIN_TYPE_DIGITAL && pin_index < NUM_DIGITAL_PINS)
        pin = digital_pins[pin_index];
    else if (pin_type == PIN_TYPE_ANALOG && pin_index < NUM_ANALOG_PINS)
        pin = analog_pins[pin_index];
    else if (pin_type == PIN_TYPE_VIRTUAL && pin_index < NUM_VIRTUAL_PINS)
        pin = virtual_pins[pin_index];
    else
        return 0;

    switch(msg_type) {
        case COMMAND_SET_PIN_MODE: {
            if (msg_len != 3)
                return 0;
            pin->setMode(command_buffer[2])
            break;
        }
        case COMMAND_SET_PIN_OUTPUT: {
            if (msg_len != 3)
                return 0;
            pin->setOutput(command_buffer[2]);
            break;
        }
        case COMMAND_READ_PIN_INPUT: {
            if (msg_len != 2)
                return 0;
            pin->markForReading();
            break;
        }
        case COMMAND_REGISTER_PIN_LISTENER: {
            if (msg_len != 6)
                return 0;
            unsigned long byte1 = (unsigned long)command_buffer[2];
            unsigned long byte2 = (unsigned long)command_buffer[3];
            unsigned long byte3 = (unsigned long)command_buffer[4];
            unsigned long byte4 = (unsigned long)command_buffer[5];
            byte2 >>= 8;
            byte3 >>= 16;
            byte4 >>= 24;
            byte1 |= byte2;
            byte1 |= byte3;
            byte1 |= byte4;
            pin->setReadingInterval(byte1);
            break;
        }
        default:
            return 0;
    }

    return 1;
}
