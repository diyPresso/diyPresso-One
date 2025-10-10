/* 
diyPresso PID controller
(c) 2025 diyPresso

Loosely based on the powerbroker2/ArduPID libary 

PID controller for the diyPresso machines. Supports static and dynamic feed forward. 

Dynamic feed forward (DFF) is calculated based on the reservoir weight. The water flowing out of
the reservoir will be needed to be heated to the set temperature, so the power needed to
heat this water is calculated and added to the output.

The calculation is as follows:
1) During a cycle the amount of water flowing into the boiler is measured (Δw in grams).
The temperature difference between the (assumed) reservoir/inflow temperature and the set
temperature is calculated. Using the specific heat capacity of water at 95°C (c = 4.21 J/g°C)
the energy needed to heat this water to the set temperature is calculated (ΔE in Joules).
Δw must be greater than 0 and is limited to 12 grams per second to avoid excessive power requests.
There is a ratio factor to adjust the amount of dynamic feed forward used.

ΔE = Δw * c * (T_set - T_inflow) * dffFactor

2) The energy needed (ΔE) is added to the Energy stock pile (E_ff in Joules).
E_ff = E_ff + ΔE

3) The energy stock pile is converted to a power request (P_ff in Watts) by dividing the
energy by the estimated sample time in seconds (Δt in seconds).
P_ff can not be greater than the maximum boiler power (P_boiler in Watts) minus
the static feed forward.

P_ff = min(E_ff / Δt,  P_boiler * (1 - staticFeedForward))

4) At the end of a cycle, the feedforward energy stock pile is reduced by the actual amount of energy
used.

E_ff = E_ff - (P_ff * Δt_actual)


*/

#ifndef ARDUPID_H
#define ARDUPID_H

#include <Arduino.h>
#include "dp_reservoir.h"
#include "dp_brew.h"

#define MAX_FLOW_RATE_G_PER_MS 0.012 // maximum flow rate in grams per millisecond for dynamic feed forward calculation (12 g/s)
#define SPECIFIC_HEAT_CAPACITY_WATER 4.21 // specific heat capacity of water in J/g°C, 95 °C
#define DFF_ENERGY_STOCK_PILE_MAX_MS 60000 // maximum energy stock pile in milliseconds (times boiler power)
#define DFF_ENERGY_STOCK_PILE_DECAY 0.90 // decay factor for energy stock pile per cycle

class DpPID
{
public:
    DpPID() {};

    void begin(double *input, double *output, double *setpoint, const double &p, const double &i, const double &d, const double &feedForward, const bool &dynamicFeedForwardEnabled, const unsigned int &minSamplePeriodMs, Reservoir *reservoir = nullptr);

    void start();
    void reset();
    void compute();
    void setOutputLimits(const double& min, const double& max);
    void setWindUpLimits(const double& min, const double& max);
    //void setDeadBand(const double& min, const double& max);
    void setCoefficients(const double& p, const double& i, const double& d);
    void setFeedForward(const double& feedForward, const bool& dynamicFeedForwardEnabled = false);
    void setSampleTime(const unsigned int& minSamplePeriodMs);
    
    void setSerialOutput(const bool& enabled) { serialOutput = enabled; }
    bool getSerialOutput() const { return serialOutput; }

    double P() {return termP;}
    double I() {return termI;}
    double D() {return termD;}

    void printToSerial();

protected:
    double calculateFeedForward();

    bool serialOutput = false; // enable/disable serial debug output

    double* input;
    double* output;
    double* setpoint;
    double Kp, Ki, Kd; // the coefficients for the proportional, integral, and derivative terms 
    double termP, termI, termD; // the calculated PID terms
    double unconstrainedOutput;
    double curError = 0, curInput = 0;
    double lastError = 0, lastSetpoint = 0, lastInput = 0;
    double windUpMin = -100, windUpMax = 5;
    double outputMin = 0, outputMax = 100;
    unsigned int minSamplePeriodMs = 1;
    unsigned long lastTime = 0;
    unsigned long curSampleTimeMs = 1000;

    double staticFeedForward = 0;  // set, 0-100 percentage of boiler power
    double dynamicFeedForward = 0; // calculated dynamic feed forward, 0-100 percentage of boiler power
    bool dynamicFeedForwardEnabled = false;
    unsigned long lastDynamicFeedForwardTime = 0;

    Reservoir* reservoir; // reservoir object for dynamic feed forward
    double previousReservoirWeight = 0; // previous weight for dynamic feed forward in grams
    double boilerPowerKiloWatt = 1.250; // boiler power (in kilowatt as it will be multiplied with sample time in ms)
    double dffFactor = 0.90; // Adjustable factor for dynamic feed forward
    double dffEnergyStockPile = 0; // Energy stock pile for dynamic feed forward in Joules

};


#endif // ARDUPID_H