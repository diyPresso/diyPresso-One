// dp_hardware.h
// Define the hardware (revision, pin numbers, scaling, etc)

#ifndef HARDWARE_H
#define HARDWARE_H

#define HARDWARE_REVISION "1"

// Machine model (detected at runtime via GPIO jumper)
typedef enum { MODEL_ONE = 1, MODEL_TWO = 2 } machine_model_t;

class Hardware
{
public:
  static Hardware& instance() { static Hardware h; return h; }
  void detect_model();
  const char* model_name() const;
  bool has_steam_group() const { return _model == MODEL_TWO; }
  machine_model_t model() const { return _model; }

private:
  Hardware() : _model(MODEL_ONE) {}
  machine_model_t _model;
};

inline Hardware& hardware = Hardware::instance();

// GPIO
#define PIN_BREW_SWITCH 1
#define PIN_SSR_PUMP 2
#define PIN_SSR_HEATER 3

// HX711
#define PIN_HX711_CLK 4
#define PIN_HX711_DAT 5

// PT1000
#define PIN_THERM_CS 7
#define PIN_THERM_MOSI 8
#define PIN_THEM_SCLK 9
#define PIN_THERM_MISO 10
#define THERM_RREF 4300.0 // The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define THERM_RNOMINAL 1000.0 // The 'nominal' 0-degrees-C resistance of the sensor (100.0 for PT100, 1000.0 for PT1000)

// DISPLAY
#define PIN_SDA 11
#define PIN_SCL 12
#define DISPLAY_I2C_ADDRESS 0x27

// ENCODER
#define PIN_ENC_A 14  // pin nr
#define BIT_ENC_A 22  // bit nr in GPIO register (PB22)
#define PIN_ENC_B 13  // pin nr
#define BIT_ENC_B 23  // bit nr in GPIO register (PB23)
#define PIN_ENC_S 0

// STEAM (Model Two only)
#define PIN_SSR_STEAM_PUMP A4
#define PIN_SSR_STEAM_HEATER A5
#define PIN_STEAM_SWITCH A6
#define PIN_STEAM_THERM_CS 6  // D6, shares SPI bus with boiler sensor


// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      4300.0

// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  1000.0


#endif // HARDWARE_H