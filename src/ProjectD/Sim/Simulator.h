#pragma once

#include "Sim/SlipStream.h"
#include "Core/Event.h"

namespace D {

struct DynamicTempData
{
	Curve temperatureCurve;
	double temperatureStartTime = 0;
	float baseRoad = 0;
	float baseAir = 0;
};

struct Wind
{
	vec3f vector;
	Speed speed;
	float directionDeg = 0;
};

struct SteerMzLowSpeedReduction
{
	float speedKMH = 0;
	float minValue = 0;
};

struct CollisionData
{
	vec3f pos;
	vec3f normal;
	float depth;
	float ts;
};

struct Simulator : public virtual ICollisionCallback, std::enable_shared_from_this<Simulator>
{
	Simulator();
	~Simulator();

	bool init(const std::wstring& basePath);
	TrackPtr initTrack(const std::wstring& trackName);
	CarPtr initCar(TrackPtr track, const std::wstring& modelName);
	void step(float dt, double physicsTime, double gameTime);
	
	void readInteropInputs();
	void updateInteropState();

	// ICollisionCallback
	void onCollisionCallback(
		IRigidBody* rb0, ICollisionObject* shape0, 
		IRigidBody* rb1, ICollisionObject* shape1, 
		const vec3f& normal, const vec3f& pos, float depth) override;

	void setDynamicTempData(const DynamicTempData& data);
	void setWind(Speed speed, float directionDEG);
	float getAirDensity() const;

	// IMPL

	void unregisterTrack(Track* obj);
	void unregisterCar(Car* obj);

	void stepWind(float dt);
	void stepCars(float dt);

	Event<double> evOnPreStep;
	Event<double> evOnStepCompleted;

	// config
	std::wstring basePath;
	int interopEnabled = 0;
	int interopMaxCars = 0;

	// objects
	IPhysicsEnginePtr physics;
	std::vector<CollisionData> collisions;
	std::vector<CollisionData> collisions2;

	// weak
	Track* track = nullptr;
	std::vector<Car*> cars;
	std::vector<SlipStream*> slipStreams;

	// shared memory
	std::unique_ptr<struct SharedMemory> sharedState;
	std::unique_ptr<struct SharedMemory> sharedInputs;

	// step
	float deltaTime = 0;
	double physicsTime = 0;
	double gameTime = 0;
	unsigned int stepCounter = 0;

	// environment
	float roadTemperature = 0;
	float ambientTemperature = 0;
	DynamicTempData dynamicTemp;
	Wind wind;

	// realism
	float fuelConsumptionRate = 0;
	float tyreConsumptionRate = 0;
	float mechanicalDamageRate = 0;
	bool allowTyreBlankets = 0;
	bool isEngineStallEnabled = 0;

	// force feedback
	float ffGyroWheelGain = 0;
	float ffFlatSpotGain = 0;
	float ffDamperGain = 0;
	float ffDamperMinValue = 0;
	SteerMzLowSpeedReduction mzLowSpeedReduction;
};

}
