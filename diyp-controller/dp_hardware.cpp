/*
 * dp_hardware.cpp
 * Hardware detection and model identification
 * (c) 2026 diyPresso
 */
#include <Arduino.h>
#include "dp_hardware.h"

const char* Hardware::model_name() const
{
  switch (_model) {
    case MODEL_ONE: return "One";
    case MODEL_TWO: return "Two";
    default:        return "Unknown";
  }
}

void Hardware::detect_model()
{
  // TODO: read GPIO jumper pin to detect model
  _model = MODEL_TWO;
}
