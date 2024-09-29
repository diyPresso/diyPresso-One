#pragma once
#include "Arduino.h"
#include "FireTimer.h"




const byte PRINT_INPUT    = B1;
const byte PRINT_OUTPUT   = B10;
const byte PRINT_SETPOINT = B100;
const byte PRINT_BIAS     = B1000;
const byte PRINT_P        = B10000;
const byte PRINT_I        = B100000;
const byte PRINT_D        = B1000000;




enum pOn { P_ON_E, P_ON_M };
enum mode { OFF, ON };
enum dir { FORWARD, BACKWARD };




class ArduPID
{
public:
	double* input;
	double* output;
	double* setpoint;




	virtual void begin(double* _input,
		               double* _output,
		               double* _setpoint,
		               const double& _p = 0,
		               const double& _i = 0,
		               const double& _d = 0,
		               const pOn& _pOn = P_ON_E,
		               const dir& _direction = FORWARD,
		               const unsigned int& _minSamplePeriodMs = 0,
		               const double& _bias = 0);
	void start();
	void reset();
	void stop();
	virtual void compute();
	void setOutputLimits(const double& min, const double& max);
	void setWindUpLimits(const double& min, const double& max);
	void setDeadBand(const double& min, const double& max);
	void setPOn(const pOn& _pOn);
	void setBias(const double& _bias);
	void setCoefficients(const double& _p, const double& _i, const double& _d);
	void setDirection(const dir& _direction);
	void reverse();
	void setSampleTime(const unsigned int& _minSamplePeriodMs);

	double B();
	double P();
	double I();
	double D();
	
	void debug(Stream* stream = &Serial,
	           const char* controllerName = "controller",
		       const byte& mask = 0xFF);




protected:
	double bias;

	double outputMax = 255;
	double outputMin = 0;

	double windupMax = 1000;
	double windupMin = -1000;

	double deadBandMax = 0;
	double deadBandMin = 0;

	double curError;
	double curSetpoint;
	double curInput;

	double lastError;
	double lastSetpoint;
	double lastInput;

	double pIn;
	double iIn;
	double dIn;

	double kp;
	double ki;
	double kd;

	double pOut;
	double iOut;
	double dOut;

	pOn pOnType;
	mode modeType;
	dir direction;

	FireTimer timer;
};