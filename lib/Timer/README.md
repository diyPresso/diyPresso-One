# Arduino Timer Library v1.1.2

The **Arduino Timer Library** allows you to measure the time between started and stop command. The time can measured in milli or micro seconds. Micro seconds have only a resolution of 4µs!

## Installation

1. Downlaod from the Release site
2. Unzip
3. Move the folder to your Arduino Library folder 

## How to use

First, include the Timer library to your project:

```
#include "Timer.h"
```

Now, you can create a new object(s):

```
Timer timer;
// for micro second resolution:
Timer timer(MICROS);
timer.start(); // start the timer
timer.pause(); // pause the timer
timer.resume(); // resume the timer
timer.stop(); // stops the timer
timer.read(); // gives back the elapsed time in milli or micro seconds
```

## Example

Complete example. Here we created one timer, you can run it and get the result in the Serial monitor.

```
#include "Timer.h"

Timer timer();

void setup() {
  Serial.begin(9600);
  timer.start();
  if(timer.state() == RUNNING) Serial.println("timer running");
  delay(1000);
  timer.stop();
  if(timer.state() == STOPPED) Serial.println("timer stopped");
  Serial.print("time elapsed ms: ");
  Serial.println(timer.read());
  }

void loop() {
  return;
  }

```

## Documentation

### Constructors / Destructor

**Timer(resolution_t resolution = MILLIS)**<br>
Creates a Timer object
- parameter resolution sets the internal resolution of the Timer, it can MICROS, or MILLIS

**~Ticker()**<br>
Destructor for Ticker object
	
### Functions

**void start()**<br>
Start the Timer. If it is paused, it will restarted the Timer.

**void pause()**<br>
Pause the Timer.

**void resume()**<br>
Resume the Timer after a pause.

**void stop()**<br>
Stops the Timer.

**uint32_t read()**<br>
Returns the time after start, you can read the elapsed time also while running.

**status_t state()**<br>
Get the Timer state (RUNNING, PAUSED, STOPPED).
