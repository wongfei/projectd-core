#pragma once

#include "Core/Core.h"

namespace D {

struct CarControls
{
	float steer = 0;
	float clutch = 0;
	float brake = 0;
	float handBrake = 0;
	float gas = 0;
	int8_t isShifterSupported = 1;
	int8_t requestedGearIndex = -1; // 0=R, 1=N, 2=H1, 3=H2, 4=H3, 5=H4, 6=H5, 7=H6
	int8_t gearUp = 0;
	int8_t gearDn = 0;
};

}
