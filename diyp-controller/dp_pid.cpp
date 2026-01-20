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
        // If auto-tuning is running, process auto-tune instead of normal PID
        if (autotuneState == AUTOTUNE_RUNNING) {
            processAutoTune();
            lastTime = now; // Update lastTime to avoid large jumps after autotune
            return;
        }

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

/**
 * @brief Sets the dynamic feed-forward factor for the PID controller. 
 * This factor influences how aggressively the controller responds to changes in
 * reservoir weight and can be used to prevent overshoot.
 * This method converts a percentage value to a normalized factor and stores it.
 * 
 * @param factor The feed-forward factor as a percentage (0-150%).
 *               Values outside this range will be clamped to the nearest valid value.
 */
void DpPID::setDynamicFeedForwardFactorPct(const double &factor)
{
    dffFactor = constrain(factor / 100.0, 0.0, 1.5); // convert percentage to factor
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
    Serial.print(Ki, 4);
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

// ============================================================================
// Auto-Tune Implementation (Relay Method / Ziegler-Nichols)
// ============================================================================

void DpPID::startAutoTune()
{

    Serial.println("=== Starting Auto-tune ===");
    Serial.print("Auto-tune Setpoint: ");
    Serial.print(*setpoint);
    Serial.println("°C");
    Serial.print("Relay amplitude: ");
    Serial.print(AUTOTUNE_RELAY_OUTPUT);
    Serial.println("%");
    Serial.print("Setpoint band: ");
    Serial.print(AUTOTUNE_SETPOINT_BAND);
    Serial.println("°C");
    Serial.print("Peak detection noise band: ");
    Serial.print(AUTOTUNE_PEAK_NOISE_BAND);
    Serial.println("°C");

    //TODO: only start if temperature is within the band of the setpoint

    autotuneState = AUTOTUNE_RUNNING;
    autotuneStartTime = millis();
    
    // Initialize auto-tune results
    autotuneResults = AutoTuneResults(); // Reset to defaults
    
    // Initialize tracking variables
    atRelayState = false;
    atPeakHigh = *input;
    atPeakLow = *input;
    atPeakCount = 0;
    atLastValue = *input;
    atRisingEdge = true;
    
    // Clear peak times array
    for (int i = 0; i < 10; i++) {
        atPeakTimes[i] = 0;
    }
    
    // Start with relay ON to begin oscillations
    *output = AUTOTUNE_RELAY_OUTPUT;
    atRelayState = true;
    
    if (serialOutput) {
        Serial.println("Auto-tune initialized successfully");
    }
}

void DpPID::cancelAutoTune()
{
    if (serialOutput) {
        Serial.println("Auto-tune cancelled");
    }
    
    autotuneState = AUTOTUNE_IDLE;
    *output = 0; // Turn power off

    Serial.println("=== Auto-tune cancelled ===");
}

AutoTuneResults DpPID::getAutoTuneResults() const
{
    return autotuneResults;
}

void DpPID::processAutoTune()
{
    // Check for timeout
    if ((millis() - autotuneStartTime) > AUTOTUNE_TIMEOUT_MS) {
        autotuneState = AUTOTUNE_FAILED_TIMEOUT;
        *output = 0;
        if (serialOutput) {
            Serial.println("=== Auto-tune FAILED: Timeout ===");
        }
        return;
    }

    double inputValue = *input;
    double error = *setpoint - inputValue;
    
    // Check if we're still in settling time (skip peak detection)
    bool inSettlingTime = (millis() - autotuneStartTime) < AUTOTUNE_SETTLING_TIME_MS;
    
    // Detect peaks (local maxima and minima) with noise band
    bool isPeak = false;
    
    Serial.print("inSettlingTime: ");
    Serial.print(inSettlingTime); 
    Serial.print(", LastValue: ");
    Serial.print(atLastValue);
    Serial.print(", inputValue: ");
    Serial.print(inputValue);
    Serial.print(", risingEdge: ");
    Serial.println(atRisingEdge);


    if (inSettlingTime) { // during initial settling time, just update last value
        atLastValue = inputValue;
    } else {
        if (atRisingEdge) {
            // Looking for a maximum (peak high)
            if (inputValue < (atLastValue - AUTOTUNE_PEAK_NOISE_BAND)) {
                // Started falling, we just passed a peak
                isPeak = true;
                if (atLastValue > atPeakHigh) {
                    atPeakHigh = atLastValue;
                }
                atRisingEdge = false;
                
                if (serialOutput) {
                    Serial.print("Peak HIGH detected: ");
                    Serial.print(atLastValue);
                    Serial.println("°C");
                }
            }

            if (inputValue > atLastValue) {
                // Only update on higher values
                atLastValue = inputValue;
            }

        } else {
            // Looking for a minimum (peak low)
            if (inputValue > (atLastValue + AUTOTUNE_PEAK_NOISE_BAND)) {
                // Started rising, we just passed a valley
                isPeak = true;
                if (atLastValue < atPeakLow || atPeakLow == 0) {
                    atPeakLow = atLastValue;
                }
                atRisingEdge = true;
                
                if (serialOutput) {
                    Serial.print("Peak LOW detected: ");
                    Serial.print(atLastValue);
                    Serial.println("°C");
                }
            }

            if (inputValue < atLastValue) {
                // Only update on lower values
                atLastValue = inputValue;
            }
        }
    }
    
    // Record peak time
    if (isPeak && atPeakCount < 10) {
        atPeakTimes[atPeakCount] = millis();
        atPeakCount++;
    }
    
    // Relay control: switch output based on error
    if (error > AUTOTUNE_SETPOINT_BAND) {
        // Below setpoint - turn ON
        atRelayState = true;
        *output = AUTOTUNE_RELAY_OUTPUT;
    } else if (error < -AUTOTUNE_SETPOINT_BAND) {
        // Above setpoint - turn OFF or reduce
        atRelayState = false;
        *output = 0.0;
    }
    // Within noise band - maintain current state
    
    
    // After enough cycles, calculate PID parameters
    if (atPeakCount >= (AUTOTUNE_MIN_CYCLES * 2)) {
        // Calculate average period (time between same-type peaks)
        // Need at least 2 complete cycles (4 peaks minimum)
        double totalPeriod = 0;
        int periodCount = 0;
        
        // Calculate periods between alternating peaks (full oscillation cycles)
        for (int i = 2; i < atPeakCount; i++) {
            unsigned long period = atPeakTimes[i] - atPeakTimes[i-2];
            totalPeriod += period;
            periodCount++;
        }
        
        if (periodCount > 0) {
            autotuneResults.Pu = (totalPeriod / periodCount) / 1000.0; // Convert to seconds
            
            // Calculate amplitude of oscillation
            double amplitude = (atPeakHigh - atPeakLow) / 2.0;
            
            if (amplitude < AUTOTUNE_SETPOINT_BAND) {
                autotuneState = AUTOTUNE_FAILED_NO_OSCILLATION;
                *output = 0;
                if (serialOutput) {
                    Serial.println("Auto-tune FAILED: Insufficient oscillation amplitude");
                    Serial.print("Amplitude: ");
                    Serial.print(amplitude);
                    Serial.print("°C (need > ");
                    Serial.print(AUTOTUNE_SETPOINT_BAND);
                    Serial.println("°C)");
                }
                return;
            }
            
            // Calculate ultimate gain (Ku)
            // Ku = (4 * d) / (π * a)
            // where d = relay output amplitude (half the peak-to-peak swing), a = oscillation amplitude
            autotuneResults.Ku = (2.0 * AUTOTUNE_RELAY_OUTPUT ) / (3.14159 * amplitude);
            
            // Ziegler-Nichols PID tuning rules (classic)
            // Kp = 0.6 * Ku
            // Ki = 1.2 * Ku / Pu
            // Kd = 0.075 * Ku * Pu
            autotuneResults.Kp = 0.6 * autotuneResults.Ku;
            autotuneResults.Ki = 1.2 * autotuneResults.Ku / autotuneResults.Pu;
            autotuneResults.Kd = 0.075 * autotuneResults.Ku * autotuneResults.Pu;
            autotuneResults.isValid = true;
            
            autotuneState = AUTOTUNE_SUCCESS;
            *output = 0;
            
            if (serialOutput) {
                Serial.println("\n=== Auto-Tune COMPLETE ===");
                Serial.print("Ultimate Gain (Ku): ");
                Serial.println(autotuneResults.Ku, 4);
                Serial.print("Ultimate Period (Pu): ");
                Serial.print(autotuneResults.Pu, 2);
                Serial.println(" seconds");
                Serial.print("Oscillation amplitude: ");
                Serial.print(amplitude, 2);
                Serial.println("°C");
                Serial.print("Peak High: ");
                Serial.print(atPeakHigh, 2);
                Serial.println("°C");
                Serial.print("Peak Low: ");
                Serial.print(atPeakLow, 2);
                Serial.println("°C");
                Serial.println("\nCalculated PID parameters (Ziegler-Nichols):");
                Serial.print("Kp: ");
                Serial.println(autotuneResults.Kp, 4);
                Serial.print("Ki: ");
                Serial.println(autotuneResults.Ki, 4);
                Serial.print("Kd: ");
                Serial.println(autotuneResults.Kd, 4);
                Serial.println("=========================\n");
            }
        }
    }
}

