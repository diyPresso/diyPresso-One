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
        int position();
        int switch_count();
        int loop_count();
        bool button_state();
        void reset();
        void start();
        void set(int value);
};

extern Encoder encoder;

#endif // ENCODER_H