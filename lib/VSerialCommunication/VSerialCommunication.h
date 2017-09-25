#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

inline void serial_init(HardwareSerial* serial);

inline void serial_update(unsigned long cur_time);

inline void send_serial_command(int type, int len, char* cmd);
