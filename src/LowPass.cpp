#include "LowPass.h"

LowPass::LowPass(float responseTime, float init) :
	prev(init),
	responseTime(responseTime)
{
}

void LowPass::init(float init) {
	prev = init;
}

float LowPass::operator()(float val, float dt) {
	float output = (1.0-dt/responseTime)*prev + (dt/responseTime)*val;
	prev = val;
	return val;
}
