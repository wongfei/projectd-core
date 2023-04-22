#pragma once

#include "Car/CarCommon.h"
#include "Car/ICarAudioRenderer.h"

namespace D {

struct FmodContext;
struct Car;

struct SmoothValue
{
	float alpha = 60;
	float value = 0;
};

struct FmodAudioRenderer : public ICarAudioRenderer
{
	FmodAudioRenderer(std::shared_ptr<FmodContext> context, const std::string& basePath, Car* car);
	~FmodAudioRenderer();

	void update(float dt) override;

	std::shared_ptr<FmodContext> context;
	Car* car = nullptr;
	void* evEngineInt = nullptr;
	void* evEngineExt = nullptr;
	void* evTurbo = nullptr;
	void* evTransmission = nullptr;
	void* evSkidInt = nullptr;
	SmoothValue skidVolumes[4];
	SmoothValue skidPitches[4];
};

}
