#pragma once

#include "Car/CarCommon.h"

namespace D {

struct ITorqueGenerator : public virtual IObject
{
	virtual float getOutputTorque() = 0;
};

}
