#pragma once

#include "Input/IGameController.h"
#include <string>

namespace D {

struct InputVariable
{
	void bind(IGameController* device, EInputType type, int id);
	void setRange(float r0, float r1);
	float getInput() const;
	bool getInputBool() const;

	std::wstring name;
	EInputType type = EInputType(0);
	IGameController* device = nullptr;
	int id = 0;

	float minv = 0;
	float maxv = 0;
	bool useRange = false;
};

}
