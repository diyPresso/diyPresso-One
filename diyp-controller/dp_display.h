/*
 * display.h
 * 4x20 character LCD
 */
#ifndef DISPLAY_H
#define DISPLAY_H

//#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

#ifndef WIRECLOCK
#define WIRECLOCK 400000L
#endif

extern const unsigned char custom_chars_spinner[];
extern void format_float(char *dest, double f, int digits=0, int len=0);

extern hd44780_I2Cexp lcd; // switched to hd44780 library as it is way faster, espcially with 400kHz I2C clock

class Display
{
    private:
    public:
        Display(void);
        void init();
        void logo(const char *date, const char *time);
        void show(const char *screen, char *args[]);
        bool button_pressed();
        bool button_long_pressed();
        int button_pressed_time();
        bool encoder_changed();
        long encoder_value();
        void custom_chars(const unsigned char *chars);
};

extern Display display;


#endif // DISPLAY_H