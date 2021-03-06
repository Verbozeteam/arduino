#include "VVirtualPins.h"
#include "VCommunication.h"

PinState* create_virtual_pin(uint8_t virtual_pin_index, uint8_t type, uint8_t data_len, char* data) {
    switch (type) {
        case VIRTUAL_PIN_DIGITAL_TEMP_SENSOR: {
            if (data_len != 1)
                return NULL;
            return new DigitalTemperaturePinState(virtual_pin_index, data[0] & 0xFF);
        }
        case VIRTUAL_PIN_ISR_LIGHT: {
            if (data_len != 3)
                return NULL;
            return new ISRLightsPinState(data[0] & 0xFF, data[1] & 0xFF, data[2] & 0xFF);
        }
        case VIRTUAL_PIN_ISR_LIGHT2: {
            if (data_len != 3)
                return NULL;
            return new ISRLights2PinState(data[0] & 0xFF, data[1] & 0xFF, data[2] & 0xFF);
        }
        case VIRTUAL_PIN_MULTIPLEXED: {
            if (data_len != 1)
                return NULL;
            return new MultiplexedPin(data[0] & 0xFF);
        }
        case VIRTUAL_PIN_NTC_TEMP_SENSOR: {
            if (data_len != 2)
                return NULL;
            return new NTCTemperaturePinState(virtual_pin_index, data[0] & 0xFF, data[1] & 0xFF);
        }
        case VIRTUAL_PIN_CURRENT_SENSOR: {
            if (data_len != 1)
                return NULL;
            return new CurrentSensorPinState(virtual_pin_index, data[0] & 0xFF);
        }
    }

    return NULL;
}

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


DigitalTemperaturePinState::DigitalTemperaturePinState(uint8_t virtual_pin_index, uint8_t temp_index)
        : PinState(temp_index, PIN_MODE_INPUT, PIN_TYPE_VIRTUAL) {
    m_index_in_middleware = virtual_pin_index;
}

void DigitalTemperaturePinState::setOutput(uint8_t output) {
}

uint8_t DigitalTemperaturePinState::readInput() {
    float m_cur_temp = TemperatureEngine::m_sensors.getTempCByIndex(m_index);
    return (uint8_t)(m_cur_temp*4.0f);
}

/*
 * NTC sensor
 */

NTCTemperaturePinState::NTCTemperaturePinState(uint8_t virtual_pin_index, uint8_t index, uint8_t k_resistance)
        : PinState(index + A0, PIN_MODE_INPUT, PIN_TYPE_VIRTUAL) {
    m_index_in_middleware = virtual_pin_index;
    m_R1 = 1000.0f * (float)k_resistance;
}

void NTCTemperaturePinState::setOutput(uint8_t output) {
}

uint8_t NTCTemperaturePinState::readInput() {
    const float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
    int Vo = analogRead(m_index);
    float R2 = m_R1 * (1023.0 / (float)Vo - 1.0);
    float logR2 = log(R2);
    float T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
    float Tc = T - 273.15;
    return (int)((float)(Tc * 4.0f));
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

ISRLightsPinState::ISRLightsPinState(uint8_t frequency, uint8_t sync_port, uint8_t out_port) {
    m_my_index = 0;
    m_target_pwm_value = 105; // this is the 0, not 0
    m_next_report = 0;

    for (int i = 0; i < MAX_ISR_LIGHTS; i++) {
        if (ISREngine::m_light_ports[i] == -1) {
            m_my_index = i;
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

void ISRLightsPinState::setOutput(uint8_t output) {
    m_target_pwm_value = (int) (float)output * 1.1f;
}

void ISRLightsPinState::update(unsigned long cur_time) {
    if (cur_time >= m_next_report && m_target_pwm_value != ISREngine::m_light_intensities[m_my_index] && ISREngine::m_light_intensities[m_my_index] != -1) {
        m_next_report = cur_time + 10;
        // if (ISREngine::m_light_intensities[m_my_index] > 90)
        //     m_next_report += 15; // slower dimming at dim intensities

        if (m_target_pwm_value > ISREngine::m_light_intensities[m_my_index]) {
            ISREngine::m_light_intensities[m_my_index]++;
        } else {
            ISREngine::m_light_intensities[m_my_index]--;
        }
    }
}

uint8_t ISRLightsPinState::readInput() {
    return ISREngine::m_light_intensities[m_my_index];
}

ISRLights2PinState::ISRLights2PinState(uint8_t frequency, uint8_t sync_port, uint8_t out_port)
    : ISRLightsPinState(frequency, sync_port, out_port) {
    ISREngine::m_light_only_wave[m_my_index] = false;
}
