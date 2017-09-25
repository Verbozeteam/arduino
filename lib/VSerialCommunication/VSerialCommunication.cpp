#include <VSerialCommunication.h>
#include <VBuffer.h>

#define MAX_COMMAND_SIZE 128

extern int on_command(int msg_type, int msg_len, char* command_buffer);

char SYNC_SEQUENCE[8] = {(char)254, (char)6, (char)252, (char)11, (char)76, (char)250, (char)250, (char)255};
const int SYNC_SEQUENCE_LEN = sizeof(SYNC_SEQUENCE) / sizeof(char);
char command_buffer[MAX_COMMAND_SIZE];

unsigned long sync_send_timer = 0;
unsigned long sync_send_period = 1000; // 1 second
int wait_for_sync = 1;

HardwareSerial* SerialRef = NULL;

inline void serial_init(HardwareSerial* serial) {
    SerialRef = serial;
    SerialRef->begin(9600);
}

inline void serial_update(unsigned long cur_time) {
    int num_bytes = SerialRef->available();
    while (num_bytes > 0 && !read_buffer_full()) {
        read_buffer_append(SerialRef->read());
        num_bytes--;
    }

    if (cur_time >= sync_send_timer) {
        sync_send_timer = cur_time;
        sync_send_timer += sync_send_period;
        for (int i = 0; i < 8; i++)
            SerialRef->write(SYNC_SEQUENCE[i]);
    }

    if (wait_for_sync) {
        sync_send_period = 1000;
        // look for a sync sequence
        int rb_size = read_buffer_size();
        if (rb_size >= SYNC_SEQUENCE_LEN) {
            int found_sync = 0;
            int sync_start;
            for (sync_start = 0; sync_start + SYNC_SEQUENCE_LEN < rb_size; sync_start++) {
                int is_sync_sequence_start = 1;
                for (int i = 0; i < SYNC_SEQUENCE_LEN; i++) {
                    if (read_buffer_at(sync_start+i) != SYNC_SEQUENCE[i]) {
                        is_sync_sequence_start = 0;
                        break;
                    }
                }
                if (is_sync_sequence_start) {
                    found_sync = 1;
                    break;
                }
            }

            // for(sync_start = read_buffer_start; (sync_start + SYNC_SEQUENCE_LEN) % BUFFER_SIZE != read_buffer_end; sync_start = (sync_start + 1) % BUFFER_SIZE) {
            //     int is_sync_start = 1;
            //     for (int i = 0; i < SYNC_SEQUENCE_LEN; i++) {
            //         if (read_buffer[sync_start+i] != SYNC_SEQUENCE[i]) {
            //             is_sync_start = 0;
            //             break;
            //         }
            //     }
            //     if (is_sync_start) {
            //         found_sync = 1;
            //         break;
            //     }
            // }
            if (found_sync) {
                wait_for_sync = 0;
                sync_send_period = 10000;
                read_buffer_consume(sync_start+SYNC_SEQUENCE_LEN);
                //read_buffer_start = (sync_start + SYNC_SEQUENCE_LEN) % BUFFER_SIZE;
            } else {
                int truncate_size = read_buffer_size() - SYNC_SEQUENCE_LEN;
                read_buffer_consume(truncate_size);
                //read_buffer_start = (read_buffer_start + truncate_size) % BUFFER_SIZE;
            }
        }
    }

    int rb_size = read_buffer_size();
    while (!wait_for_sync && rb_size > 2) {
        char msg_type = read_buffer_at(0);
        char msg_len = read_buffer_at(1);
        if (rb_size >= 2 + msg_len) {
            read_buffer_consume(2);
            for (int i = 0; i < msg_len; i++)
                command_buffer[i] = read_buffer_consume();

            // first check for sync sequence
            if (msg_type == SYNC_SEQUENCE[0]) {
                int is_valid = msg_len == SYNC_SEQUENCE[1] ? 1 : 0;
                for (int i = 2; i < SYNC_SEQUENCE_LEN && is_valid; i++)
                    if (command_buffer[i-2] != SYNC_SEQUENCE[i])
                        is_valid = 0;
                if (!is_valid)
                    wait_for_sync = 1;
            } else {
                if (!on_command(msg_type, msg_len, &command_buffer[0]))
                    wait_for_sync = 1;
            }
        } else
            break;
        rb_size = read_buffer_size();
    }
}

inline void send_serial_command(int type, int len, char* cmd) {
    SerialRef->write(type);
    SerialRef->write(len);
    for (int i = 0; i < len; i++)
        SerialRef->write(cmd[i]);
}

