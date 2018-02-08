#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

#define MAX_NUM_SLAVES 40
#define ZIGBEE_VERSION 1

typedef void (*ZIGBEE_CALLBACK) (int16_t, char*, uint8_t);

void zigbeeInit(HardwareSerial* serial, ZIGBEE_CALLBACK on_command, int16_t myAddr, int8_t channel, int16_t panId);
void zigbeeDiscover();
void zigbeeTx(int16_t addrTo, const char* data, uint8_t len);
void zigbeeUpdate(unsigned long curTime);
