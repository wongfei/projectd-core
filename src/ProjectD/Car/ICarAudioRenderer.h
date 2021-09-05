#pragma once

#include "Core/Core.h"

namespace D {

struct ICarAudioRenderer : public virtual IObject
{
	virtual void update(float dt) = 0;
};

}
