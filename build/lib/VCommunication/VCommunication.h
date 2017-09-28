#pragma once

#include <HardwareSerial.h>

#ifndef uchar
#define uchar unsigned char
#endif

void serial_init(HardwareSerial* serial);

uchar serial_is_synced(int set_to = -1);

void serial_update(unsigned long cur_time);

void send_serial_command(uchar type, uchar len, char* cmd);
