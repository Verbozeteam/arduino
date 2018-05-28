#include "VPinState.h"
#include "VVirtualPins.h"
#include "VCommunication.h"
#include "Arduino.h"

extern pin_state_t digital_pins[];
extern pin_state_t analog_pins[];
extern pin_state_t virtual_pins[];

uint8_t num_digital_pins = 0;
uint8_t num_analog_pins = 0;
uint8_t num_virtual_pins = 0;

void pin_state_update_method(pin_state_t* pin, unsigned long cur_time) {
    if (pin->m_mode == PIN_MODE_INPUT || pin->m_mode == PIN_MODE_INPUT_PULLUP) {
        if (pin->m_read_interval == 0) {
            uint8_t reading = pin->readInput(pin);
            if (reading != pin->m_last_reading_sent || pin->m_is_first_send) {
                pin->m_is_first_send = 0;
                pin->m_last_reading_sent = reading;
                char cmd[3] = {(char)pin->m_type, (char)pin->m_index_in_middleware, (char)reading};
                communication_send_command(COMMAND_PIN_READING, 3, cmd);
            }
        } else if (cur_time >= pin->m_next_report) {
            if (pin->m_read_interval == -1)
                pin->m_next_report = -1;
            else
                pin->m_next_report = cur_time + pin->m_read_interval;
            uint8_t reading = pin->readInput(pin);
            char cmd[3] = {(char)pin->m_type, (char)pin->m_index_in_middleware, (char)reading};
            communication_send_command(COMMAND_PIN_READING, 3, cmd);
        }
    } else if (pin->m_mode == PIN_MODE_PWM_SMOOTH) {
        if (cur_time >= pin->m_next_report && pin->m_target_pwm_value != pin->m_last_reading_sent) {
            pin->m_next_report = cur_time + 10;
            if (pin->m_target_pwm_value > pin->m_last_reading_sent)
                pin->m_last_reading_sent++;
            else
                pin->m_last_reading_sent--;
            analogWrite(pin->m_index, pin->m_last_reading_sent);
        }
    }
}

void pin_state_setMode_method(pin_state_t* pin, uint8_t mode) {
    #ifdef __SHAMMAM_SIMULATION__
        printf("PIN MODE [%d] -> %d\n", pin->m_index, mode);
    #endif
    pin->m_mode = mode;
    if (pin->m_mode == PIN_MODE_INPUT)
        pinMode(pin->m_index, INPUT);
    else if (pin->m_mode == PIN_MODE_OUTPUT || pin->m_mode == PIN_MODE_PWM || pin->m_mode == PIN_MODE_PWM_SMOOTH) {
        pinMode(pin->m_index, OUTPUT);
        if (pin->m_mode == PIN_MODE_PWM_SMOOTH) {
            pin->m_last_reading_sent = 0;
            pin->m_next_report = 0;
            analogWrite(pin->m_index, 0);
        }
    } else if (pin->m_mode == PIN_MODE_INPUT_PULLUP)
        pinMode(pin->m_index, INPUT_PULLUP);
}

void pin_state_markForReading_method(pin_state_t* pin) {
    pin->m_next_report = 0; // next cycle must read this pin immediately
}

void pin_state_setReadingInterval_method(pin_state_t* pin, unsigned long interval) {
    pin->m_read_interval = interval;
    pin->m_next_report = 0;
}

void initialize_pin(pin_state_t* pin, uint8_t index, uint8_t mode, uint8_t type) {
    pin->m_index = index;
    pin->m_index_in_middleware = index;
    pin->m_mode = mode;
    pin->m_type = type;

    pin->m_next_report = 0;
    pin->m_read_interval = -1;
    pin->m_last_reading_sent = 3;
    pin->m_is_first_send = 1;
    pin->m_target_pwm_value = 0;

    pin->update = pin_state_update_method;
    pin->setMode = pin_state_setMode_method;
    pin->markForReading = pin_state_markForReading_method;
    pin->setReadingInterval = pin_state_setReadingInterval_method;
    pin->setOutput = NULL;
    pin->readInput = NULL;

    if (type != PIN_TYPE_VIRTUAL)
        pin->setMode(pin, mode);
}

void digital_pin_state_setOutput_method(pin_state_t* pin, uint8_t output) {
    if (pin->m_mode == PIN_MODE_OUTPUT)
        digitalWrite(pin->m_index, output);
    else if (pin->m_mode == PIN_MODE_PWM)
        analogWrite(pin->m_index, output);
    else if (pin->m_mode == PIN_MODE_PWM_SMOOTH) {
        pin->m_next_report = 0;
        pin->m_target_pwm_value = output;
    }
}

