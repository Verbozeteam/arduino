#pragma once

inline void serial_init();

inline void serial_update(unsigned long cur_time);

void send_serial_command(int type, int len, char* cmd);
