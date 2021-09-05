#include "Car/Turbo.h"

namespace D {

Turbo::Turbo(const TurboDef& _data) : data(_data)
{}

Turbo::~Turbo()
{}

void Turbo::step(float gas, float rpms, float dt)
{
	float fNewRotation = 0;
	float fLag = 0;

	if (rpms > 0.0f && gas > 0.0f)
	{
		fNewRotation = powf(tclamp(((gas * rpms) / data.rpmRef), 0.0f, 1.0f), data.gamma);
	}

	if (fNewRotation <= rotation)
	{
		fLag = tclamp((dt * data.lagDN), 0.0f, 1.0f);
	}
	else
	{
		fLag = tclamp((dt * data.lagUP), 0.0f, 1.0f);
	}

	rotation += ((fNewRotation - rotation) * fLag);

	if (data.wastegate != 0.0f)
	{
		float fUserWG = data.wastegate * userSetting;
		if ((data.maxBoost * rotation) > fUserWG)
		{
			rotation = fUserWG / data.maxBoost;
		}
	}
}

void Turbo::reset()
{
	rotation = 0;
}

void Turbo::setTurboBoostLevel(float value)
{
	if (data.isAdjustable)
		userSetting = value;
	else
		userSetting = 1.0f;
}

float Turbo::getBoost() const
{
	return data.maxBoost * rotation;
}

}
