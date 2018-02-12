#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>

#include <RPC.h>
#include <VPinState.h>
#include <VBuffer.h>
#include <VZigbee.h>
#include "DallasTemperature.h"

// just to make the libs work...
#define _NUM_DIGITAL_PINS 12
#define _NUM_ANALOG_PINS 8
#define _NUM_VIRTUAL_PINS 8
#define ONE_WIRE_PIN 13

DigitalPinState* digital_pins[_NUM_DIGITAL_PINS] = { NULL };
AnalogPinState* analog_pins[_NUM_ANALOG_PINS] = { NULL };
PinState* virtual_pins[_NUM_VIRTUAL_PINS] = { NULL };
// ----------


float DallasTemperature::getTempCByIndex(int i) {
    return 0; // doesn't need a lock
}

/**
 * This extends ArduinoServiceBasicImpl
 */
class ZigbeeService : public ArduinoServiceBasicImpl {
};

/**
 * This function must be defined when using a custom protocol to tell shammam about the new RPC imeplementation
 */
ArduinoServiceBasicImpl* __get_rpc_service() {
    return new ZigbeeService();
}

const int DELIMITER = 0x7E;
const int ESCAPE_CHARACTER = 0x7D;
const int UNESCAPE_VALUE = 0x20;

VBuffer zigbee_buffer;
SerialClass ArduinoSerial(8990, true);

void on_message_from_arduino (int16_t addr, char* msg, uint8_t len) {
    printf("To [master]: [");
    for (int i = 0; i < len; i++)
        printf(" 0x%X", msg[i] & 0xFF);
    printf(" ]\n");

    char* api_msg = new char[len+5];
    api_msg[0] = 0x81;
    api_msg[1] = (addr >> 8) & 0xFF;
    api_msg[2] = addr & 0xFF;
    api_msg[3] = 0;
    api_msg[4] = 0;
    memcpy(api_msg+5, msg, len);
    zigbeeAPICall(api_msg, len+5);
    delete[] api_msg;
}

void on_received_message(char* msg, int len) {
    char cmd = msg[0];

    switch (cmd) {
        case 0x1: { // Tx'd
            if (len < 3) {
                printf("Command 0x1: Bad length\n");
                break;
            }

            int frame = msg[1];
            int msb = msg[2];
            int lsb = msg[3];
            int addr = ((msb & 0xFF) << 8) | (lsb & 0xFF);

            printf("To [0x%X]: [", addr);
            for (int i = 4; i < len; i++)
                printf(" 0x%X", msg[i] & 0xFF);
            printf(" ]\n");

            if (len > 4 && addr == 0x2)
                ArduinoSerial.write(&msg[4], len - 4);

            break;
        }
        default:
            printf("wtf command 0x%X\n", cmd);
    }
}

int check_checksum(char* msg, int len, char checksum) {
    uint8_t s = 0;
    for (int i = 0; i < len; i++)
        s = s + msg[i];
    s = s + checksum;
    return (uint8_t)(s & 0xFF) == (uint8_t)0xFF;
}

void setup() {
    zigbeeInit(&Serial, NULL, 0x1, 0, 0);
    ArduinoSerial.begin(9600);
}

void loop() {
    while (Serial.available()) {
        zigbee_buffer.append(Serial.read());
    }

    while (zigbee_buffer.size() > 1) {
        int cur_pos = 0;
        if (zigbee_buffer.at(cur_pos++) != DELIMITER)
            zigbee_buffer.consume();
        else {
            if (cur_pos >= zigbee_buffer.size())
                break;
            int len_msb = zigbee_buffer.at(cur_pos++);
            if (cur_pos >= zigbee_buffer.size())
                break;
            if (len_msb == ESCAPE_CHARACTER)
                len_msb = zigbee_buffer.at(cur_pos++) ^ UNESCAPE_VALUE;
            if (cur_pos >= zigbee_buffer.size())
                break;

            int len_lsb = zigbee_buffer.at(cur_pos++);
            if (cur_pos >= zigbee_buffer.size())
                break;
            if (len_lsb == ESCAPE_CHARACTER)
                len_lsb = zigbee_buffer.at(cur_pos++) ^ UNESCAPE_VALUE;
            if (cur_pos >= zigbee_buffer.size())
                break;

            int bytes_till_content = cur_pos;
            int content_len = ((len_msb & 0xFF) << 8) | (len_lsb & 0xFF);
            if (zigbee_buffer.size() < bytes_till_content + content_len + 1) // +1 for checksum. This is just a lower bound...
                break;

            // count escape characters found
            char* content_and_checksum = new char[content_len + 1];
            int i = 0;
            for (i = 0; i < content_len + 1; i++) { // +1 for checksum
                if (cur_pos >= zigbee_buffer.size())
                    break;
                content_and_checksum[i] = zigbee_buffer.at(cur_pos++);
                if (content_and_checksum[i] == ESCAPE_CHARACTER) {
                    if (cur_pos >= zigbee_buffer.size())
                        break;
                    content_and_checksum[i] = zigbee_buffer.at(cur_pos++) ^ UNESCAPE_VALUE;
                }
            }
            if (i != content_len + 1) {
                delete[] content_and_checksum;
                break;
            }

            if (!check_checksum(content_and_checksum, content_len, content_and_checksum[content_len])) {
                printf("Checksum failed! [");
                for (int i = 0; i < cur_pos; i++)
                    printf(" 0x%X", zigbee_buffer.at(i) & 0xFF);
                printf(" ]\n");
            } else
                on_received_message(content_and_checksum, content_len);

            delete[] content_and_checksum;
            zigbee_buffer.consume(cur_pos);
        }
    }

    ArduinoSerial.__update__();
    int len = ArduinoSerial.available();
    if (len) {
        char* msg = new char[len];
        for (int i = 0; i < len; i++)
            msg[i] = ArduinoSerial.read();
        on_message_from_arduino(0x2, msg, len);
        delete[] msg;
    }
}

