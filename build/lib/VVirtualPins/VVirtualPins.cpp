#include "VVirtualPins.h"
#include "VCommunication.h"

OneWire TemperatureEngine::m_one_wire(ONE_WIRE_PIN);
DallasTemperature TemperatureEngine::m_sensors;
uint8_t TemperatureEngine::m_num_sensors;
float TemperatureEngine::m_cur_temp;
unsigned long TemperatureEngine::m_next_read;

void TemperatureEngine::initialize(uint8_t one_wire_pin) {
    m_one_wire = OneWire(one_wire_pin);
    m_sensors = DallasTemperature(&m_one_wire);
    // m_sensors.begin(); <--- this causes issues with Serial port overflowing ???????
    discoverOneWireDevices();
    m_cur_temp = 25.0f;
    m_next_read = 0;
}

void TemperatureEngine::discoverOneWireDevices() {
    LOG_INFO("TemperatureEngine::discoverOneWireDevices()");
    m_num_sensors = 0;
    uint8_t addr[8];
    while (m_one_wire.search(addr)) {
        LOG_INFO("found temperature sensor on: ");
        m_num_sensors++;
    }
    m_one_wire.reset_search();
    LOG_INFO2("TemperatureEngine::discoverOneWireDevices() discovered devices: ", m_num_sensors);
}

void TemperatureEngine::update(unsigned long cur_time) {
    if (cur_time >= m_next_read) {
        m_next_read = cur_time + TEMPERATURE_READ_INTERVAL;
        m_sensors.requestTemperatures(); // Send the command to get temperatures
    }
}

void central_ac_setOutput_method(pin_state_t* pin, uint8_t output) {
}

uint8_t central_ac_readInput_method(pin_state_t* pin) {
    float m_cur_temp = TemperatureEngine::m_sensors.getTempCByIndex(pin->m_index);
    return (uint8_t)(m_cur_temp*4.0f);
}

void initialize_central_ac_pin(pin_state_t* pin, uint8_t temp_index) {
    initialize_pin(pin, temp_index, PIN_MODE_INPUT, PIN_TYPE_VIRTUAL);

    pin->setOutput = central_ac_setOutput_method;
    pin->readInput = central_ac_readInput_method;
}

/**
 * ISR Light
 */

int ISREngine::m_light_ports[MAX_ISR_LIGHTS];
int ISREngine::m_light_intensities[MAX_ISR_LIGHTS];
int ISREngine::m_light_intensities_copies[MAX_ISR_LIGHTS];
bool ISREngine::m_light_only_wave[MAX_ISR_LIGHTS];
int ISREngine::m_sync_port = -1;
int ISREngine::m_sync_full_period = -1;
int ISREngine::m_sync_wavelength = -1;
int ISREngine::m_clock_tick = 0;

void ISREngine::timer_interrupt() {
    for (int i = 0; i < MAX_ISR_LIGHTS; i++) {
        int port = m_light_ports[i];
        int intensity = m_light_intensities_copies[i];
        if (m_light_only_wave[i]) {
            if (port != -1 && intensity == m_clock_tick) {
                digitalWrite(port, HIGH);
                delayMicroseconds(m_sync_wavelength);
                digitalWrite(port, LOW);
            }
        } else {
            if (port != -1 && m_clock_tick >= intensity) {
                m_light_intensities_copies[i] = -1;
                digitalWrite(port, HIGH);
            }
        }
    }

    m_clock_tick++;
}

void ISREngine::zero_cross_interrupt() {
    m_clock_tick = 0;

    for (int i = 0; i < MAX_ISR_LIGHTS; i++) {
        int port = m_light_ports[i];
        m_light_intensities_copies[i] = m_light_intensities[i];
        if (!m_light_only_wave[i] && port != -1)
            digitalWrite(port, LOW);
    }
}

