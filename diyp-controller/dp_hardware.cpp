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

// Drive AREF low and high while A0 is pulled the opposite way. A0 only follows AREF in both cases
// when the jumper is present, so a floating or stuck A0 is never mistaken for a Model Two.
// Only AREF is ever driven; A0 stays an input, so the jumper can never short two outputs.
void Hardware::detect_model()
{
  pinMode(PIN_MODEL_SENSE, INPUT_PULLUP);
  pinMode(PIN_MODEL_DRIVE, OUTPUT);
  digitalWrite(PIN_MODEL_DRIVE, LOW);
  delay(1);
  bool follows_low = digitalRead(PIN_MODEL_SENSE) == LOW;

  pinMode(PIN_MODEL_SENSE, INPUT_PULLDOWN);
  digitalWrite(PIN_MODEL_DRIVE, HIGH);
  delay(1);
  bool follows_high = digitalRead(PIN_MODEL_SENSE) == HIGH;

  // Leave both pins high-impedance
  pinMode(PIN_MODEL_DRIVE, INPUT);
  pinMode(PIN_MODEL_SENSE, INPUT);

  _model = (follows_low && follows_high) ? MODEL_TWO : MODEL_ONE;
}
