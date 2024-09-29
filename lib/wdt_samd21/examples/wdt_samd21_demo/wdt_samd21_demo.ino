#include <wdt_samd21.h>

void setup() {
   delay ( 500 );
   //
   Serial.begin ( 9600 );
   while ( !Serial ) {
      delay ( 100 );
   }
   //
   // Initialze WDT with a 2 sec. timeout
   wdt_init ( WDT_CONFIG_PER_2K );
}

void loop() {
   for ( byte i = 0; i < 5; i++ ) {
      // wait a second
      delay ( 1000 );
      // write on the serial port
      Serial.print ( "Iteration " );
      Serial.print ( i + 1 );
      Serial.println ( " of 5" );
      // "feed" the WDT to avoid restart
      wdt_reset();
   }
   //
   // now disable wdt and wait ...
   wdt_disable();
   Serial.println( "wdt disabled ..." );
   Serial.println ( "Now waiting for 3 seconds ..." );
   delay(3000);
   //
   // ... then reEnable the wdt ...
   wdt_reEnable();
   Serial.println( "wdt reEnabled ..." );
   //
   // ... and wait 4 seconds ... the WDT should restart the board
   Serial.println ( "Now waiting for 4 seconds ..." );
   delay ( 4000 );
   //
   Serial.println ( "*** You will never see this message printed ***" );
   delay ( 1000 );
}
