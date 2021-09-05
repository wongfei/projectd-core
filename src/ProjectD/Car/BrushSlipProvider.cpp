#include "Car/BrushSlipProvider.h"
#include "Car/BrushTyreModel.h"

namespace D {

BrushSlipProvider::BrushSlipProvider()
{
	brushModel.reset(new BrushTyreModel());
}

BrushSlipProvider::BrushSlipProvider(float maxAngle, float flex)
{
	brushModel.reset(new BrushTyreModel());
	brushModel->data.CF = brushModel->getCFFromSlipAngle(maxAngle);
	brushModel->data.CF1 = flex * -50000.0f;
	recomputeMaximum();
}

BrushSlipProvider::~BrushSlipProvider()
{
}

TyreSlipOutput BrushSlipProvider::getSlipForce(TyreSlipInput &input, bool useasy)
{
	BrushOutput bo = brushModel->solveV5(input.slip, input.load, (useasy ? asy : 1.0f));

	TyreSlipOutput tso;
	tso.normalizedForce = bo.force;
	tso.slip = bo.slip;

	return tso;
}

void BrushSlipProvider::calcMaximum(float load, float& outMaxForce, float& outMaxSlip)
{
	outMaxForce = 0;
	outMaxSlip = 0;

	float fLoad = (load >= 0.0f) ? load : 2000.0f;
	float fSlip = 0;

	do
	{
		BrushOutput bo = brushModel->solveV5(fSlip, fLoad, asy);

		if (bo.force > outMaxForce)
		{
			outMaxForce = bo.force;
			outMaxSlip = fSlip;
		}

		fSlip += 0.001f;
	}
	while (fSlip < 1.0f);
}

void BrushSlipProvider::recomputeMaximum()
{
	calcMaximum(2000.0f, maxForce, maxSlip);
}

}
