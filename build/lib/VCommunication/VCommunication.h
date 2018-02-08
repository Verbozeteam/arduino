#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

typedef uint8_t (*COMMAND_CALLBACK) (uint8_t, uint8_t, char*);

void communication_init(HardwareSerial* serial, COMMAND_CALLBACK on_command, int use_sync = 1);

uint8_t communication_is_synced(int set_to = -1);

void communication_update(unsigned long cur_time);

void communication_send_command(uint8_t type, uint8_t len, char* cmd);
