#include "VPinState.h"
#include "VVirtualPins.h"
#include "VCommunication.h"
#include "Arduino.h"

extern DigitalPinState* digital_pins[];
extern AnalogPinState* analog_pins[];
extern PinState* virtual_pins[];

uint8_t num_digital_pins = 0;
uint8_t num_analog_pins = 0;
uint8_t num_virtual_pins = 0;

PinState::PinState(uint8_t index, uint8_t mode, uint8_t type) : m_index(index), m_index_in_middleware(index), m_mode(mode), m_type(type) {
    if (type != PIN_TYPE_VIRTUAL)
        setMode(mode);
    m_next_report = 0;
    m_read_interval = -1;
    m_last_reading_sent = 3;
    m_is_first_send = 1;
    m_target_pwm_value = 0;
}

void PinState::update(unsigned long cur_time) {
    if (m_mode == PIN_MODE_INPUT || m_mode == PIN_MODE_INPUT_PULLUP) {
        if (m_read_interval == 0) {
            uint8_t reading = readInput();
            if (reading != m_last_reading_sent || m_is_first_send) {
                m_is_first_send = 0;
                m_last_reading_sent = reading;
                char cmd[3] = {(char)m_type, (char)m_index_in_middleware, (char)reading};
                communication_send_command(COMMAND_PIN_READING, 3, cmd);
            }
        } else if (cur_time >= m_next_report) {
            if (m_read_interval == -1)
                m_next_report = -1;
            else
                m_next_report = cur_time + m_read_interval;
            uint8_t reading = readInput();
            char cmd[3] = {(char)m_type, (char)m_index_in_middleware, (char)reading};
            communication_send_command(COMMAND_PIN_READING, 3, cmd);
        }
    } else if (m_mode == PIN_MODE_PWM_SMOOTH) {
        if (cur_time >= m_next_report && m_target_pwm_value != m_last_reading_sent) {
            m_next_report = cur_time + 10;
            if (m_target_pwm_value > m_last_reading_sent)
                m_last_reading_sent++;
            else
                m_last_reading_sent--;
            analogWrite(m_index, m_last_reading_sent);
        }
    }
}

void PinState::setMode(uint8_t mode) {
    #ifdef __SHAMMAM_SIMULATION__
        printf("PIN MODE [%d] -> %d\n", m_index, mode);
    #endif
    m_mode = mode;
    if (m_mode == PIN_MODE_INPUT)
        pinMode(m_index, INPUT);
    else if (m_mode == PIN_MODE_OUTPUT || m_mode == PIN_MODE_PWM || m_mode == PIN_MODE_PWM_SMOOTH) {
        pinMode(m_index, OUTPUT);
        if (m_mode == PIN_MODE_PWM_SMOOTH) {
            m_last_reading_sent = 0;
            m_next_report = 0;
            analogWrite(m_index, 0);
        }
    } else if (m_mode == PIN_MODE_INPUT_PULLUP)
        pinMode(m_index, INPUT_PULLUP);
}

void PinState::markForReading() {
    m_next_report = 0; // next cycle must read this pin immediately
}

void PinState::setReadingInterval(unsigned long interval) {
    m_read_interval = interval;
    m_next_report = 0;
}

DigitalPinState::DigitalPinState(uint8_t index, uint8_t mode) : PinState(index, mode, PIN_TYPE_DIGITAL) {
}

void DigitalPinState::setOutput(uint8_t output) {
    if (m_mode == PIN_MODE_OUTPUT)
        digitalWrite(m_index, output);
    else if (m_mode == PIN_MODE_PWM)
        analogWrite(m_index, output);
    else if (m_mode == PIN_MODE_PWM_SMOOTH) {
        m_next_report = 0;
        m_target_pwm_value = output;
    }
}

uint8_t DigitalPinState::readInput() {
    return digitalRead(m_index);
}

AnalogPinState::AnalogPinState(uint8_t index, uint8_t mode) : PinState(A0 + index, mode, PIN_TYPE_ANALOG) {
    m_index_in_middleware = index;
}

void AnalogPinState::setOutput(uint8_t output) {
    digitalWrite(m_index, output);
}

uint8_t AnalogPinState::readInput() {
    int val = analogRead(m_index);
    // resolution bye bye! from 1023 to 255
    return val / 4;
}

void reset_board() {
    /*for (uint8_t i = 0; i < num_digital_pins; i++) {
        if (digital_pins[i]) {
            delete digital_pins[i];
            digital_pins[i] = NULL;
        }
    }
    for (uint8_t i = 0; i < num_analog_pins; i++) {
        if (analog_pins[i]) {
            delete analog_pins[i];
            analog_pins[i] = NULL;
        }
    }
    for (uint8_t i = 0; i < num_virtual_pins; i++) {
        if (virtual_pins[i]) {
            delete virtual_pins[i];
            virtual_pins[i] = NULL;
        }
    }

    ISREngine::reset();*/
}

