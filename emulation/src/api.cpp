#include <RPC.h>

class CustomArduinoService : public ArduinoServiceBasicImpl {
    Status SetTemperatureSensor(
        ServerContext* context,
        const shammam::Temperature* request,
        shammam::Empty* reply) {

        return Status::OK;
    }
};

ArduinoServiceBasicImpl* __get_rpc_service() {
    return new CustomArduinoService();
}
