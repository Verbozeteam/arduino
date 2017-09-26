//#pragma once

void read_buffer_append(char b);

char read_buffer_consume(int len = 1);

int read_buffer_full();

int read_buffer_empty();

int read_buffer_size();

char read_buffer_at(int index);
