#include "dp_pid.h"

/// @brief Initialise the DpPID controller
/// @param input pointer to the input variable
/// @param output pointer to the output variable
/// @param setpoint pointer to the setpoint variable
/// @param p the coefficient for the proportional term
/// @param i the coefficient for the integral term
/// @param d the coefficient for the derivative term
/// @param staticFeedForward the static feed forward term
/// @param minSamplePeriodMs the minimum sample period in milliseconds
/// @param reservoir pointer to the reservoir object to get weight for dynamic feed forward
/// @param brewProcess pointer to the brew process object to check if brewing for dynamic feed forward
void DpPID::begin(double *input, double *output, double *setpoint, 
             const double &p, const double &i, const double &d, 
             const double &staticFeedForward, const bool &dynamicFeedForwardEnabled, const unsigned int &minSamplePeriodMs,
             Reservoir *reservoir)
{
    this->input = input;
    this->output = output;
    this->setpoint = setpoint;
    this->reservoir = reservoir;

    this->setCoefficients(p, i, d);

    this->setFeedForward(staticFeedForward, dynamicFeedForwardEnabled);
    this->setSampleTime(minSamplePeriodMs);

    lastTime = millis();
    curSampleTimeMs = 0;

    this->previousReservoirWeight = reservoir ? reservoir->weight() : 0;
    this->dffEnergyStockPile = 0;

}

void DpPID::start()
{
    lastError = 0;
    lastInput = *input;
    lastSetpoint = *setpoint;

    lastTime = millis();

    this->previousReservoirWeight = reservoir ? reservoir->weight() : 0;
    this->dffEnergyStockPile = 0;
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

    dynamicFeedForward = 0;
    dynamicFeedForwardEnabled = false;
    this->previousReservoirWeight = reservoir ? reservoir->weight() : 0;
    this->dffEnergyStockPile = 0;
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
        termI = termI + Ki * (curSampleTimeMs / 1000.0) * (curError + lastError) / 2.0; // trapezoidal integration: sum of the error over time.
        termI = constrain(termI, windUpMin, windUpMax); // prevent integral wind-up

        // derivative term
        termD = Kd * -1 * dInput * 1000.0 / curSampleTimeMs;



        unconstrainedOutput = calculateFeedForward() + termP + termI + termD;
        *output = constrain(unconstrainedOutput, outputMin, outputMax);

        lastInput = *input;
        lastSetpoint = *setpoint;
        lastError = curError;
        lastTime = now;

        if (serialOutput) {
            printToSerial();
        }
    }
}

double DpPID::calculateFeedForward()
{
    if (reservoir == nullptr) return staticFeedForward;

    double currentWeight = reservoir->weight();

    // Reduce the energy stock pile with previous dynamic feed forward and actual time passed, decay the stock pile as well.
    dffEnergyStockPile -= dynamicFeedForward * curSampleTimeMs * boilerPowerKiloWatt / 100.0; // in Joules
    dffEnergyStockPile = max(dffEnergyStockPile, 0.0); // don't allow negative stock pile
    dffEnergyStockPile = dffEnergyStockPile * DFF_ENERGY_STOCK_PILE_DECAY; // decay the energy stock pile

    if (curSampleTimeMs < 1 || *setpoint <= reservoir->outflowTemp()) {
        dynamicFeedForward = 0;
    } else {
        double deltaWeight = previousReservoirWeight - currentWeight; // Reservoir weight decreases when water is flowing in the boiler

        if(this->dynamicFeedForwardEnabled || (long)(this->lastTime - this->lastDynamicFeedForwardTime < 0)) { // only increase stock pile when dynamic feed forward is enabled (typically when pump is on) or was enabled during last cycle

            // delta weight is limited to the maximum flow rate and should be > 0
            deltaWeight = constrain(deltaWeight, 0, MAX_FLOW_RATE_G_PER_MS * curSampleTimeMs);

            // Serial.println();
            // Serial.print("deltaWeight (g): ");
            // Serial.println(deltaWeight, 2);

            // Update the energy stock pile
            dffEnergyStockPile += deltaWeight * SPECIFIC_HEAT_CAPACITY_WATER * ( *setpoint - reservoir->outflowTemp()) * dffFactor; // in Joules
            dffEnergyStockPile = constrain(dffEnergyStockPile, 0.0, DFF_ENERGY_STOCK_PILE_MAX_MS * boilerPowerKiloWatt); // limit the energy stock pile

            // Serial.print("energy stock pile (kJ): ");
            // Serial.println(dffEnergyStockPile / 1000.0, 2);
        }

        // Calculate the feed forward power request
        dynamicFeedForward = 100 * dffEnergyStockPile / (curSampleTimeMs * boilerPowerKiloWatt); // in percentage of boiler power
        dynamicFeedForward = constrain(dynamicFeedForward, 0, (100 - staticFeedForward));
    }

    // Update the previous reservoir weight
    previousReservoirWeight = currentWeight;

    return dynamicFeedForward + staticFeedForward;
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

void DpPID::setFeedForward(const double &staticFeedForward, const bool &dynamicFeedForwardEnabled)
{
    this->staticFeedForward = constrain(staticFeedForward, 0.0, 100.0);

    if (this->dynamicFeedForwardEnabled && !dynamicFeedForwardEnabled) {
        // Dynamic feed forward is being disabled, store last dynamic feed forward time
        this->lastDynamicFeedForwardTime = millis();
    }
    this->dynamicFeedForwardEnabled = dynamicFeedForwardEnabled;
}

/// @brief Set the sample time for the PID controller
/// @param minSamplePeriodMs the minimum sample period in milliseconds
void DpPID::setSampleTime(const unsigned int &minSamplePeriodMs)
{
    this->minSamplePeriodMs = max(minSamplePeriodMs, 1); // make sure it is at least 1ms
}

void DpPID::printToSerial()
{
    Serial.print("DP_PID_STATE ");
    Serial.print("input: ");
    Serial.print(*input);
    Serial.print(", setpoint: ");
    Serial.print(*setpoint);
    Serial.print(", termP: ");
    Serial.print(termP);
    Serial.print(", termI: ");
    Serial.print(termI);
    Serial.print(", termD: ");
    Serial.print(termD);
    Serial.print(", sFF: ");
    Serial.print(staticFeedForward);
    Serial.print(", dFF: ");
    Serial.print(dynamicFeedForward);
    Serial.print(", output: ");
    Serial.print(*output);
    Serial.print(", unconstrainedOutput: ");
    Serial.print(unconstrainedOutput);
    Serial.print(", Kp: ");
    Serial.print(Kp);
    Serial.print(", Ki: ");
    Serial.print(Ki);
    Serial.print(", Kd: ");
    Serial.print(Kd);
    Serial.print(", windupMin: ");
    Serial.print(windUpMin);
    Serial.print(", windupMax: ");
    Serial.print(windUpMax);
    Serial.print(", stockPileKJ): ");
    Serial.print(dffEnergyStockPile / 1000.0, 2);
    Serial.print(", prevWeight: ");
    Serial.println(previousReservoirWeight);
    
}
