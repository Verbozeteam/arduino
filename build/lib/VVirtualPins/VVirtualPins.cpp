#include <VVirtualPins.h>

PinState* create_virtual_pin(byte type, byte data_len, char* data) {
    switch (type) {
        case 0: {
            if (data_len != 1)
                return NULL;
            return new CentralACPinState(data[0] & 0xFF);
        }
        case 1: {
            return NULL;
        }
    }

    return NULL;
}


OneWire TemperatureEngine::m_one_wire(0);
DallasTemperature TemperatureEngine::m_sensors;
byte TemperatureEngine::m_num_sensors;
float TemperatureEngine::m_cur_temp;
unsigned long TemperatureEngine::m_next_read;

void TemperatureEngine::initialize(byte one_wire_pin) {
    m_one_wire = OneWire(one_wire_pin);
    m_sensors = DallasTemperature(&m_one_wire);
    m_sensors.begin();
    discoverOneWireDevices();
    m_cur_temp = 25.0f;
    m_next_read = 0;
}

void TemperatureEngine::discoverOneWireDevices() {
    m_num_sensors = 0;
    byte addr[8];
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


CentralACPinState::CentralACPinState(byte temp_index)
    : PinState(temp_index, PIN_MODE_INPUT, PIN_TYPE_VIRTUAL)
{
}

void CentralACPinState::setOutput(byte output) {
}

byte CentralACPinState::readInput() {
    float m_cur_temp = TemperatureEngine::m_sensors.getTempCByIndex(m_index);
    return (byte)(m_cur_temp*2.0f);
}
