#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

#define MAX_NUM_SLAVES 40
#define ZIGBEE_VERSION 2

typedef void (*ZIGBEE_CALLBACK) (int16_t, char*, uint8_t);


void zigbeeAPICall(const char* cmd, uint8_t len);

void zigbeeInit(HardwareSerial* serial, ZIGBEE_CALLBACK on_command, const char* panId);
void zigbeeDiscover();
void zigbeeTx(int16_t addrTo, const char* data, uint8_t len);
void zigbeeUpdate(unsigned long curTime);
