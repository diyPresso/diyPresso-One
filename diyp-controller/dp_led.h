// NOTE: On mkr1010 board Wifi RGB led cannot be used before init() or loop() (e.g. in constructor of classes): this hangs the board.
#ifndef LED_H
#define LED_H

#include <Arduino.h>

class ColorLed
{
    private:
    public:
        ColorLed(void);
        void color(const bool c[]);
        inline constexpr static const bool BLACK[3] = {false,false,false}; // RGB
        inline constexpr static const bool RED[3] =   {true,false,false}; 
        inline constexpr static const bool GREEN[3] = {false,true,false};
        inline constexpr static const bool BLUE[3] =  {false,false,true};
        inline constexpr static const bool WHITE[3] = {true,true,true};
        inline constexpr static const bool PURPLE[3] = {true,false,true};
        inline constexpr static const bool YELLOW[3] = {true,true,false};
        inline constexpr static const bool CYAN[3] = {false,true,true};
        

};

extern ColorLed statusLed;


#endif // LED_H