#pragma once

#include <HardwareSerial.h>

#ifndef uchar
#define uchar unsigned char
#endif

void communication_init(HardwareSerial* serial);

uchar communication_is_synced(int set_to = -1);

void communication_update(unsigned long cur_time);

void communication_send_command(uchar type, uchar len, char* cmd);
