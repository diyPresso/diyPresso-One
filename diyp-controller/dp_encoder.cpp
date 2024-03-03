/*
  Rotary Encoder driver
  (c) 2024 diyEspresso -- CC-BY-NC 
  Hacky implementation of debounced rotary encoder, working with a periodic timer.
  This should work on all GPIO pins (no interrupt capability required)
  Only one instance supported at the moment (due to one set of global variables and one interrupt handler)
 */
#include "dp_encoder.h"
#include "dp_hardware.h"
#include "uTimerLib.h"

Encoder encoder(PIN_ENC_A,  PIN_ENC_B, PIN_ENC_S);

volatile static int enc_value=0, enc_button=0, enc_button_count=0, enc_button_time=0;
volatile static int timer_count = 0;
static int _pin_a, _pin_b, _pin_s;


#define TIMER_PERIOD_US 400 // Timer period [microseconds] (400usec timer = 2.5kHz)
#define DEGLITCH 2 // Deglitch count value. The degitch time is: 2 * TIMER_PERIOD_US * DEGLITCH
#define BUTTON_DEGLITCH_BITS 0xFFFFFFF // Set bits that need to be high before we switch back. Deglitch period is: TIMER_PERIOD_US * HIGH_BITS

void encoder_timer_function()
{
    volatile static unsigned int val=0, cur=0, prev=0, enc_switch=0, enc_prev_button=0;
    volatile static int afilt=0, bfilt=0;
    timer_count = (timer_count+1) & 0xFFFF;


    // button de-glitching and handling
    enc_switch = (enc_switch<<1) | (digitalRead(_pin_s) ? 0 : 1);
    enc_button = (enc_switch & BUTTON_DEGLITCH_BITS) == BUTTON_DEGLITCH_BITS ? 1 : 0;

    if ( (enc_button) && (!enc_prev_button) ) // falling edge
      enc_button_count += 1;
    enc_prev_button = enc_button;
    if ( enc_button )
      enc_button_time += 1;
    else
      enc_button_time = 0;

    // encoder de-glitching and handling
    val = 0;
    val |=  digitalRead(_pin_a) ? 1 : 0;
    val |=  digitalRead(_pin_b) ? 2 : 0;

    if (val & 1)
      afilt += 1;
    else
      afilt -= 1;
    if (afilt > DEGLITCH) { afilt = DEGLITCH; cur |= 1; }
    if (afilt < -DEGLITCH) { afilt = -DEGLITCH; cur &= 2; }

    if (val & 2)
      bfilt += 1;
    else
      bfilt -= 1;
    if (bfilt > DEGLITCH) { bfilt = DEGLITCH; cur |= 2; }
    if (bfilt < -DEGLITCH) { bfilt = -DEGLITCH; cur &= 1; }

    if ( ((prev & 1) == 1) && ((cur & 1) == 0)  )
    {
      enc_value += ( cur & 2 ? -1 : 1 );
    }
    prev = cur;
}

Encoder::Encoder(int pin_a, int pin_b, int pin_s)
{
    _pin_a = pin_a; _pin_b = pin_b; _pin_s = pin_s;
    reset();
}

void Encoder::start(void)
{
    TimerLib.setInterval_us(encoder_timer_function,  TIMER_PERIOD_US );
}

volatile int Encoder::position()
{
    return enc_value;
}

volatile int Encoder::loop_count()
{
    return timer_count;
}

volatile int Encoder::button_count()
{
    return enc_button_count;
}

volatile bool Encoder::button_state()
{
    return enc_button;
}

/// @brief Return time the button is pressed
/// @return  time in msec, zero if raised, counting up while pressed
volatile int Encoder::button_time()
{
    return (enc_button_time*TIMER_PERIOD_US) / 1000;
}

volatile void Encoder::reset()
{
    _button_count = 0;
    _encoder_count = 0;
    timer_count = 0;
}

volatile void Encoder::set(int value)
{
  enc_value = value;
}

