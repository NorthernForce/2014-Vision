#pragma once
#include <opencv2/core/core.hpp>
#include "LowPass.h"

class Control {
private:
	int state;
	float prev_tan_el;
	float init_dtan_el;
	LowPass lowpass;
public:
	Control();
	void reset();
	cv::Point3f getOutput(cv::Point2f ball, int size, float dt);
};
