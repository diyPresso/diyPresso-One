/* Ticker library code is placed under the MIT license
 * Copyright (c) 2020 Stefan Staub
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Timer.h"

Timer::Timer(resolution_t resolution) {
	this->resolution = resolution;
	}

Timer::~Timer() {}

void Timer::start() {
	elapsed = 0;
	if(resolution == MILLIS) started = millis();
	if(resolution == MICROS) started = micros();
	status = RUNNING;
	}

void Timer::pause() {
	if (status == RUNNING) {
		if(resolution == MILLIS) elapsed = elapsed + millis() - started;
		if(resolution == MICROS) elapsed = elapsed + micros() - started;
		status = PAUSED;
		}
	}

void Timer::resume() {
	if (status == PAUSED) {
		if(resolution == MILLIS) started = millis();
		if(resolution == MICROS) started = micros();
		status = RUNNING;
		}
	}

void Timer::stop() {
	if (status == RUNNING) {
		if(resolution == MILLIS) elapsed = millis() - started + elapsed;
		if(resolution == MICROS) elapsed = micros() - started + elapsed;
		}
	status = STOPPED;
	}

uint32_t Timer::read() {
	if (status == RUNNING) {
		if(resolution == MILLIS) return millis() - started + elapsed;
		if(resolution == MICROS) return micros() - started + elapsed;
		}
	return elapsed;
	}

status_t Timer::state() {
	return status;
	}