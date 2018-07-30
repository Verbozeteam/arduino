#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

typedef uint8_t (*COMMAND_CALLBACK) (uint8_t, uint8_t, char*);

void communication_init(HardwareSerial* serial, COMMAND_CALLBACK on_command, int use_sync = 1);

uint8_t communication_is_synced(int set_to = -1);

void communication_update(unsigned long cur_time);

void communication_send_command(uint8_t type, uint8_t len, char* cmd);

void communication_time_rollover();

extern HardwareSerial* LoggingSerial;

void communication_init_logging(HardwareSerial* serial);

//#define ENABLE_LOGGING
#ifdef ENABLE_LOGGING
#define LOG_INFO(x) {if (LoggingSerial) {LoggingSerial->print("[info] "); LoggingSerial->println(x);}}
#define LOG_INFO_CONTINUE(x) {if (LoggingSerial) {LoggingSerial->print(x);}}
#define LOG_INFO2(x1, x2) {if (LoggingSerial) {LoggingSerial->print("[info] "); LoggingSerial->print(x1); LoggingSerial->println(x2);}}
#define LOG_INFO3(x1, x2, x3) {if (LoggingSerial) {LoggingSerial->print("[info] "); LoggingSerial->print(x1); LoggingSerial->print(x2); LoggingSerial->println(x3);}}
#define LOG_INFO2_CONTINUTE(x1, x2) {if (LoggingSerial) {LoggingSerial->print(x1); LoggingSerial->print(x2);}}
#define LOG_INFO4_NONL(x1, x2, x3, x4) {if (LoggingSerial) {LoggingSerial->print("[info] "); LoggingSerial->print(x1); LoggingSerial->print(x2); LoggingSerial->print(x3); LoggingSerial->print(x4);}}
#define LOG_WARNING(x) {if (LoggingSerial) {LoggingSerial->print("[warning] "); LoggingSerial->println(x);}}
#define LOG_WARNING2(x1, x2) {if (LoggingSerial) {LoggingSerial->print("[warning] "); LoggingSerial->print(x1); LoggingSerial->println(x2);}}
#define LOG_ERROR(x) {if (LoggingSerial) {LoggingSerial->print("[error] "); LoggingSerial->println(x);}}
#else
#define LOG_INFO(x) {}
#define LOG_INFO_CONTINUE(x) {}
#define LOG_INFO2(x1, x2) {}
#define LOG_INFO3(x1, x2, x3) {}
#define LOG_INFO2_CONTINUTE(x1, x2) {}
#define LOG_INFO4_NONL(x1, x2, x3, x4) {}
#define LOG_WARNING(x) {}
#define LOG_WARNING2(x1, x2) {}
#define LOG_ERROR(x) {}
#endif
