#include <VPinState.h>
#include <VVirtualPins.h>
#include <VCommunication.h>
#include <Arduino.h>

extern DigitalPinState* digital_pins[];
extern AnalogPinState* analog_pins[];
extern PinState* virtual_pins[];

int num_digital_pins = 0;
int num_analog_pins = 0;
int num_virtual_pins = 0;

PinState::PinState(int index, int mode, int type) : m_index(index), m_mode(mode), m_type(type) {
    pinMode(index, mode);
    m_next_report = 0;
    m_read_interval = -1;
}

void PinState::update(unsigned long cur_time) {
    if (m_mode == PIN_MODE_INPUT && cur_time >= m_next_report) {
        if (m_read_interval == -1)
            m_next_report = -1;
        else
            m_next_report = cur_time + m_read_interval;
        int reading = readInput();
        char cmd[3] = {(char)m_type, (char)m_index, (char)reading};
        send_serial_command(COMMAND_PIN_READING, 3, cmd);
    }
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

DigitalPinState::DigitalPinState(int index, int mode) : PinState(index, mode, PIN_TYPE_DIGITAL) {
}

void DigitalPinState::setOutput(int output) {
    if (m_mode == PIN_MODE_OUTPUT)
        digitalWrite(m_index, output);
    else if (m_mode == PIN_MODE_PWM)
        analogWrite(m_index, output);
}

int DigitalPinState::readInput() {
    return digitalRead(m_index);
}

AnalogPinState::AnalogPinState(int index, int mode) : PinState(index, mode, PIN_TYPE_ANALOG) {
}

void AnalogPinState::setOutput(int output) {
}

int AnalogPinState::readInput() {
    return analogRead(m_index);
}

void reset_board() {
    for (int i = 0; i < num_digital_pins; i++) {
        if (digital_pins[i]) {
            delete digital_pins[i];
            digital_pins[i] = NULL;
        }
    }
    for (int i = 0; i < num_analog_pins; i++) {
        if (analog_pins[i]) {
            delete analog_pins[i];
            analog_pins[i] = NULL;
        }
    }
    for (int i = 0; i < num_virtual_pins; i++) {
        if (virtual_pins[i]) {
            delete virtual_pins[i];
            virtual_pins[i] = NULL;
        }
    }
}

void init_pin_states(int num_digital, int num_analog, int num_virtual) {
    num_digital_pins = num_digital;
    num_analog_pins = num_analog;
    num_virtual_pins = num_virtual;
}

int on_command(int msg_type, int msg_len, char* command_buffer) {
    if (msg_type == COMMAND_RESET_BOARD) {
        reset_board();
        return 1;
    }

    if (msg_len < 2)
        return 0;

    int pin_type = (int)command_buffer[0] & 0xFF;
    int pin_index = (int)command_buffer[1] & 0xFF;

    PinState* pin;
    if (pin_type == PIN_TYPE_DIGITAL && pin_index < num_digital_pins) {
        pin = digital_pins[pin_index];
    } else if (pin_type == PIN_TYPE_ANALOG && pin_index < num_analog_pins) {
        pin = analog_pins[pin_index];
    } else if (pin_type == PIN_TYPE_VIRTUAL && pin_index < num_virtual_pins) {
        pin = virtual_pins[pin_index];
    } else
        return 0;

    switch(msg_type) {
        case COMMAND_SET_PIN_MODE: {
            if (msg_len != 3)
                return 0;
            if (!pin) {
                if (pin_type == PIN_TYPE_DIGITAL) {
                    pin = digital_pins[pin_index] = new DigitalPinState(pin_index, command_buffer[2] & 0xFF);
                } else if (pin_type == PIN_TYPE_ANALOG) {
                    pin = analog_pins[pin_index] = new AnalogPinState(pin_index, command_buffer[2] & 0xff);
                } else
                    return 0;
            } else
                pin->setMode(command_buffer[2] & 0xFF);
            break;
        }
        case COMMAND_SET_VIRTUAL_PIN_MODE: {
            if (msg_len < 3)
                return 0;

            if (pin)
                delete pin;

            pin = virtual_pins[pin_index] = create_virtual_pin(command_buffer[2] & 0xFF, msg_len-3, &command_buffer[3]);
            if (!pin)
                return 0;

            break;
        }
        case COMMAND_SET_PIN_OUTPUT: {
            if (msg_len != 3)
                return 0;
            pin->setOutput(command_buffer[2] & 0xFF);
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
            unsigned long byte1 = (unsigned long)command_buffer[2] & 0xFF;
            unsigned long byte2 = (unsigned long)command_buffer[3] & 0xFF;
            unsigned long byte3 = (unsigned long)command_buffer[4] & 0xFF;
            unsigned long byte4 = (unsigned long)command_buffer[5] & 0xFF;
            byte2 <<= 8;
            byte3 <<= 16;
            byte4 <<= 24;
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

void pin_states_update(unsigned long cur_time) {
    if (serial_is_synced()) {
        for (int i = 0; i < num_digital_pins; i++)
            if (digital_pins[i])
                digital_pins[i]->update(cur_time);
        for (int i = 0; i < num_analog_pins; i++)
            if (analog_pins[i])
                analog_pins[i]->update(cur_time);
        for (int i = 0; i < num_virtual_pins; i++)
            if (virtual_pins[i])
                virtual_pins[i]->update(cur_time);
    }
}

