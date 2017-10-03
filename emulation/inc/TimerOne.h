#pragma once

class TimerOne {
public:
    void initialize(int timer);
    void attachInterrupt(void (*f)(void));
};

extern TimerOne Timer1;
