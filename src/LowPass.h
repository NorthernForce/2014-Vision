#pragma once

class LowPass {
private:
	float prev;
	float responseTime;
public:
	LowPass(float reponseTime, float init = 0.0);
	void init(float init);
	float operator()(float val, float dt);
};
