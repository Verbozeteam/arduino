#include "VZigbee.h"

HardwareSerial* ZigbeeSerialRef = NULL;
HardwareSerial* SerialSerialRef = NULL;

ZIGBEE_CALLBACK g_commandCallback = NULL;
int64_t g_slaves[MAX_NUM_SLAVES] = { 0 };
uint8_t g_numSlaves = 0;
uint8_t g_curFrameNumber = 0;

void addSlave(int64_t addr) {
    if (g_numSlaves < MAX_NUM_SLAVES) {
        for (int i = 0; i < MAX_NUM_SLAVES; i++) {
            if (g_slaves[i] == addr) // this slave already exists, don't add it
                return;
            if (g_slaves[i] > addr) { // this slave needs to be inserted here
                // shift all slaves
                int64_t tmp = addr;
                for (int j = i; j < MAX_NUM_SLAVES-1; j++) {
                    int64_t tmp2 = g_slaves[j];
                    g_slaves[j] = tmp;
                    tmp = tmp2;
                }
                break;
            } else if (g_slaves[i] == 0) { // this is the highest address slave, put it at the end
                g_slaves[i] = addr;
                break;
            }
        }
        g_numSlaves++;
    }
}

void zigbeeInit(HardwareSerial* serial, ZIGBEE_CALLBACK on_command, const char* panId) {
    ZigbeeSerialRef = serial;
    if (on_command) {
        ZigbeeSerialRef->begin(9600);
        g_commandCallback = on_command;
    }

    // initialize the zigbee chip
    delay(1100);
    ZigbeeSerialRef->write("+++");
    ZigbeeSerialRef->flush();
    delay(1100);
    ZigbeeSerialRef->write("ATCE 0\r"); // not a coordinator
    ZigbeeSerialRef->write("ATAP 0\r"); // transparent mode
    ZigbeeSerialRef->write("ATEE 0\r"); // disable security (@TODO: CHANGE)
    ZigbeeSerialRef->write("ATJV 0\r"); // ???
    ZigbeeSerialRef->write("ATDH 0\r"); // destination 0 is coordinator
    ZigbeeSerialRef->write("ATDL 0\r"); // destination 0 is coordinator

    ZigbeeSerialRef->write("ATID ");    // set panID
    ZigbeeSerialRef->write(panId);
    ZigbeeSerialRef->write("\r");
    ZigbeeSerialRef->write("ATCN\r");
}

void zigbeeDiscover() {
    #if ZIGBEE_VERSION == 2
        // initiate discovery
        zigbeeTx(0xFFFF, "D", 1);

        // wait for discovered slaves
        for (int i = 0; i < 4; i++) {
            delay(500);
            zigbeeUpdate(0);
        }
    #endif
}

uint8_t zigbeeChecksum(const char* data, uint8_t len) {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++)
        sum += data[i];
    return (uint8_t)((int)0xFF - (int)sum);
}

void zigbeeEscapeAndSend(uint8_t b) {
    if (b == 0x7E || b == 0x7D || b == 0x11 || b == 0x13) {
        ZigbeeSerialRef->write(0x7D);
        ZigbeeSerialRef->write(b ^ 0x20);
    } else
        ZigbeeSerialRef->write(b);
}

void zigbeeAPICall(const char* cmd, uint8_t len) {
    uint8_t msb = (len >> 8) & 0xFF;
    uint8_t lsb = (len     ) & 0xFF;
    uint8_t checksum = zigbeeChecksum(cmd, len);

    ZigbeeSerialRef->write(0x7E);
    zigbeeEscapeAndSend(msb);
    zigbeeEscapeAndSend(lsb);
    for (int i = 0; i < len; i++)
        zigbeeEscapeAndSend(cmd[i]);
    zigbeeEscapeAndSend(checksum);
}

void zigbeeTx(int16_t addrTo, const char* data, uint8_t len) {
    #if ZIGBEE_VERSION == 1
        // simply forward the message (to whatever zigbee destination is)
        for (int i = 0; i < len; i++)
            ZigbeeSerialRef->write(data[i]);
    #elif ZIGBEE_VERSION == 2
        // Create an API frame targeting addrTo and escape and stuff
        char buf[100];
        buf[0] = 0x01;
        buf[1] = g_curFrameNumber++;
        buf[2] = (addrTo >> 8) & 0xFF;
        buf[3] = (addrTo     ) & 0xFF;
        for (int i = 0; i < len; i++)
            buf[4+i] = data[i];

        if (g_curFrameNumber == 0)
            g_curFrameNumber = 1;

        zigbeeAPICall(buf, len + 4);
    #endif
}

void zigbeeUpdate(unsigned long curTime) {
    #if ZIGBEE_VERSION == 1
        // Forward zigbee stuff to the Serial
        char tmp[100];
        int index = 0;
        while (ZigbeeSerialRef->available())
            tmp[index++] = ZigbeeSerialRef->read();
        g_commandCallback(0xFF, tmp, index);
    #elif ZIGBEE_VERSION == 2
        // TODO
    #endif
}
