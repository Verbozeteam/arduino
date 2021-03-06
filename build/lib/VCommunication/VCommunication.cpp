#include "VCommunication.h"
#include "VBuffer.h"
#include <Arduino.h>

#define MAX_COMMAND_SIZE 32

char FULL_SYNC_SEQUENCE[8] = {(char)254, (char)6, (char)253, (char)11, (char)76, (char)250, (char)250, (char)255};
char SYNC_SEQUENCE[8] = {(char)254, (char)6, (char)252, (char)11, (char)76, (char)250, (char)250, (char)255};
const int SYNC_SEQUENCE_LEN = sizeof(SYNC_SEQUENCE) / sizeof(char);
char command_buffer[MAX_COMMAND_SIZE];

VBuffer g_read_buffer;
COMMAND_CALLBACK on_command_cb = NULL;
unsigned long sync_send_timer = 0;
unsigned long sync_send_period = 1000; // 1 second
int half_sync = 0;
int full_sync = 0;
int is_using_sync = 1;

HardwareSerial* SerialRef = NULL;
HardwareSerial* LoggingSerial = NULL;

void communication_time_rollover() {
    // time rolled-over, reset the sync send timer
    sync_send_timer = 0;
}

void communication_init(HardwareSerial* serial, COMMAND_CALLBACK on_command, int use_sync) {
    SerialRef = serial;
    SerialRef->begin(9600);
    on_command_cb = on_command;
    is_using_sync = use_sync;
    if (!is_using_sync)
        half_sync = full_sync = 1;
    else
        LOG_INFO("Looking for sync");
}

uint8_t communication_is_synced(int set_to) {
    if (!is_using_sync)
        return 1;

    if (set_to != -1) {
        half_sync = set_to;
        full_sync = set_to;
        if (set_to == 0) {
            sync_send_timer = 0;
            sync_send_period = 1000;
        } else {
            sync_send_period = 10000;
        }
    }
    return full_sync == 1;
}

void communication_update(unsigned long cur_time) {
    int num_bytes = SerialRef->available();
    while (num_bytes > 0 && !g_read_buffer.full()) {
        char c = SerialRef->read();
        g_read_buffer.append(c);
        num_bytes--;
    }

    if (g_read_buffer.full()) {
        LOG_WARNING("communication_update(): buffer is full");
    }

    if (cur_time >= sync_send_timer && is_using_sync) {
        sync_send_timer = cur_time;
        sync_send_timer += sync_send_period;
        if (full_sync) {
            LOG_INFO("sending a full sync sequence");
            #ifdef __SHAMMAM_SIMULATION__
                printf("SENDING FULL SYNC\n");
            #endif
            for (int i = 0; i < 8; i++)
                SerialRef->write(FULL_SYNC_SEQUENCE[i]);
        } else {
            LOG_INFO("sending a half sync sequence");
            #ifdef __SHAMMAM_SIMULATION__
                printf("SENDING HALF SYNC\n");
            #endif
            for (int i = 0; i < 8; i++)
                SerialRef->write(SYNC_SEQUENCE[i]);
        }
    }

    if (!full_sync) {
        // look for a sync sequence
        int rb_size = g_read_buffer.size();
        if (rb_size >= SYNC_SEQUENCE_LEN) {
            int found_sync = 0;
            int sync_start;
            for (sync_start = 0; sync_start + SYNC_SEQUENCE_LEN < rb_size; sync_start++) {
                int is_sync_sequence_start = 1;
                for (int i = 0; i < SYNC_SEQUENCE_LEN; i++) {
                    if (g_read_buffer.at(sync_start+i) != SYNC_SEQUENCE[i] && g_read_buffer.at(sync_start+i) != FULL_SYNC_SEQUENCE[i]) {
                        is_sync_sequence_start = 0;
                        break;
                    }
                }
                if (is_sync_sequence_start) {
                    found_sync = 1;
                    break;
                }
            }

            if (found_sync) {
                int found_full = g_read_buffer.at(sync_start+2) == FULL_SYNC_SEQUENCE[2];
                g_read_buffer.consume(sync_start+SYNC_SEQUENCE_LEN);
                if (found_full) {
                    LOG_INFO("found full sync sequence");
                    #ifdef __SHAMMAM_SIMULATION__
                        printf("Found full sync\n");
                    #endif
                    sync_send_timer = 0;
                    communication_is_synced(1);
                } else {
                    LOG_INFO("found half sync sequence");
                    #ifdef __SHAMMAM_SIMULATION__
                        printf("Found half sync\n");
                    #endif
                    half_sync = 1;
                }
            } else {
                rb_size = g_read_buffer.size();
                int truncate_size;
                for (truncate_size = 0; truncate_size < rb_size; truncate_size++) {
                    if (g_read_buffer.at(truncate_size) == SYNC_SEQUENCE[0])
                        break;
                    else
                        LOG_WARNING2("found bad byte: ", (int)g_read_buffer.at(truncate_size));
                }
                g_read_buffer.consume(truncate_size);
            }
        }
    }

    int rb_size = g_read_buffer.size();
    while (full_sync && rb_size > 2) {
        char msg_type = g_read_buffer.at(0);
        char msg_len = g_read_buffer.at(1);
        if (rb_size >= 2 + msg_len) {
            g_read_buffer.consume(2);
            LOG_INFO4_NONL("found on buffer: [", (int)msg_type, ", ", (int)msg_len);
            for (int i = 0; i < msg_len; i++) {
                command_buffer[i] = g_read_buffer.consume();
                LOG_INFO2_CONTINUTE(", ", (int)command_buffer[i]);
            }
            LOG_INFO_CONTINUE("]\r\n");
            #ifdef __SHAMMAM_SIMULATION__
                printf(">> [0x%X 0x%X", msg_type & 0xFF, msg_len & 0xFF);
                for (int x = 0; x < msg_len; x++)
                    printf(" 0x%X", command_buffer[x] & 0xFF);
                printf("]\n");
            #endif

            // first check for sync sequence
            if (msg_type == SYNC_SEQUENCE[0]) {
                int is_valid = msg_len == SYNC_SEQUENCE[1] ? 1 : 0;
                for (int i = 2; i < SYNC_SEQUENCE_LEN && is_valid; i++)
                    if (command_buffer[i-2] != FULL_SYNC_SEQUENCE[i])
                        is_valid = 0;
                if (!is_valid) {
                    #ifdef __SHAMMAM_SIMULATION__
                        printf("INVALID SYNC SEQUENCE\n");
                    #endif
                    communication_is_synced(0);
                    LOG_ERROR("BAD sync sequence!");
                } else {
                    LOG_INFO("found full sync sequence");
                }
            } else {
                if (on_command_cb(msg_type, msg_len, &command_buffer[0]) != 0) {
                    #ifdef __SHAMMAM_SIMULATION__
                        printf("INVALID COMMAND\n");
                    #endif
                    LOG_ERROR("INVALID COMMAND!");
                    communication_is_synced(0);
                }
            }
        } else {
            if (msg_len > MAX_COMMAND_SIZE) {
                // hmm? huge command!
                communication_is_synced(0);
            }
            break;
        }
        rb_size = g_read_buffer.size();
    }
}

void communication_send_command(uint8_t type, uint8_t len, char* cmd) {
    SerialRef->write(type);
    SerialRef->write(len);
    for (int i = 0; i < len; i++)
        SerialRef->write(cmd[i]);
}

void communication_init_logging(HardwareSerial* serial) {
#ifdef ENABLE_LOGGING
    LoggingSerial = serial;
    LoggingSerial->begin(9600);
#endif
}
