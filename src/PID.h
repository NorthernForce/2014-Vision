#pragma once

class PID {
public:
	float p_gain;
	float i_gain;
	float d_gain;

	float prev_error;
	float accumulator;

	PID(float p, float i, float d);
	void reset();
	float getOutput(float error, float dt);
	float getOutput(float error, float error_integral, float dt);
};
