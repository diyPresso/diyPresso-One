/*
 * dp_hardware.cpp
 * Hardware detection and model identification
 * (c) 2026 diyPresso
 */
#include <Arduino.h>
#include "dp_hardware.h"

machine_model_t machineModel = MODEL_ONE;

const char* model_name()
{
  switch (machineModel) {
    case MODEL_ONE: return "One";
    case MODEL_TWO: return "Two";
    default:        return "Unknown";
  }
}

void detect_model()
{
  // TODO: read GPIO jumper pin to detect model
  machineModel = MODEL_ONE;
}