void ISREngine::initialize(int frequency, int sync_port) {
    if (m_sync_port != -1) {
        detachInterrupt(m_sync_port);
        Timer1.detachInterrupt();
    }

    if (frequency == 60) {
        m_sync_full_period = 83;
        m_sync_wavelength = 8;
    } else {
        m_sync_full_period = 100;
        m_sync_wavelength = 10;
    }

    m_sync_port = sync_port;
    m_clock_tick = 0;

    attachInterrupt(m_sync_port, zero_cross_interrupt, RISING);
    Timer1.initialize(m_sync_full_period);
    Timer1.attachInterrupt(timer_interrupt);
}

void ISREngine::reset() {
    for (int i = 0; i < MAX_ISR_LIGHTS; i++) {
        m_light_ports[i] = -1;
        m_light_intensities[i] = -1;
    }
}

#define ISRDATA(pin) ((isr_light_custom_data_t*)pin->_m_custom_data)
struct isr_light_custom_data_t {
    uint8_t m_my_index;
};

void isr_light_setOutput_method(pin_state_t* pin, uint8_t output) {
    pin->m_target_pwm_value = (int) (float)output * 1.1f;
}

void isr_light_update_method(pin_state_t* pin, unsigned long cur_time) {
    int my_index = ISRDATA(pin)->m_my_index;

    if (cur_time >= pin->m_next_report && pin->m_target_pwm_value != ISREngine::m_light_intensities[my_index] && ISREngine::m_light_intensities[my_index] != -1) {
        pin->m_next_report = cur_time + 10;
        // if (ISREngine::m_light_intensities[m_my_index] > 90)
        //     pin->m_next_report += 15; // slower dimming at dim intensities

        if (pin->m_target_pwm_value > ISREngine::m_light_intensities[my_index]) {
            ISREngine::m_light_intensities[my_index]++;
        } else {
            ISREngine::m_light_intensities[my_index]--;
        }
    }
}

uint8_t isr_light_readInput_method(pin_state_t* pin) {
    return ISREngine::m_light_intensities[ISRDATA(pin)->m_my_index];
}

void initialize_isr_light_pin(pin_state_t* pin, uint8_t frequency, uint8_t sync_port, uint8_t out_port) {
    initialize_pin(pin);

    ISRDATA(pin)->m_my_index = 0;
    pin->m_target_pwm_value = 105; // this is the 0, not 0
    pin->m_next_report = 0;

    pin->setOutput = isr_light_setOutput_method;
    pin->readInput = isr_light_readInput_method;
    pin->update = isr_light_update_method;

    for (int i = 0; i < MAX_ISR_LIGHTS; i++) {
        if (ISREngine::m_light_ports[i] == -1) {
            ISRDATA(pin)->m_my_index = i;
            ISREngine::m_light_intensities[i] = 105;
            ISREngine::m_light_intensities_copies[i] = 105;
            ISREngine::m_light_ports[i] = out_port;
            ISREngine::m_light_only_wave[i] = true;
            break;
        }
    }
    pinMode(out_port, OUTPUT);
    ISREngine::initialize(frequency, sync_port);
}

void initialize_isr_light_2_pin(pin_state_t* pin, uint8_t frequency, uint8_t sync_port, uint8_t out_port) {
    initialize_isr_light_pin(pin, frequency, sync_port, out_port);
    ISREngine::m_light_only_wave[ISRDATA(pin)->m_my_index] = false;
}


void initialize_virtual_pin(pin_state_t** pin_out, uint8_t type, uint8_t data_len, char* data) {
    switch (type) {
        case VIRTUAL_PIN_CENTRAL_AC: {
            if (data_len != 1)
                *pin_out = NULL;
            else
                initialize_central_ac_pin(*pin_out, data[0] & 0xFF);
            break;
        }
        case VIRTUAL_PIN_ISR_LIGHT: {
            if (data_len != 3)
                *pin_out = NULL;
            else
                initialize_isr_light_pin(*pin_out, data[0] & 0xFF, data[1] & 0xFF, data[2] & 0xFF);
            break;
        }
        case VIRTUAL_PIN_ISR_LIGHT2: {
            if (data_len != 3)
                *pin_out = NULL;
            else
                initialize_isr_light_2_pin(*pin_out, data[0] & 0xFF, data[1] & 0xFF, data[2] & 0xFF);
            break;
        }
        default:
            *pin_out = NULL;
    }
}


