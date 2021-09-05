#include "Car/BrushTyreModel.h"

namespace D {

BrushTyreModel::BrushTyreModel()
{}

BrushTyreModel::~BrushTyreModel()
{}

BrushOutput BrushTyreModel::solveV5(float slip, float load, float asy)
{
	BrushOutput result;

	float v8 = ((((1.0f / ((((load - data.Fz0) / data.Fz0) * (data.maxSlip1 - data.maxSlip0)) + data.maxSlip0)) * 3.0f) * 78.125f) * 2.0f) * 0.0064f;
	float v9 = 1.0f / (v8 * 0.3333333f);

	float fNewSlip = slip / v9;
	result.slip = fNewSlip;

	if (slip > v9)
		result.force = ((1.0f - asy) / (((slip - v9) * data.falloffSpeed) + 1.0f)) + asy;
	else
		result.force = (((1.0f - fNewSlip) * (1.0f - fNewSlip)) * (v8 * slip)) + ((3.0f - (fNewSlip * 2.0f)) * (fNewSlip * fNewSlip));

	return result;
}

float BrushTyreModel::getCFFromSlipAngle(float angle)
{
	return ((1.0f / tanf(angle * 0.017453f)) * 3.0f) * 78.125f;
}

}
