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
	FmodAudioRenderer(Car* car, const std::wstring& basePath, const std::wstring& carModel);
	~FmodAudioRenderer();

	void update(float dt) override;

	std::unique_ptr<FmodContext> context;
	Car* car = nullptr;
	void* evEngineInt = nullptr;
	void* evSkidInt = nullptr;
	SmoothValue skidVolumes[4];
	SmoothValue skidPitches[4];
};

}
