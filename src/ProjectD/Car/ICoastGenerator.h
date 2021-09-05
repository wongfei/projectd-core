#pragma once

#include "Car/CarCommon.h"

namespace D {

struct ICoastGenerator : public virtual IObject
{
	virtual float getCoastTorque() = 0;
};

}
