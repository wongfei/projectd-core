#include "Car/TyreModel.h"

namespace D {

SCTM::SCTM()
{}

SCTM::~SCTM()
{}

TyreModelOutput SCTM::solve(const TyreModelInput& tmi) // TODO: cleanup
{
	TyreModelOutput tmo;

	if (tmi.load <= 0.0f || tmi.slipAngleRAD == 0.0f && tmi.slipRatio == 0.0f && tmi.camberRAD == 0.0f)
	{
		return tmo;
	}

	float fAsy = asy;
	if (tmi.useSimpleModel)
		asy = 1.0f;

	float fSlipAngle = tmi.slipAngleRAD;
	float fUnk1 = (sinf(tmi.camberRAD) * camberGain) + fSlipAngle;
	float fUnk1Tan = tanf(fUnk1);
	float fSlipAngleSin = sinf(fSlipAngle);

	float fBlister1 = tclamp(tmi.blister * 0.01f, 0.0f, 1.0f);
	float fBlister2 = (fBlister1 * 0.2f) + 1.0f;

	float fStaticDy = getStaticDY(tmi.load);
	float fStaticDx = getStaticDX(tmi.load);

	float fUDy = tmi.u * fStaticDy / fBlister2;
	float fUDx = tmi.u * fStaticDx / fBlister2;

	if (tmi.slipRatio < 0.0f)
		fUDx = fUDx * brakeDXMod;

	float fCamberRad = tmi.camberRAD;
	float fCamberRadTmp = fabsf(fCamberRad);

	if ((fCamberRad < 0.0f || fUnk1 < 0.0f) && (fCamberRad > 0.0f || fUnk1 > 0.0f))
		fCamberRadTmp = -fCamberRadTmp;

	fCamberRadTmp = -fCamberRadTmp;

	if (dCamberCurve.getCount())
	{
		float fCamberDeg = fCamberRadTmp * 57.29578f;

		if (useSmoothDCamberCurve)
			fUDy *= dCamberCurve.getCubicSplineValue(fCamberDeg);
		else
			fUDy *= dCamberCurve.getValue(fCamberDeg);
	}
	else
	{
		float fCamberUnk = (fCamberRadTmp * dcamber0) - ((fCamberRadTmp * fCamberRadTmp) * dcamber1);
		if (fCamberUnk <= -1.0f)
			fCamberUnk = -0.8999999f;

		fUDy += (((fUDy / (fCamberUnk + 1.0f)) - fUDy) * dCamberBlend);
	}

	float fSlipRatio = tmi.slipRatio;
	float fSlipAngleCos = cosf(tmi.slipAngleRAD);
	float fSlipRatioClamped = (fSlipRatio > -0.9999999f ? fSlipRatio : -0.9999999f); // TODO: ???

	float fSpeed = tmi.speed;
	float a = fSpeed * fSlipAngleSin;
	float b = (fSpeed * fSlipRatio) * fSlipAngleCos;
	float fUnk2 = sqrtf((a * a) + (b * b));
	float fUnk2Scaled = fUnk2 * speedSensitivity;

	float fDy = fUDy / (fUnk2Scaled + 1.0f);
	float fDx = fUDx / (fUnk2Scaled + 1.0f);

	float fLoadSubFz0 = tmi.load - Fz0;
	float fPCfGain = pressureCfGain;

	float fCF = ((((1.0f / ((((fLoadSubFz0 / Fz0) * (maxSlip1 - maxSlip0)) + maxSlip0) * (((tmi.u - 1.0f) * 0.75f) + 1.0f))) * 3.0f) * 78.125f) / ((tmi.grain * 0.01f) + 1.0f)) * ((fPCfGain * tmi.pressureRatio) + 1.0f);

	float fUnk3 = fSlipRatio / (fSlipRatioClamped + 1.0f);
	float fUnk4 = fUnk1Tan / (fSlipRatioClamped + 1.0f);
	float fSlip;

	float fCombFactor = combinedFactor;
	if (fCombFactor <= 0.0f || fCombFactor == 2.0f) // TODO: check
	{
		fSlip = sqrtf((fUnk4 * fUnk4) + (fUnk3 * fUnk3));
	}
	else
	{
		float fUnk34Comb = powf(fabsf(fUnk4), fCombFactor) + powf(fabsf(fUnk3), fCombFactor);
		fSlip = powf(fUnk34Comb, 1.0f / fCombFactor);
	}

	float fPureFyDx = getPureFY(fDx, fCF * cfXmult, tmi.load, fSlip) * fDx;
	float fPureFyDy = getPureFY(fDy, fCF, tmi.load, fSlip);

	tmo.Fy = ((fPureFyDy * fDy) * (fUnk4 / fSlip)) * tmi.load;
	tmo.Fx = ((fUnk3 / fSlip) * fPureFyDx) * tmi.load;

	float fNdSlip = fSlip / (1.0f / (((fCF * 2.0f) * 0.0064f) / 3.0f));
	float fUnk5 = tclamp((1.0f - (fNdSlip * 0.8f)), 0.0f, 1.0f);
	float fUnk6 = (((((3.0f - (fUnk5 * 2.0f)) * (fUnk5 * fUnk5)) * 1.1f) - 0.1f) * tmi.cpLength) * 0.12f;

	tmo.Mz = -(fUnk6 * tmo.Fy);
	tmo.trail = fUnk6 * tclamp(tmi.speed, 0.0f, 1.0f);
	tmo.ndSlip = fNdSlip;
	tmo.Dy = fDy;
	tmo.Dx = fDx;

	asy = fAsy;

	return tmo;
}

float SCTM::getStaticDX(float load)
{
	if (dxLoadCurve.getCount() <= 0)
	{
		if (load != 0.0)
			return (powf(load, lsExpX) * lsMultX) / load;
	}
	else
	{
		return dxLoadCurve.getCubicSplineValue(load);
	}
	return 0;
}

float SCTM::getStaticDY(float load)
{
	if (dyLoadCurve.getCount() <= 0)
	{
		if (load != 0.0f)
			return (powf(load, lsExpY) * lsMultY) / load;
	}
	else
	{
		return dyLoadCurve.getCubicSplineValue(load);
	}
	return 0;
}

float SCTM::getPureFY(float D, float cf, float load, float slip)
{
	float v5 = (cf * 2.0f) * 0.0064f;
	float v6 = 1.0f / (v5 / 3.0f);
	float fy;

	if (v6 < slip)
		fy = ((1.0f / (((slip - v6) * falloffSpeed) + 1.0f)) * (1.0f - asy)) + asy;
	else
		fy = (((1.0f - (slip / v6)) * (1.0f - (slip / v6))) * (v5 * slip)) + ((3.0f - ((slip / v6) * 2.0f)) * ((slip / v6) * (slip / v6)));

	return fy;
}

}
