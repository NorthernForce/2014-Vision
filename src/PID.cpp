#include "PID.h"

PID::PID(float p, float i, float d) :
	p_gain(p), i_gain(i), d_gain(d), 
	prev_error(0.0), accumulator(0.0)
{}

void PID::reset() {
	prev_error = 0.0;
	accumulator = 0.0;
}

float PID::getOutput(float error, float dt) {
	accumulator += error*dt;
	float de = error - prev_error;
	float output = p_gain*error + i_gain*accumulator + d_gain*de/dt;
	prev_error = error;
	return output;
}

float PID::getOutput(float error, float error_integral, float dt) {
	float de = error - prev_error;
	float output = p_gain*error + i_gain*error_integral + d_gain*de/dt;
	prev_error = error;
	return output;
}
