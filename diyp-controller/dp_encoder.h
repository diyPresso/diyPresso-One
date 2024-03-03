// LED.h
#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Encoder
{
    private:
      int _encoder_count = 0;
      int _button_count = 0;
    public:
        Encoder(int pin_a, int pin_b, int pin_s);
        volatile int position();
        volatile void set(int value);
        volatile int loop_count();
        volatile int button_count();
        volatile bool button_state();
        volatile int button_time();
        volatile void reset();
        void start();
};

extern Encoder encoder;

#endif // ENCODER_H