#pragma once

#include "Car/CarCommon.h"

namespace D {

struct BrushTyreModelData
{
	float CF = 1200.0f;
	float xu = 0; // legacy
	float CF1 = -10.0f;
	float Fz0 = 2000.0f;
	float maxSlip0 = 0.2f;
	float maxSlip1 = 0.4f;
	float falloffSpeed = 2.0f;
};

struct BrushOutput
{
	float force = 0;
	float slip = 0;
};

struct BrushTyreModel : public NonCopyable
{
	BrushTyreModel();
	~BrushTyreModel();
	BrushOutput solveV5(float slip, float load, float asy);
	float getCFFromSlipAngle(float angle);

	// config
	BrushTyreModelData data;
};

}
