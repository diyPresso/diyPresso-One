#ifndef DPSERIAL_H
#define DPSERIAL_H

#include <Arduino.h>
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
        void print_state();
        void send_info();
        void send_settings();

    private:
        unsigned long _baudRate;
        void put_settings(String value);
        void get_serial_output_config();
        void put_serial_output_config(String value);
        void get_boiler();
        void put_boiler(String value);
        void get_boiler_pid_autotune();
        void put_boiler_pid_autotune(String value);
};

extern DpSerial dpSerial;

#endif // DPSERIAL_H