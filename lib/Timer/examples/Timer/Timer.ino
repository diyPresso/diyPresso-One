#include "Timer.h"

Timer timer;

void setup() {
  Serial.begin(9600);
  timer.start();
  if(timer.state() == RUNNING) Serial.println("timer running");
  delay(1000);
  timer.stop();
  if(timer.state() == STOPPED) Serial.println("timer stopped");
  Serial.print("time elapsed ms: ");
  Serial.println(timer.read());
  }

void loop() {
  return;
  }
