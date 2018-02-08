#include "VBuffer.h"
#include "VZigbee.h"
#include "VCommunication.h"
#include "VPinState.h"
#include "HardwareSerial.h"

int16_t g_myAddress = 0x1;
int8_t  g_zChannel = 'C';
int16_t g_zPanID = 3332;

uint8_t on_serial_command(uint8_t msg_type, uint8_t msg_len, char* command_buffer) {
    int16_t target_address;

    if (msg_type == COMMAND_RESET_BOARD) {
        target_address = 0xFFFF;
        return 0;
    } else if (msg_len < 2)
        return 1;
    else {
        uint8_t pin_index = command_buffer[1];
        target_address = 0x2 + pin_index % 10;
        command_buffer[1] = (pin_index % 10) + 2;
    }


    char cmd[32];
    cmd[0] = msg_type;
    cmd[1] = msg_len;
    for (int i = 0; i < msg_len; i++)
        cmd[i+2] = command_buffer[i];
    zigbeeTx(target_address, cmd, msg_len + 2);
}

void on_zigbee_command(int16_t addr_from, char* command, uint8_t len) {
    Serial.write(command, len);
}

void setup() {
    communication_init(&Serial, on_serial_command);
    zigbeeInit(&Serial1, on_zigbee_command, g_myAddress, g_zChannel, g_zPanID);
}

void loop() {
    unsigned long cur_time = millis();

    communication_update(cur_time);
    zigbeeUpdate(cur_time);
}


