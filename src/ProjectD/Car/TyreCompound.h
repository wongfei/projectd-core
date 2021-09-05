#pragma once

#include "Car/CarCommon.h"

namespace D {

struct TyreData // orig
{
	float width = 0.15f;
	float radius = 0.3f;
	float k = 220000.0f;
	float d = 400.0f;
	float angularInertia = 1.6f;
	float thermalFrictionK = 0.03f;
	float thermalRollingK = 0.5f;
	float thermalRollingSurfaceK = 0;
	float grainThreshold = 0;
	float blisterThreshold = 9000.0f;
	float grainGamma = 1.0f;
	float blisterGamma = 1.0f;
	float grainGain = 0;
	float blisterGain = 0;
	float rimRadius = 0;
	float optimumTemp = 80.0f;
	float softnessIndex = 0;
	float radiusRaiseK = 0;
};

struct TyreModelData // orig
{
	int version = 0;
	float Dy0 = 1.4f;
	float Dy1 = -0.145f;
	float Dx0 = 1.68f;
	float Dx1 = -0.0145f;
	float Fz0 = 2000.0f;
	float flexK = 0.005f;
	float speedSensitivity = 0.003f;
	float relaxationLength = 0.8f;
	float rr0 = 500.0f;
	float rr1 = 0.00745f;
	float rr_sa = 0;
	float rr_sr = 0;
	float rr_slip = 0;
	float camberGain = 0;
	float pressureSpringGain = 20000.0f;
	float pressureFlexGain = 1.0f;
	float pressureRRGain = 0.5f;
	float pressureGainD = 0;
	float idealPressure = 0;
	float pressureRef = 26.0f;
	Curve wearCurve;
	float dcamber0 = 0.1f;
	float dcamber1 = -0.8f;
	Curve dyLoadCurve;
	Curve dxLoadCurve;
	float lsMultY = 0;
	float lsExpY = 0;
	float lsMultX = 0;
	float lsExpX = 0;
	float maxWearKM = 0;
	float maxWearMult = 0;
	float asy = 1.0f;
	float cfXmult = 1.0f;
	float brakeDXMod = 1.0f;
	Curve dCamberCurve;
	bool useSmoothDCamberCurve = 0;
	float combinedFactor = 0;
};

struct TyrePatchData // orig
{
	float surfaceTransfer = 0.3f;
	float patchTransfer = 0.2f;
	float patchCoreTransfer = 0.2f;
	float internalCoreTransfer = 0.004f;
	float coolFactorGain = 0;
};

struct TyreCompound : public NonCopyable
{
	unsigned int index = 0;
	std::wstring name;
	std::wstring shortName;
	TyreData data;
	TyreModelData modelData;
	TyrePatchData thermalPatchData;
	Curve thermalPerformanceCurve;
	float pressureStatic = 0;
	std::unique_ptr<BrushSlipProvider> slipProvider;
};

}
