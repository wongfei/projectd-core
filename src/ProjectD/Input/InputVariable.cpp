#include "Input/InputVariable.h"
#include "Core/Math.h"

namespace D {

void InputVariable::bind(IGameController* _device, EInputType _type, int _id)
{
	device = _device;
	type = _type;
	id = _id;
}

void InputVariable::setRange(float r0, float r1)
{
	minv = r0;
	maxv = r1;
	useRange = (fabsf(r0) > 0.0001f) && (fabsf(r1) > 0.0001f);
}

float InputVariable::getInput() const
{
	if (device)
	{
		float value = device->getInput(type, id);

		if (useRange)
			value = (value - minv) / (maxv - minv);

		return value;
	}

	return 0;
}

bool InputVariable::getInputBool() const
{
	return fabsf(getInput()) > 0.0001f;
}

}
