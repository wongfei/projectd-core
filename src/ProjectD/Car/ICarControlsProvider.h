#pragma once

#include "Car/CarControls.h"

namespace D {

struct CarControlsInput
{
	float steerLock = 0;
	float speed = 0;
};

struct VibrationDef
{
	float curbs = 0;
	float gforce = 0;
	float slips = 0;
	float engine = 0;
	float abs = 0;
};

struct ICarControlsProvider : public virtual IObject
{
	virtual void acquireControls(CarControlsInput* input, CarControls* controls, float dt) = 0;
	virtual void sendFF(float ff, float damper, float userGain) = 0;
	virtual void setVibrations(VibrationDef* vib) = 0;
	virtual float getFeedbackFilter() = 0;
};

}
