#include "dp_pid.h"

/// @brief Initialise the DpPID controller
/// @param input pointer to the input variable
/// @param output pointer to the output variable
/// @param setpoint pointer to the setpoint variable
/// @param p the coefficient for the proportional term
/// @param i the coefficient for the integral term
/// @param d the coefficient for the derivative term
/// @param feedForward the feed forward term
/// @param minSamplePeriodMs the minimum sample period in milliseconds
void DpPID::begin(double *input, double *output, double *setpoint, 
             const double &p, const double &i, const double &d, 
             const double &feedForward, const unsigned int &minSamplePeriodMs)
{
    this->input = input;
    this->output = output;
    this->setpoint = setpoint;

    this->setCoefficients(p, i, d);

    this->setFeedForward(feedForward);
    this->setSampleTime(minSamplePeriodMs);
}

void DpPID::start()
{
    reset();
}

void DpPID::reset()
{
    curError = 0;
    curInput = 0;

    lastError = 0;
    lastInput = *input;
    lastSetpoint = *setpoint;

    termP = 0;
    termI = 0;
    termD = 0;

    lastTime = millis();
    curSampleTimeMs = 0;
}

void DpPID::compute()
{
    unsigned long now = millis();
    curSampleTimeMs = now - lastTime;
    if (curSampleTimeMs >= minSamplePeriodMs) // check if enough time has passed, minSamplePeriodMs can't be < 1ms
    {
        curError = *setpoint - *input; // temp diff between setpoint and actual
        double dInput = *input - lastInput; // the change in temperature

        // proportional term
        termP = Kp * curError; // Kp time the error in temperature.

        // integral term
        #ifdef PID_DEBUG
            Serial.print("curError: ");
            Serial.print(curError);
            Serial.print("lastError: ");
            Serial.println(lastError);
        #endif
        termI = termI + Ki * (curSampleTimeMs / 1000.0) * (curError + lastError) / 2.0; // trapezoidal integration: sum of the error over time.
        termI = constrain(termI, windUpMin, windUpMax); // prevent integral wind-up

        // derivative term
        termD = Kd * -1 * dInput * 1000.0 / curSampleTimeMs;


        unconstrainedOutput = feedForward + termP + termI + termD;
        *output = constrain(unconstrainedOutput, outputMin, outputMax);

        lastInput = *input;
        lastSetpoint = *setpoint;
        lastError = curError;
        lastTime = now;

    #ifdef PID_DEBUG
        printToSerial();
    #endif
    }
}

void DpPID::setOutputLimits(const double &min, const double &max)
{
    if (max > min)
    {
        outputMax = max;
        outputMin = min;
    }
}

void DpPID::setWindUpLimits(const double &min, const double &max)
{
    if (max > min)
    {
        windUpMax = max;
        windUpMin = min;
    }
}

void DpPID::setCoefficients(const double &p, const double &i, const double &d)
{
    Kp = p;
    Ki = i;
    Kd = d;
}

void DpPID::setFeedForward(const double &feedForward)
{
    this->feedForward = feedForward;
}

/// @brief Set the sample time for the PID controller
/// @param minSamplePeriodMs the minimum sample period in milliseconds
void DpPID::setSampleTime(const unsigned int &minSamplePeriodMs)
{
    this->minSamplePeriodMs = max(minSamplePeriodMs, 1); // make sure it is at least 1ms
}

void DpPID::printToSerial()
{
    Serial.print("input: ");
    Serial.print(*input);
    Serial.print(", setpoint: ");
    Serial.print(*setpoint);
    Serial.print(", P: ");
    Serial.print(termP);
    Serial.print(", I: ");
    Serial.print(termI);
    Serial.print(", D: ");
    Serial.print(termD);
    Serial.print(", FF: ");
    Serial.print(feedForward);
    Serial.print(", Output: ");
    Serial.print(*output);
    Serial.print(", Unconstrained Output: ");
    Serial.println(unconstrainedOutput);

    Serial.print("Kp: ");
    Serial.print(Kp);
    Serial.print(", Ki: ");
    Serial.print(Ki);
    Serial.print(", Kd: ");
    Serial.print(Kd);
    Serial.print(", Windup Min: ");
    Serial.print(windUpMin);
    Serial.print(", Windup Max: ");
    Serial.println(windUpMax);

}
