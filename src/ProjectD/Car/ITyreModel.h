#pragma once

#include "Car/CarCommon.h"

namespace D {

struct TyreModelInput // orig
{
	float load = 0;
	float slipAngleRAD = 0;
	float slipRatio = 0;
	float camberRAD = 0;
	float speed = 0;
	float u = 0;
	int tyreIndex = 0;
	float cpLength = 0;
	float grain = 0;
	float blister = 0;
	float pressureRatio = 0;
	bool useSimpleModel = false;
};

struct TyreModelOutput // orig
{
	float Fy = 0;
	float Fx = 0;
	float Mz = 0;
	float trail = 0;
	float ndSlip = 0;
	float Dy = 0;
	float Dx = 0;
};

struct ITyreModel : public virtual IObject
{
	virtual TyreModelOutput solve(const TyreModelInput& tmi) = 0;
};

}
