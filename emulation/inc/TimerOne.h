#pragma once

class TimerOne {
public:
    void initialize(int timer);
    void attachInterrupt(void (*f)(void));
    void detachInterrupt();
};

extern TimerOne Timer1;
