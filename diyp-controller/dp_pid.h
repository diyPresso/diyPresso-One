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
There is a factor (dffFactor) to adjust the amount of dynamic feed forward, to tune and to prevent overshoot.

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

// Auto-tune parameters
#define AUTOTUNE_RELAY_OUTPUT 10.0 // Relay output amplitude (% of max power)
#define AUTOTUNE_MIN_CYCLES 3 // Minimum number of oscillation cycles to measure
#define AUTOTUNE_TIMEOUT_MS 1800000 // Auto-tune timeout (30 minutes)
#define AUTOTUNE_SETPOINT_BAND 0.3 // Band around setpoint for relay switching (+/- degC)
#define AUTOTUNE_PEAK_NOISE_BAND 0.08 // Noise band for peak detection (+/- degC)
#define AUTOTUNE_SETTLING_TIME_MS 25000 // Initial settling time before peak detection (25 seconds)

// Auto-tune results structure
struct AutoTuneResults {
    double Kp;
    double Ki;
    double Kd;
    double Ku;  // Ultimate gain (for reference)
    double Pu;  // Ultimate period in seconds (for reference)
    bool isValid;
    
    AutoTuneResults() : Kp(0), Ki(0), Kd(0), Ku(0), Pu(0), isValid(false) {}
};

typedef enum {
    AUTOTUNE_IDLE,
    AUTOTUNE_RUNNING,
    AUTOTUNE_SUCCESS,
    AUTOTUNE_FAILED_TIMEOUT,
    AUTOTUNE_FAILED_NO_OSCILLATION
} autotune_state_t;


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
    void setCoefficients(const double& p, const double& i, const double& d);
    void setFeedForward(const double& feedForward, const bool& dynamicFeedForwardEnabled = false);
    void setDynamicFeedForwardFactorPct(const double& factor); // In percentage (0.0 - 100.0%)
    void setSampleTime(const unsigned int& minSamplePeriodMs);
    
    void setSerialOutput(const bool& enabled) { serialOutput = enabled; }
    bool getSerialOutput() const { return serialOutput; }

    // Auto-tune functions
    void startAutoTune();
    void cancelAutoTune();
    autotune_state_t getAutoTuneState() const { return autotuneState; }
    bool isAutoTuning() const { return autotuneState == AUTOTUNE_RUNNING; }
    AutoTuneResults getAutoTuneResults() const;

    double P() {return termP;}
    double I() {return termI;}
    double D() {return termD;}

    void printToSerial();

protected:
    double calculateFeedForward();
    void processAutoTune();

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

    // Auto-tune variables
    autotune_state_t autotuneState = AUTOTUNE_IDLE;
    unsigned long autotuneStartTime = 0;
    AutoTuneResults autotuneResults;
    bool atRelayState = false;      // Current relay state (high/low)
    double atPeakHigh = 0;          // Highest peak temperature
    double atPeakLow = 0;           // Lowest peak temperature  
    unsigned long atPeakTimes[10];  // Store times of peaks
    int atPeakCount = 0;            // Number of peaks detected
    double atLastValue = 0;         // Previous input value
    bool atRisingEdge = true;       // Tracking if we're rising or falling
};


#endif // ARDUPID_H