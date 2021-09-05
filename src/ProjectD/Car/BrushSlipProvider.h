#pragma once

#include "Car/CarCommon.h"

namespace D {

struct TyreSlipInput
{
	float slip = 0;
	float load = 0;
};

struct TyreSlipOutput
{
	float normalizedForce = 0;
	float slip = 0;
};

struct BrushSlipProvider : public NonCopyable
{
	BrushSlipProvider();
	BrushSlipProvider(float maxAngle, float flex);
	~BrushSlipProvider();
	TyreSlipOutput getSlipForce(TyreSlipInput& input, bool useasy);
	void calcMaximum(float load, float& outMaxForce, float& outMaxSlip);
	void recomputeMaximum();

	// config
	int version = 5;
	float asy = 1;

	// runtime
	std::unique_ptr<BrushTyreModel> brushModel;
	float maxForce = 0;
	float maxSlip = 0;
};

}
