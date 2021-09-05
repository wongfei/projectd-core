#pragma once

#include "Car/TyreCompound.h"

namespace D {

struct TyreThermalPatch
{
	std::vector<TyreThermalPatch*> connections;
	int elementIndex = 0;
	int stripeIndex = 0;
	float inputT = 0;
	float T = 0;
};

struct TyreThermalModel : public NonCopyable
{
	TyreThermalModel();
	~TyreThermalModel();
	void init(Car* car, int elements, int stripes);
	void buildTyre();
	void step(float dt, float angularSpeed, float camberRAD);
	TyreThermalPatch& getPatchAt(int x, int y);
	float getCorrectedD(float d, float camberRAD);
	void getIMO(float* pfOut);
	void addThermalCoreInput(float temp);
	void addThermalInput(float xpos, float pressureRel, float temp);
	float getCurrentCPTemp(float camber);
	float getPracticalTemp(float camberRAD);
	float getAvgSurfaceTemp();
	void setTemperature(float optimumTemp);
	void reset();

	// config
	bool isActive = true;
	float camberSpreadK = 1.4f;
	TyrePatchData patchData;
	Curve performanceCurve;

	// runtime
	Car* car = nullptr;
	std::vector<TyreThermalPatch> patches;
	int elements = 0;
	int stripes = 0;
	double phase = 0;
	float coreTInput = 0;
	float coreTemp = 0;
	float practicalTemp = 0;
	float thermalMultD = 1.0f;
};

}
