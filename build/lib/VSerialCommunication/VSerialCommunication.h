//#pragma once

#include <HardwareSerial.h>

void serial_init(HardwareSerial* serial);

int serial_is_synced();

void serial_update(unsigned long cur_time);

void send_serial_command(int type, int len, char* cmd);
