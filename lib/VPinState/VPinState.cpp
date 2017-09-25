#include <VPinState.h>
#include <Arduino.h>

extern DigitalPinState* digital_pins[];
extern AnalogPinState* analog_pins[];
extern PinState* virtual_pins[];

int num_digital_pins = 0;
int num_analog_pins = 0;
int num_virtual_pins = 0;

inline void init_pin_states(int num_digital, int num_analog, int num_virtual) {
    num_digital_pins = num_digital;
    num_analog_pins = num_analog;
    num_virtual_pins = num_virtual;
}

PinState::PinState(int index, int mode) : m_index(index), m_mode(mode) {
    pinMode(index, mode);
    if (mode == 1)
        digitalWrite(index, mode);
    m_next_report = 0;
    m_read_interval = -1;
}

void PinState::setMode(int mode) {
    m_mode = mode;
    pinMode(m_index, m_mode);
}

void PinState::markForReading() {
    m_next_report = 0; // next cycle must read this pin immediately
}

void PinState::setReadingInterval(unsigned long interval) {
    m_read_interval = interval;
}

DigitalPinState::DigitalPinState(int index, int mode) : PinState(index, mode) {
}

void DigitalPinState::setOutput(int output) {
    if (m_mode == PIN_MODE_OUTPUT)
        digitalWrite(m_index, output);
    else if (m_mode == PIN_MODE_PWM)
        analogWrite(m_index, output);
}

int DigitalPinState::readInput() {
    if (m_mode == PIN_MODE_INPUT)
        return digitalRead(m_index);
    return 0;
}

AnalogPinState::AnalogPinState(int index, int mode) : PinState(index, mode) {
}

void AnalogPinState::setOutput(int output) {}

int AnalogPinState::readInput() {
    if (m_mode == PIN_MODE_INPUT)
        return analogRead(m_index);
    return 0;
}

CentralACPinState::CentralACPinState() : PinState() {

}

void CentralACPinState::setOutput(int output) {
}

int CentralACPinState::readInput() {
}

int on_command(int msg_type, int msg_len, char* command_buffer) {
    if (msg_len < 2)
        return 0;

    int pin_type = (int)command_buffer[0];
    int pin_index = (int)command_buffer[1];

    PinState* pin;
    if (pin_type == PIN_TYPE_DIGITAL && pin_index < num_digital_pins)
        pin = digital_pins[pin_index];
    else if (pin_type == PIN_TYPE_ANALOG && pin_index < num_analog_pins)
        pin = analog_pins[pin_index];
    else if (pin_type == PIN_TYPE_VIRTUAL && pin_index < num_virtual_pins)
        pin = virtual_pins[pin_index];
    else
        return 0;

    switch(msg_type) {
        case COMMAND_SET_PIN_MODE: {
            if (msg_len != 3)
                return 0;
            pin->setMode(command_buffer[2]);
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