void init_pin_states(uint8_t num_digital, uint8_t num_analog, uint8_t num_virtual) {
    num_digital_pins = num_digital;
    num_analog_pins = num_analog;
    num_virtual_pins = num_virtual;
}

uint8_t pin_states_on_command(uint8_t msg_type, uint8_t msg_len, char* command_buffer) {
    if (msg_type == COMMAND_RESET_BOARD) {
        LOG_INFO("command: COMMAND_RESET_BOARD");
        reset_board();
        return 0;
    }

    if (msg_len < 2) {
        LOG_ERROR("command too short");
        return 1;
    }

    uint8_t pin_type = command_buffer[0];
    uint8_t pin_index = command_buffer[1];

    LOG_INFO3("pin: ", pin_type == PIN_TYPE_DIGITAL ? "digital" : (pin_type == PIN_TYPE_ANALOG ? "analog" : (pin_type == PIN_TYPE_VIRTUAL ? "virtual" : "unknown")), pin_index);

    PinState* pin;
    if (pin_type == PIN_TYPE_DIGITAL && pin_index < num_digital_pins) {
        pin = digital_pins[pin_index];
    } else if (pin_type == PIN_TYPE_ANALOG && pin_index < num_analog_pins) {
        pin = analog_pins[pin_index];
    } else if (pin_type == PIN_TYPE_VIRTUAL && pin_index < num_virtual_pins) {
        pin = virtual_pins[pin_index];
    } else {
        LOG_ERROR("invalid pin type");
        return 2;
    }

    switch(msg_type) {
        case COMMAND_SET_PIN_MODE: {
            LOG_INFO("command: COMMAND_SET_PIN_MODE");

            if (msg_len != 3) {
                LOG_ERROR("invalid length");
                return 3;
            }

            if (!pin) {
                if (pin_type == PIN_TYPE_DIGITAL) {
                    digital_pins[pin_index] = new DigitalPinState(pin_index, command_buffer[2]);
                } else if (pin_type == PIN_TYPE_ANALOG) {
                    analog_pins[pin_index] = new AnalogPinState(pin_index, command_buffer[2]);
                } else {
                    LOG_ERROR("invalid pin type for this command");
                    return 4;
                }
            } else {
                pin->setMode(command_buffer[2]);
            }

            break;
        }
        case COMMAND_SET_VIRTUAL_PIN_MODE: {
            LOG_INFO("command: COMMAND_SET_VIRTUAL_PIN_MODE");
            #ifdef __SHAMMAM_SIMULATION__
                printf("SetVirtualPinMode() (msg_len=%d)\n", msg_len);
            #endif

            if (msg_len < 3) {
                LOG_ERROR("command too short");
                return 3;
            }

            if (pin) {
                LOG_INFO("deleting old pin");
                delete pin;
            }

            #ifdef __SHAMMAM_SIMULATION__
                printf("SetVirtualPinMode(%d, %d, [...]) (msg_len=%d)\n", command_buffer[2], msg_len-3, msg_len);
            #endif

            pin = virtual_pins[pin_index] = create_virtual_pin(command_buffer[2], msg_len-3, &command_buffer[3]);
            if (!pin) {
                LOG_ERROR("FAILED to create virtual pin");
                return 4;
            }

            break;
        }
        case COMMAND_SET_PIN_OUTPUT: {
            LOG_INFO("command: COMMAND_SET_PIN_OUTPUT");

            if (msg_len != 3) {
                LOG_ERROR("invalid command length");
                return 3;
            }
            if (!pin) {
                LOG_ERROR("invalid pin");
                return 4;
            }

            pin->setOutput(command_buffer[2]);
            break;
        }
        case COMMAND_READ_PIN_INPUT: {
            LOG_INFO("command: COMMAND_READ_PIN_INPUT");

            if (msg_len != 2) {
                LOG_ERROR("invalid command length");
                return 3;
            }
            if (!pin) {
                LOG_ERROR("invalid pin");
                return 4;
            }

            pin->markForReading();
            break;
        }
        case COMMAND_REGISTER_PIN_LISTENER: {
            LOG_INFO("command: COMMAND_REGISTER_PIN_LISTENER");

            if (msg_len != 6) {
                LOG_ERROR("invalid command length");
                return 3;
            }
            if (!pin) {
                LOG_ERROR("invalid pin");
                return 4;
            }

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
            LOG_ERROR("command: UNKNOWN!");
            return 5;
    }

    return 0;
}

void pin_states_update(unsigned long cur_time) {
    if (communication_is_synced()) {
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

void pin_states_time_rollover() {
    // time rolled-over, reset all m_next_reports

    for (int i = 0; i < num_digital_pins; i++)
        if (digital_pins[i])
            digital_pins[i]->m_next_report = 0;
    for (int i = 0; i < num_analog_pins; i++)
        if (analog_pins[i])
            analog_pins[i]->m_next_report = 0;
    for (int i = 0; i < num_virtual_pins; i++)
        if (virtual_pins[i])
            virtual_pins[i]->m_next_report = 0;
}
