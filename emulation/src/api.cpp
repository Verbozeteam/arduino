#include <RPC.h>
#include "DallasTemperature.h" // A header file that mimics the actual DallasTemperature.h library header
#ifndef _COMPILE_NO_LIBS_
#include <VVirtualPins.h>
#endif

float g_temperature = 0.0f; // global "current sensor reading"

/**
 * So any time the Arduino code calls getTempCByIndex() it will get the value g_temperature...
 */
float DallasTemperature::getTempCByIndex(int i) {
    return g_temperature; // doesn't need a lock
}

/**
 * This extends ArduinoServiceBasicImpl (which implements SetPinState, GetPinState and ResetPins)
 * by defining a new RPC for setting the temperature
 */
class CustomArduinoService : public ArduinoServiceBasicImpl {
    Status SetTemperatureSensor(
        ServerContext* context,
        const shammam::Temperature* request,
        shammam::Empty* reply) {

        // g_temperature is set in the RPC call!
        g_temperature = request->temp(); // doesn't need a lock
        return Status::OK;
    }

#ifndef _COMPILE_NO_LIBS_
    Status GetISRState(
        ServerContext* context,
        const shammam::Empty* request,
        shammam::ISRState* reply) {

        reply->set_sync(ISREngine::m_sync_port);
        reply->set_full_period(ISREngine::m_sync_full_period);
        reply->set_wavelength(ISREngine::m_sync_wavelength);
        return Status::OK;
    }

    Status GetISRPinState(
        ServerContext* context,
        const shammam::ISRPin* request,
        shammam::ISRPinState* reply) {

        int pin = request->index();
        for (int i = 0; i < MAX_ISR_LIGHTS; i++) {
            if (ISREngine::m_light_ports[i] == -1)
                break;
            if (ISREngine::m_light_ports[i] == pin) {
                reply->set_port(ISREngine::m_light_ports[i]);
                reply->set_state(ISREngine::m_light_intensities[i]);
                break;
            }
        }

        return Status::OK;
    }
#endif
};

/**
 * This function must be defined when using a custom protocol to tell shammam about the new RPC imeplementation
 */
ArduinoServiceBasicImpl* __get_rpc_service() {
    return new CustomArduinoService();
}
