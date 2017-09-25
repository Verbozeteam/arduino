#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

void serial_init(HardwareSerial* serial);

void serial_update(unsigned long cur_time);

void send_serial_command(int type, int len, char* cmd);
