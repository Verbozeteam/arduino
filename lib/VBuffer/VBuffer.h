#pragma once

inline void read_buffer_append(char b);

inline char read_buffer_consume(int len = 1);

inline int read_buffer_full();

inline int read_buffer_empty();

inline int read_buffer_size();

inline char read_buffer_at(int index);
