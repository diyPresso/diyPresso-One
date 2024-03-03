/*
 * display.h
 * 4x20 character LCD
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <LiquidCrystal_I2C.h>

extern const unsigned char custom_chars_spinner[];
extern void format_float(char *dest, double f, int digits=0, int len=0);

extern LiquidCrystal_I2C lcd;

class Display
{
    private:
    public:
        Display(void);
        void init();
        void logo();
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