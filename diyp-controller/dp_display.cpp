/*
  display
 */
#include "dp.h"
#include "dp_display.h"
#include "dp_hardware.h"
#include "dp_chars.h"
#include "dp_encoder.h"

#include <math.h>

hd44780_I2Cexp lcd(DISPLAY_I2C_ADDRESS,20,4); // address 0x27, 4 lines, 20 chars:
Display display;

Display::Display(void)
{ 
  return;
}


/*
 * show display content, expect 4x20 chars + zero terminator
 * Replace placeholders ('#' chars) with supplied string arguments
 * And display to lcd screen
 * Example screen:
 *
 * char *screen = 
 * "01234567890123456789"
 * "arg0:####  arg1:####"
 * "arg3: ##############"
 * "arg4: ## arg 5: ####"
 *
 */
void Display::show(const char *screen, char *args[])
{
  const char *s = screen;
  char buf[4*21], *d, *a, l[32];
  bool in_arg = false;
  int idx=0;
  d = buf;
  a = args[idx];

  while( *s )
  {
    if ( *s != '#' ) // copy source to dest
    {
      if ( in_arg )
      {
         in_arg = false;
         idx += 1;
         a = args[idx];
      }
      *d = *s;
    }
    else // args
    {
      in_arg = true;
      *d = *a ? *a : ' '; // on end of argument string: pad with spaces
      if ( *a ) a += 1; // If not end of string, advance one char
    }
    s += 1;
    d += 1;
  }
  *d = 0;

  for(int i=0; i<4; i++)
  {
    memcpy((void*)l, &buf[20*i], 20);
    l[20] = 0;
    lcd.setCursor(0,i);
    lcd.print(l); // [Done] used to be: slowwwww 27ms for 20 chars > switch to https://github.com/duinoWitchery/hd44780/tree/master
  }
}

void format_float(char *dest, double f, int digits, int len)
{
  bool neg = f < 0;
  if ( neg ) f *= -1.0;
  char sign[2] = {'-', 0};
  if ( !neg )
    sign[0] = 0;
  switch( digits )
  {
    case 1: sprintf(dest, "%s%d.%d", sign, (int)f, (int)fmod(10.0*f,10)); break;
    case 2: sprintf(dest, "%s%d.%d%d", sign, (int)f, (int)fmod(10.0*f,10),(int)fmod(100.0*f,10)); break;
    case 3: sprintf(dest, "%s%d.%d%d%d", sign, (int)f, (int)fmod(10.0*f,10),(int)fmod(100.0*f,10),(int)fmod(1000.0*f,10)); break;
    default: sprintf(dest, "%s%d", sign, (int)f); break;
  }
  if ( len ) // right align result to length
  {
    int org = strlen(dest)-1;
    dest[len] = 0;
    for(int i=len-1; i>=0; i--)
    {
      if (org >= 0)
        dest[i] = dest[org--];
      else
        dest[i] = ' ';
    }

  }
}

// Load custom characters
void Display::custom_chars(const unsigned char *chars)
{
  for(int i=0; i<8; i++)
    lcd.createChar(i, (unsigned char*)chars+8*i);
}


void Display::init()
{
  lcd.init();
  lcd.backlight();
  lcd.flush();
  lcd.createChar(1, (unsigned char*)cC1);
  lcd.createChar(2, (unsigned char*)cC2);
  lcd.createChar(3, (unsigned char*)cC3);
  lcd.createChar(4, (unsigned char*)cC4);
  lcd.createChar(5, (unsigned char*)cC5);
  lcd.createChar(6, (unsigned char*)cC6);
  lcd.createChar(7, (unsigned char*)cC7);
  
#ifdef WIRECLOCK
    Wire.setClock(WIRECLOCK);
#endif

}


bool Display::button_pressed()
{
  static unsigned long prev_count=0;
  bool pressed = prev_count != encoder.button_count();
  prev_count = encoder.button_count();
  return pressed;
}

bool Display::button_long_pressed()
{
  static unsigned long prev_count=0;
  bool pressed = prev_count != encoder.button_count();
  if ( pressed && (encoder.button_time() > 1000) )
  {
    prev_count = encoder.button_count();
    return true;
  }
  return false;
}

int Display::button_pressed_time()
{
    return encoder.button_time();
}

bool Display::encoder_changed()
{
  static long prev_value=0;
  bool changed = prev_value != encoder.position();
  prev_value = encoder.position();
  return changed;
}

long Display::encoder_value()
{
  return encoder.position();
}


void Display::logo(const char *date, const char*time)
{
  int textDelay = 1;
  lcd.setCursor(8,0);
  lcd.write (1);
  delay(textDelay);
  lcd.print (" ");
  delay(textDelay);
  lcd.write (1);
  delay(textDelay);
  lcd.write (2);
  delay(textDelay);
  lcd.write (2);
  delay(textDelay);
  lcd.write (2);
  delay(textDelay);
  lcd.write (1);
  delay(textDelay);

  lcd.setCursor(8,1);
  lcd.write (1);
  delay(textDelay);
  lcd.print (" ");
  delay(textDelay);
  lcd.write (1);
  delay(textDelay);
  lcd.write (3);
  delay(textDelay);
  lcd.write (3);
  delay(textDelay);
  lcd.write (6);
  delay(textDelay);
  lcd.write (7);

  lcd.setCursor(4,2);
  delay(textDelay);
  lcd.write (4);
  delay(textDelay);
  lcd.write (5);
  delay(textDelay);
  lcd.write (2);
  delay(textDelay);
  lcd.write (2);
  delay(textDelay);
  lcd.write (1);
  delay(textDelay);
  lcd.print (" ");
  delay(textDelay);
  lcd.write (1);
  delay(textDelay);

  lcd.setCursor(4,3);
  lcd.write (1);
  delay(textDelay);
  lcd.write (3);
  delay(textDelay);
  lcd.write (3);
  delay(textDelay);
  lcd.write (3);
  delay(textDelay);
  lcd.write (1);
  delay(textDelay);
  lcd.print (" ");
  delay(textDelay);
  lcd.write (1);
  delay(textDelay);

  delay(1000);
  lcd.setCursor(0,3);
  lcd.print("r" HARDWARE_REVISION);
  lcd.setCursor(20-strlen("v" SOFTWARE_VERSION), 3);
  lcd.print("v" SOFTWARE_VERSION);

  lcd.setCursor(4,1);
  lcd.print(date);
  lcd.setCursor(6,2);
  lcd.print(time);

#if defined(SIMULATE) || !defined(WATCHDOG_ENABLED)
  lcd.setCursor(2,0);
  lcd.print("!!! TESTING !!!");
#endif


  delay(3000);
}
