/* 
diyPresso PID controller
(c) 2025 diyPresso

Loosely based on the powerbroker2/ArduPID libary 
*/

#ifndef ARDUPID_H
#define ARDUPID_H

#include <Arduino.h>

//#define PID_DEBUG

class DpPID
{
public:
    DpPID() {};

    void begin(double *input, double *output, double *setpoint, const double &p, const double &i, const double &d, const double &feedForward, const unsigned int &minSamplePeriodMs);

    void start();
    void reset();
    void compute();
    void setOutputLimits(const double& min, const double& max);
    void setWindUpLimits(const double& min, const double& max);
    //void setDeadBand(const double& min, const double& max);
    void setCoefficients(const double& p, const double& i, const double& d);
    void setFeedForward(const double& feedForward);
    void setSampleTime(const unsigned int& minSamplePeriodMs);

    double P() {return termP;}
    double I() {return termI;}
    double D() {return termD;}

    void printToSerial();

protected:
    double* input;
    double* output;
    double* setpoint;
    double Kp, Ki, Kd; // thee coefficients for the proportional, integral, and derivative terms 
    double feedForward;
    double termP, termI, termD; // the calculated PID terms
    double unconstrainedOutput;
    double curError = 0, curInput = 0;
    double lastError = 0, lastSetpoint = 0, lastInput = 0;
    //double deadBandMin, deadBandMax;
    double windUpMin = -100, windUpMax = 5;
    double outputMin = 0, outputMax = 100;
    unsigned int minSamplePeriodMs = 0;
    unsigned long lastTime = 0;
    unsigned long curSampleTimeMs = 0;
    //bool modeType;
};


#endif // ARDUPID_H