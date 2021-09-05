#include "Car/TyreThermalModel.h"
#include "Car/Car.h"
#include "Sim/Simulator.h"

namespace D {

TyreThermalModel::TyreThermalModel()
{}

TyreThermalModel::~TyreThermalModel()
{}

void TyreThermalModel::init(Car* _car, int _elements, int _stripes)
{
	car = _car;
	elements = _elements;
	stripes = _stripes;
	coreTemp = car->sim->ambientTemperature;
	buildTyre(); 
}

inline void connect(TyreThermalPatch& p1, TyreThermalPatch& p2)
{
	p1.connections.emplace_back(&p2);
	p2.connections.emplace_back(&p1);
}

void TyreThermalModel::buildTyre()
{
	float t = car->sim->ambientTemperature;

	patches.resize(stripes * elements);
	for (int i = 0; i < stripes; ++i)
	{
		for (int j = 0; j < elements; ++j)
		{
			auto& patch = getPatchAt(i, j);
			patch.T = t;
			patch.inputT = t;
			patch.stripeIndex = i;
			patch.elementIndex = j;

			if (i + 1 < stripes)
			{
				connect(patch, getPatchAt(i + 1, j));
			}

			if (j + 1 < elements)
			{
				connect(patch, getPatchAt(i, j + 1));
			}
			else if (j + 1 == elements) // loop
			{
				connect(patch, getPatchAt(i, 0));
			}
		}
	}
}

void TyreThermalModel::step(float dt, float angularSpeed, float camberRAD)
{
	float fPhase = (float)phase + (angularSpeed * dt);
	if (fPhase > 100000.0) fPhase -= 100000.0;
	else if (fPhase < 0.0) fPhase += 100000.0;
	phase = fPhase;

	float fAmbientTemp = car->sim->ambientTemperature;
	float fCoreTempInput = tmax(fAmbientTemp, coreTInput);
	coreTemp += ((fCoreTempInput - coreTemp) * (patchData.internalCoreTransfer * dt));
	coreTInput = 0;

	float fSpeed = car ? car->speed.ms() : 17.32f; // sqrt(300)
	float fAmbientFactor = ((((fSpeed * fSpeed) * patchData.coolFactorGain) + 1.0f) * patchData.surfaceTransfer) * dt;
	float fPctDt = patchData.patchCoreTransfer * dt;

	if (car)
	{
		for (auto& patch : patches)
		{
			float fInputT = patch.inputT;
			float fPatchT = patch.T;

			if (fInputT <= fAmbientTemp)
				fPatchT += ((fAmbientTemp - fPatchT) * fAmbientFactor);
			else
				fPatchT += ((fInputT - fPatchT) * (patchData.surfaceTransfer * dt));

			for (auto* conn : patch.connections)
			{
				fPatchT += (conn->T - fPatchT) * (patchData.patchTransfer * dt);
			}

			fPatchT += (coreTemp - fPatchT) * fPctDt;
			patch.T = fPatchT;
			patch.inputT = 0;

			coreTemp += ((fPatchT - coreTemp) * fPctDt);
		}
	}

	if (isActive)
	{
		if (performanceCurve.getCount() > 0)
		{
			float fPracT = ((getCurrentCPTemp(camberRAD) - coreTemp) * 0.25f) + coreTemp;
			practicalTemp = fPracT;
			thermalMultD = performanceCurve.getValue(fPracT);
		}
	}
}

TyreThermalPatch& TyreThermalModel::getPatchAt(int x, int y)
{
	if (x >= 0 && x < stripes && y >= 0 && y < elements)
		return patches[y + x * elements];

	SHOULD_NOT_REACH_WARN;
	return patches[0];
}

float TyreThermalModel::getCorrectedD(float d, float camberRAD)
{
	if (isActive)
		return d * thermalMultD;
	return d;
}

void TyreThermalModel::getIMO(float* pfOut)
{
	for (int i = 0; i < 3; ++i)
	{
		float fSum = 0;
		for (int j = 0; j < 12; ++j)
		{
			auto& patch = patches[j + i * elements];
			fSum += patch.T;
		}
		*pfOut++ = fSum / 12.0f;
	}
}

void TyreThermalModel::addThermalCoreInput(float temp)
{
	coreTInput += temp;
}

void TyreThermalModel::addThermalInput(float xpos, float pressureRel, float temp)
{
	float fNormXcs = tclamp((xpos * camberSpreadK), -1.0f, 1.0f);

	float fPhase = (float)(phase * 0.1591549430964443);
	int iElemY = ((int)(fPhase * elements)) % elements;

	float fT = (car ? car->sim->roadTemperature : 26.0f) + temp;
	float fPr1 = pressureRel * 0.1f;
	float fPr2 = (pressureRel * -0.5f) + 1.0f;

	auto& patch0 = getPatchAt(0, iElemY);
	patch0.inputT += ((((fNormXcs + 1.0f) - (fPr1 * 0.5f)) * fPr2) * fT);

	auto& patch1 = getPatchAt(1, iElemY);
	patch1.inputT += (((fPr1 + 1.0f) * fPr2) * fT);

	auto& patch2 = getPatchAt(2, iElemY);
	patch2.inputT += ((((1.0f - fNormXcs) - (fPr1 * 0.5f)) * fPr2) * fT);
}

float TyreThermalModel::getCurrentCPTemp(float camber)
{
	float fNormCsk = tclamp((camber * camberSpreadK), -1.0f, 1.0f);

	float fPhase = (float)(phase * 0.1591549430964443);
	int iElemY = ((int)(fPhase * elements)) % elements;

	auto& patch0 = getPatchAt(0, iElemY);
	auto& patch1 = getPatchAt(1, iElemY);
	auto& patch2 = getPatchAt(2, iElemY);

	return ((((fNormCsk + 1.0f) * patch0.T) + patch1.T) + ((1.0f - fNormCsk) * patch2.T)) * 0.33333334f;
}

float TyreThermalModel::getPracticalTemp(float camberRAD)
{
	return ((getCurrentCPTemp(camberRAD) - coreTemp) * 0.25f) + coreTemp;
}

float TyreThermalModel::getAvgSurfaceTemp()
{
	float fSum = 0;
	for (auto& patch : patches)
	{
		fSum += patch.T;
	}
	return fSum / 36.0f;
}

void TyreThermalModel::setTemperature(float optimumTemp)
{
	coreTemp = optimumTemp;
	for (auto& patch : patches)
	{
		patch.T = optimumTemp;
	}
}

void TyreThermalModel::reset()
{
	setTemperature(car->sim->ambientTemperature);
	phase = 0;
}

}
