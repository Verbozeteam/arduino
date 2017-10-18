#include <VVirtualPins.h>

PinState* create_virtual_pin(uchar type, uchar data_len, char* data) {
    switch (type) {
        case VIRTUAL_PIN_CENTRAL_AC: {
            if (data_len != 1)
                return NULL;
            return new CentralACPinState(data[0] & 0xFF);
        }
        case VIRTUAL_PIN_ISR_LIGHT: {
            if (data_len != 3)
                return NULL;
            return new ISRLightsPinState(data[0] & 0xFF, data[1] & 0xFF, data[2] & 0xFF);
        }
    }

    return NULL;
}

OneWire TemperatureEngine::m_one_wire(0);
DallasTemperature TemperatureEngine::m_sensors;
uchar TemperatureEngine::m_num_sensors;
float TemperatureEngine::m_cur_temp;
unsigned long TemperatureEngine::m_next_read;

void TemperatureEngine::initialize(uchar one_wire_pin) {
    m_one_wire = OneWire(one_wire_pin);
    m_sensors = DallasTemperature(&m_one_wire);
    m_sensors.begin();
    discoverOneWireDevices();
    m_cur_temp = 25.0f;
    m_next_read = 0;
}

void TemperatureEngine::discoverOneWireDevices() {
    m_num_sensors = 0;
    uchar addr[8];
    while (m_one_wire.search(addr))
        m_num_sensors++;
    m_one_wire.reset_search();
}

void TemperatureEngine::update(unsigned long cur_time) {
    if (cur_time >= m_next_read) {
        m_next_read = cur_time + TEMPERATURE_READ_INTERVAL;
        m_sensors.requestTemperatures(); // Send the command to get temperatures
    }
}


CentralACPinState::CentralACPinState(uchar temp_index)
    : PinState(temp_index, PIN_MODE_INPUT, PIN_TYPE_VIRTUAL)
{
}

void CentralACPinState::setOutput(uchar output) {
}

uchar CentralACPinState::readInput() {
    float m_cur_temp = TemperatureEngine::m_sensors.getTempCByIndex(m_index);
    return (uchar)(m_cur_temp*4.0f);
}

/**
 * ISR Light
 */

int ISREngine::m_light_ports[MAX_ISR_LIGHTS];
int ISREngine::m_light_intensities[MAX_ISR_LIGHTS];
int ISREngine::m_sync_port = -1;
int ISREngine::m_sync_full_period = -1;
int ISREngine::m_sync_wavelength = -1;
int ISREngine::m_clock_tick = 0;

void ISREngine::timer_interrupt() {
    for (int i = 0; i < MAX_ISR_LIGHTS; i++) {
        int port = m_light_ports[i];
        int intensity = m_light_intensities[i];
        if (port != -1 && intensity == m_clock_tick) {
            digitalWrite(port, HIGH);
            delayMicroseconds(m_sync_wavelength);
            digitalWrite(port, LOW);
        }
    }
    m_clock_tick++;
}

void ISREngine::zero_cross_interrupt() {
    m_clock_tick=0;
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

ISRLightsPinState::ISRLightsPinState(uchar frequency, uchar sync_port, uchar out_port) {
    m_my_index = 0;
    for (int i = 0; i < MAX_ISR_LIGHTS; i++) {
        if (ISREngine::m_light_ports[i] == -1) {
            m_my_index = i;
            ISREngine::m_light_intensities[i] = 0;
            ISREngine::m_light_ports[i] = out_port;
            break;
        }
    }
    pinMode(out_port, OUTPUT);
    ISREngine::initialize(frequency, sync_port);
}

void ISRLightsPinState::setOutput(uchar output) {
    ISREngine::m_light_intensities[m_my_index] = output;
}

uchar ISRLightsPinState::readInput() {
    return ISREngine::m_light_intensities[m_my_index];
}
