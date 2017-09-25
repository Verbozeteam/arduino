
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

int on_command(int msg_type, int msg_len, char* command_buffer) {
    return 1;
}

void setup() {
    serial_init();
}

void loop() {
    unsigned long cur_time = millis();

    serial_update(cur_time);
}


