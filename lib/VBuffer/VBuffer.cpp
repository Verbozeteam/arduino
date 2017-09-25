
#define BUFFER_SIZE 4096

char read_buffer[BUFFER_SIZE];
int read_buffer_start = 0;
int read_buffer_end = 0;

inline void read_buffer_append(char b) {
    read_buffer[read_buffer_end] = b;
    read_buffer_end = (read_buffer_end + 1) % BUFFER_SIZE;
}

inline char read_buffer_consume(int len) {
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

inline char read_buffer_at(int index) {
    return read_buffer[(read_buffer_start + index) % BUFFER_SIZE];
}
