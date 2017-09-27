#pragma once

#include <HardwareSerial.h>

#ifndef byte
#define byte unsigned char
#endif

void serial_init(HardwareSerial* serial);

byte serial_is_synced();

void serial_update(unsigned long cur_time);

void send_serial_command(byte type, byte len, char* cmd);
