#include <VVirtualPins.h>

PinState* create_virtual_pin(int type, int data_len, char* data) {
    switch (type) {
        case 0: {
            if (data_len != 1)
                return NULL;
            return new CentralACPinState(data[0]);
        }
        case 1: {
            return NULL;
        }
    }

    return NULL;
}

OneWire TemperatureEngine::m_one_wire;
DallasTemperature TemperatureEngine::m_sensors;
int TemperatureEngine::m_num_sensors;
float TemperatureEngine::m_cur_temp;
unsigned long TemperatureEngine::m_next_read;
const float TemperatureEngine::fHomeostasis = 0.49f;

void TemperatureEngine::initialize(int one_wire_pin) {
    m_one_wire = OneWire(one_wire_pin);
    m_sensors = DallasTemperature(&m_one_wire);
    m_sensors.begin();
    discoverOneWireDevices();
    m_cur_temp = 25.0f;
    m_next_read = 0;
}

void TemperatureEngine::discoverOneWireDevices() {
    m_num_sensors = 0;
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


CentralACPinState::CentralACPinState(int temp_index)
    : PinState(temp_index, PIN_MODE_INPUT, PIN_TYPE_VIRTUAL),
      m_cur_temp(25.0f),
      m_target_temp(25.0f)
{
}

void CentralACPinState::update(unsigned long cur_time) {
    PinState::update(cur_time);

    float diff = m_cur_temp - m_target_temp;
    float coeff = (min(max(diff, -10), 10)) / 10; // [-1, 1]
    fAirflow = min(max(fAirflow + TemperatureEngine::fHomeostasis * coeff, 0), 255);
    analogWrite(, (int)fAirflow);
}

void CentralACPinState::setOutput(int output) {
    m_target_temp = ((float)output) / 2.0f;
}

int CentralACPinState::readInput() {
    m_cur_temp = TemperatureEngine::m_sensors.getTempCByIndex(m_index);
    return (int)(m_cur_temp*2.0f);
}

