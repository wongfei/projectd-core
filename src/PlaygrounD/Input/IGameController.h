#pragma once

#include "Core/Core.h"

namespace D {

enum class EInputType
{
	Axis = 0,
	Button,
	Pov,
};

struct IGameController : public virtual IObject
{
	virtual float getInput(EInputType type, int id) = 0;
	virtual void sendFF(float v, float damper) = 0;
};

}
