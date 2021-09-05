#pragma once

#include "Car/TyreCompound.h"
#include "Car/TyreStatus.h"
#include "Sim/ITrackRayCastProvider.h"
#include "Core/SignalGenerator.h"
#include <functional>

namespace D {

struct TyreInputs
{
	float brakeTorque = 0;
	float handBrakeTorque = 0;
	float electricTorque = 0;
};

struct TyreExternalInputs
{
	float load = 0;
	float slipAngle = 0;
	float slipRatio = 0;
	bool isActive = false;
};

struct Tyre : public NonCopyable
{
	Tyre();
	~Tyre();
	void init(Car* car, ISuspension* hub, ITrackRayCastProvider* rayCastProvider, int index, const std::wstring& dataPath);
	void initCompounds(const std::wstring& dataPath);
	void setCompound(int cindex);
	void reset();

	void step(float dt);
	void addGroundContact(const vec3f& pos, const vec3f& normal);
	void updateLockedState(float dt);
	void updateAngularSpeed(float dt);
	void stepRotationMatrix(float dt);
	void stepThermalModel(float dt);
	void stepTyreBlankets(float dt);
	void stepGrainBlister(float dt, float hubVelocity);
	void stepFlatSpot(float dt, float hubVelocity);

	void addTyreForcesV10(const vec3f& pos, const vec3f& normal, Surface* pSurface, float dt);
	float getCorrectedD(float d, float* outWearMult);
	void stepDirtyLevel(float dt, float hubSpeed);
	void stepPuncture(float dt, float hubSpeed);
	void addTyreForceToHub(const vec3f& pos, const vec3f& force);

	// utils
	float getDX(float load);

	std::function<void (void)> onStepCompleted;

	// config
	std::vector<std::unique_ptr<TyreCompound>> compoundDefs;
	int index = 0;
	int currentCompoundIndex = 0;
	float aiMult = 1.0f;
	float flatSpotK = 0.15f;
	float explosionTemperature = 350.0f;
	float blanketTemperature = 80.0f;
	float pressureTemperatureGain = 0.16f;
	bool driven = false;
	bool tyreBlanketsOn = false;
	bool useLoadForVKM = false;

	// runtime
	Car* car = nullptr;
	ISuspension* hub = nullptr;
	ITrackRayCastProvider* rayCastProvider = nullptr;

	IRayCasterPtr rayCaster;
	Surface* surfaceDef = nullptr;

	std::unique_ptr<SCTM> tyreModel;
	std::unique_ptr<TyreThermalModel> thermalModel;

	TyreData data;
	TyreModelData modelData;
	TyreInputs inputs;
	TyreExternalInputs externalInputs;
	TyreStatus status;
	SinSignalGenerator shakeGenerator;
	
	mat44f worldRotation;
	mat44f localWheelRotation;
	vec3f worldPosition;
	vec3f roadHeading;
	vec3f roadRight;
	vec3f unmodifiedContactPoint;
	vec3f contactPoint;
	vec3f contactNormal;

	float absOverride = 1.0f;
	float slidingVelocityX = 0;
	float slidingVelocityY = 0;
	float rSlidingVelocityX = 0;
	float rSlidingVelocityY = 0;
	float roadVelocityX = 0;
	float roadVelocityY = 0;
	float totalHubVelocity = 0;
	float totalSlideVelocity = 0;
	float oldAngularVelocity = 0;
	float localMX = 0;
};

}
