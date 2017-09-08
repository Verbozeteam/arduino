#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

#define BUFFER_SIZE 8192
#define MAX_COMMAND_SIZE 128

char SYNC_SEQUENCE[8] = {254, 6, 252, 11, 76, 250, 250, 255};
const int SYNC_SEQUENCE_LEN = sizeof(SYNC_SEQUENCE) / sizeof(char);

unsigned long sync_send_timer = 0;
unsigned long sync_send_period = 1000; // 1 second
int wait_for_sync = 1;

char read_buffer[BUFFER_SIZE];
char command_buffer[MAX_COMMAND_SIZE];
int read_buffer_start = 0;
int read_buffer_end = 0;

inline void read_buffer_append(char b) {
    read_buffer[read_buffer_end] = b
    read_buffer_end = (read_buffer_end + 1) % BUFFER_SIZE;
}

inline char read_buffer_consume(int len=1) {
    char c = read_buffer[read_buffer_start];
    read_buffer_start = (read_buffer_start + len) % BUFFER_SIZE;
    return c;
}

inline int read_buffer_full() {
    return (read_buffer_end+1) % BUFFER_SIZE == read_buffer_start;
}

inline int read_buffer_empty() {
    return read_buffer_start == read_buffer_end;
}

inline int read_buffer_size() {
    return read_buffer_end >= read_buffer_start ? read_buffer_end - read_buffer_start : BUFFER_SIZE - (read_buffer_start-read_buffer_end);
}

int on_command(int msg_type, int len, char* command) {
    return 1;
}

void setup() {
    Serial.begin(9600);
}

void loop() {
    int num_bytes = Serial.available();
    while (num_bytes > 0 && !read_buffer_full()) {
        read_buffer_append(Serial.read());
        num_bytes--;
    }

    unsigned long cur_time = millis();
    if (cur_time >= sync_send_timer && !wait_for_sync) {
        sync_send_timer = cur_time;
        sync_send_timer += sync_send_period;
        Serial.write((char*)&SYNC_SEQUENCE[0], 8);
    }

    if (wait_for_sync) {
        sync_send_period = 1000;
        // look for a sync sequence
        if (read_buffer_size() > SYNC_SEQUENCE_LEN) {
            int found_sync = 0;
            for(int sync_start = read_buffer_start; (sync_start + SYNC_SEQUENCE_LEN) % BUFFER_SIZE != read_buffer_end; sync_start = (sync_start + 1) % BUFFER_SIZE) {
                int is_sync_start = 1;
                for (int i = 0; i < SYNC_SEQUENCE_LEN; i++) {
                    if (read_buffer[sync_start+i] != SYNC_SEQUENCE[i]) {
                        is_sync_start = 0;
                        break;
                    }
                }
                if (is_sync_start) {
                    found_sync = 1;
                    break;
                }
            }
            if (found_sync) {
                wait_for_sync = 0;
                sync_send_period = 10000;
                read_buffer_start = (sync_start + SYNC_SEQUENCE_LEN) % BUFFER_SIZE;
            } else {
                int truncate_size = read_buffer_size() - SYNC_SEQUENCE_LEN;
                read_buffer_start = (read_buffer_start + truncate_size) % BUFFER_SIZE;
            }
        }
    }

    int buffer_size = read_buffer_size();
    while (!wait_for_sync && buffer_size > 2) {
        char msg_type = buffer[read_buffer_start];
        char msg_len = buffer[(read_buffer_start+1)%BUFFER_SIZE];
        if (buffer_size >= 2 + msg_len) {
            read_buffer_consume(2);
            for (int i = 0; i < msg_len; i++) {
                command_buffer[i] = read_buffer_consume();
            }

            // first check for sync sequence
            if (msg_type == SYNC_SEQUENCE[0]) {
                int is_valid = msg_len == SYNC_SEQUENCE[1] ? 1 : 0;
                for (int i = 2; i < SYNC_SEQUENCE_LEN && is_valid; i++)
                    if (command_buffer[i-2] != SYNC_SEQUENCE[i])
                        is_valid = 0;
                if (!is_valid)
                    wait_for_sync = 1;
            } else {
                if (!on_command(msg_type, msg_len, command_buffer))
                    wait_for_sync = 1;
            }
        }
        buffer_size = read_buffer_size();
    }
}