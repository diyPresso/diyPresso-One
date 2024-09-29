# ArduPID
[![GitHub version](https://badge.fury.io/gh/PowerBroker2%2FArduPID.svg)](https://badge.fury.io/gh/PowerBroker2%2FArduPID) [![arduino-library-badge](https://www.ardu-badge.com/badge/ArduPID.svg?)](https://www.ardu-badge.com/ArduPID)<br /><br />
PID library for Arduinos with greater accuracy than the legacy Arduino PID library.

# Why Use PID?
PID stands for Proportional, Integral, and Derivative and is a simple type of controller. PID controllers can provide stable control of nearly any system such as the speed of a motor, position of a servo, speed of a car (cruise control), and much more.

# How Does it Work?
See the explanation video [here](https://www.youtube.com/watch?v=OqvrYNJvtaU)

# How to Tune a PID:
See [here](https://pidexplained.com/how-to-tune-a-pid-controller/)

# How to Use the Library:
First import the library, instantiate an ArduPID class, and create 6 doubles:
- Setpoint
- Input
- Output
- P Gain
- I Gain
- D Gain

```C++
#include "ArduPID.h"

ArduPID myController;

double setpoint = 512;
double input;
double output;
double p = 1;
double i = 0;
double d = 0;
```

Next, within the `setup()` function, initialize the controller with the references to the input, output, and setpoint variables. Also pass the values of the PID gains. After initializing the controller within `setup()`, you can change the settings/configuration of the controller.

```C++
void setup()
{
  Serial.begin(115200);
  myController.begin(&input, &output, &setpoint, p, i, d);
  
  // ADD MORE CONFIGURATION COMMANDS HERE <--------------------
}
```

Here are the different examples of configuration commands available via the library:

- `reverse()`
  - Reverse direction of output
- `setSampleTime(const unsigned int& _minSamplePeriodMs)`
  - Will ensure at least `_minSamplePeriodMs` have past between successful compute() calls - *this function is not necessary, but is included for convenience*
- `setOutputLimits(const double& min, const double& max)`
  - Clip output to values to `min` and `max`
- `setBias(const double& _bias)`
  - Output will have a constant offset of `_bias`, usually used in conjunction with `setOutputLimits()`
- `setWindUpLimits(const double& min, const double& max)`
  - Clip integral term values to `min` and `max`to prevent [integral wind-up](https://www.youtube.com/watch?v=H4YlL3rZaNw)
- `start()`
  - Enable/turns on the controller
- `reset()`
  - Used for resetting the I and D terms - *only use this if you know what you're doing*
- `stop()`
  - Disable/turn off the PID controller (`compute()` will not do anything until `start()` is called)

Within the `loop()`, update the input variable via your sensor and then call `compute()`. The output variable will then be automatically updated and you can use that updated value to send an updated command to your actuator. You can also use the `debug()` command to print various status information about your controller to the serial plottor/monitor:

```C++
void loop()
{
  input = analogRead(A0); // Replace with sensor feedback

  myController.compute();
  myController.debug(&Serial, "myController", PRINT_INPUT    | // Can include or comment out any of these terms to print
                                              PRINT_OUTPUT   | // in the Serial plotter
                                              PRINT_SETPOINT |
                                              PRINT_BIAS     |
                                              PRINT_P        |
                                              PRINT_I        |
                                              PRINT_D);
  
  analogWrite(3, output); // Replace with plant control signal
}
```
