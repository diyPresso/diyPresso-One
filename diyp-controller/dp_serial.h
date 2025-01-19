#ifndef DPSERIAL_H
#define DPSERIAL_H

#include <Arduino.h>
#include "dp.h"
#include "dp_hardware.h"
#include "dp_brew.h"
#include "dp_boiler.h"
#include "dp_reservoir.h"
#include "dp_settings.h"


class DpSerial {
    public:
        DpSerial(unsigned long baudRate);
        void begin();
        void send(const String &data);
        void send(double data);
        void send(int data);
        void send(char data);
        void send(const char *data);
        void receive();
        void send_info();
        void send_settings();

    private:
        unsigned long _baudRate;
        void put_settings(String value);
};

extern DpSerial dpSerial;

#endif // DPSERIAL_H