#pragma once

#include "Car/ITyreModel.h"

namespace D {

struct SCTM : public ITyreModel
{
	SCTM();
	~SCTM();

	TyreModelOutput solve(const TyreModelInput& tmi) override;
	float getStaticDX(float load);
	float getStaticDY(float load);
	float getPureFY(float D, float cf, float load, float slip);

	// orig
	float lsMultY = 0;
	float lsExpY = 0;
	float lsMultX = 0;
	float lsExpX = 0;
	float Fz0 = 0;
	float maxSlip0 = 0;
	float maxSlip1 = 0;
	float asy = 0;
	float falloffSpeed = 0;
	float speedSensitivity = 0;
	float camberGain = 0;
	float dcamber0 = 0;
	float dcamber1 = 0;
	float cfXmult = 1.0f;
	Curve dyLoadCurve;
	Curve dxLoadCurve;
	float pressureCfGain = 0.1f;
	float brakeDXMod = 1.0f;
	Curve dCamberCurve;
	bool useSmoothDCamberCurve = false;
	float dCamberBlend = 1.0f;
	float combinedFactor = 2.0f;

	// TODO: not used???
	float dy0 = 0;
	float dx0 = 0;
	float pacE = 0;
	float pacCf = 0;
	float pacFlex = 0;
};

}
