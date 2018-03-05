#pragma once

#include <Arduino.h>

#define BUFFER_SIZE 512

class VBuffer {
    char m_buffer[BUFFER_SIZE];
    int m_start;
    int m_end;

public:
    VBuffer() : m_start(0), m_end(0) {};

    void append(char b);

    char consume(int len = 1);

    int full();

    int empty();

    int size();

    char at(int index);
};
