#include "VBuffer.h"

void VBuffer::append(char b) {
    m_buffer[m_end] = b;
    m_end = (m_end + 1) % BUFFER_SIZE;
}

char VBuffer::consume(int len) {
    char c = m_buffer[m_start];
    m_start = (m_start + len) % BUFFER_SIZE;
    return c;
}

int VBuffer::full() {
    return (m_end+1) % BUFFER_SIZE == m_start;
}

int VBuffer::empty() {
    return m_start == m_end;
}

int VBuffer::size() {
    return m_end >= m_start ? m_end - m_start : BUFFER_SIZE - (m_start-m_end);
}

char VBuffer::at(int index) {
    return m_buffer[(m_start + index) % BUFFER_SIZE];
}