uint8_t digital_pin_state_readInput_method(pin_state_t* pin) {
    return digitalRead(pin->m_index);
}

void initialize_digital_pin(pin_state_t* pin, uint8_t index, uint8_t mode) {
    initialize_pin(pin, index, mode, PIN_TYPE_DIGITAL);
    pin->setOutput = digital_pin_state_setOutput_method;
    pin->readInput = digital_pin_state_readInput_method;
}

void analog_pin_state_setOutput_method(pin_state_t* pin, uint8_t output) {
    digitalWrite(pin->m_index, output);
}

uint8_t analog_pin_state_readInput_method(pin_state_t* pin) {
    int val = analogRead(pin->m_index);
    // resolution bye bye! from 1023 to 255
    return val / 4;
}

void initialize_analog_pin(pin_state_t* pin, uint8_t index, uint8_t mode) {
    initialize_pin(pin, A0 + index, mode, PIN_TYPE_ANALOG);

    pin->m_index_in_middleware = index;

    pin->setOutput = analog_pin_state_setOutput_method;
    pin->readInput = analog_pin_state_readInput_method;
}

void zero_mem(char* mem, int size) {
    for (int i = 0; i < size; i++)
        mem[i] = 0;
}

void reset_board() {
    for (uint8_t i = 0; i < num_digital_pins; i++) {
        zero_mem((char*)&digital_pins[i], sizeof(pin_state_t));
    }
    for (uint8_t i = 0; i < num_analog_pins; i++) {
        zero_mem((char*)&analog_pins[i], sizeof(pin_state_t));
    }
    for (uint8_t i = 0; i < num_virtual_pins; i++) {
        zero_mem((char*)&virtual_pins[i], sizeof(pin_state_t));
    }

    ISREngine::reset();
}

void init_pin_states(uint8_t num_digital, uint8_t num_analog, uint8_t num_virtual) {
    num_digital_pins = num_digital;
    num_analog_pins = num_analog;
    num_virtual_pins = num_virtual;

    reset_board();
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

    pin_state_t* pin;
    if (pin_type == PIN_TYPE_DIGITAL && pin_index < num_digital_pins) {
        pin = &digital_pins[pin_index];
    } else if (pin_type == PIN_TYPE_ANALOG && pin_index < num_analog_pins) {
        pin = &analog_pins[pin_index];
    } else if (pin_type == PIN_TYPE_VIRTUAL && pin_index < num_virtual_pins) {
        pin = &virtual_pins[pin_index];
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
                    initialize_digital_pin(pin, pin_index, command_buffer[2]);
                } else if (pin_type == PIN_TYPE_ANALOG) {
                    initialize_analog_pin(pin, pin_index, command_buffer[2]);
                } else {
                    LOG_ERROR("invalid pin type for this command");
                    return 4;
                }
            } else {
                pin->setMode(pin, command_buffer[2]);
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

            #ifdef __SHAMMAM_SIMULATION__
                printf("SetVirtualPinMode(%d, %d, [...]) (msg_len=%d)\n", command_buffer[2], msg_len-3, msg_len);
            #endif

            initialize_virtual_pin(&pin, command_buffer[2], msg_len-3, &command_buffer[3]);
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
            if (!pin->setOutput) {
                LOG_ERROR("invalid pin");
                return 4;
            }

            pin->setOutput(pin, command_buffer[2]);
            break;
        }
        case COMMAND_READ_PIN_INPUT: {
            LOG_INFO("command: COMMAND_READ_PIN_INPUT");

            if (msg_len != 2) {
                LOG_ERROR("invalid command length");
                return 3;
            }
            if (!pin->markForReading) {
                LOG_ERROR("invalid pin");
                return 4;
            }

            pin->markForReading(pin);
            break;
        }
        case COMMAND_REGISTER_PIN_LISTENER: {
            LOG_INFO("command: COMMAND_REGISTER_PIN_LISTENER");

            if (msg_len != 6) {
                LOG_ERROR("invalid command length");
                return 3;
            }
            if (!pin->setReadingInterval) {
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

            pin->setReadingInterval(pin, byte1);
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
            if (digital_pins[i].update)
                digital_pins[i].update(&digital_pins[i], cur_time);
        for (int i = 0; i < num_analog_pins; i++)
            if (analog_pins[i].update)
                analog_pins[i].update(&digital_pins[i], cur_time);
        for (int i = 0; i < num_virtual_pins; i++)
            if (virtual_pins[i].update)
                virtual_pins[i].update(&digital_pins[i], cur_time);
    }
}

