#include "Control.h"

Control::Control() :
	lowpass(0.1)
{
	reset();
}

void Control::reset() {
	state = 0;
}

cv::Point3f Control::getOutput(cv::Point2f ball, int size, float dt) {
	float x = 0.0;
	float y = 0.0;
	float w = 0.0;

	switch(state) {
	default:
	case 0:
		++state; break;
	case 1:
		init_dtan_el = (ball.y - prev_tan_el) / dt;
		lowpass.init(0.0);
		++state; break;
	case 2:
		float dtan_el = (ball.y - prev_tan_el) / dt;
		y = 1.1*lowpass(dtan_el - init_dtan_el, dt);
	}

	x = 1.2*ball.x;

	prev_tan_el = ball.y;
	return cv::Point3f(x,y,w);
}
