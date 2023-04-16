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

struct SimInteropHeader
{
	volatile uint64_t producerId = 0;
	volatile uint64_t consumerId = 0;
	volatile int32_t numCars = 0;
};

struct Simulator : public virtual ICollisionCallback // std::enable_shared_from_this<Simulator>
{
	Simulator();
	~Simulator();

	bool init(const std::wstring& basePath);

	Track* loadTrack(const std::wstring& trackName);
	void unloadTrack();

	Car* addCar(const std::wstring& modelName);
	void removeCar(unsigned int carId);

	void step(float dt, double physicsTime, double gameTime);

	// ICollisionCallback
	void onCollisionCallback(
		IRigidBody* rb0, ICollisionObject* shape0, 
		IRigidBody* rb1, ICollisionObject* shape1, 
		const vec3f& normal, const vec3f& pos, float depth) override;

	void setDynamicTempData(const DynamicTempData& data);
	void setWind(Speed speed, float directionDEG);
	float getAirDensity() const;

	// internals

	void stepWind(float dt);
	void stepCars(float dt);

	Event<double> evOnPreStep;
	Event<double> evOnStepCompleted;

	void readInteropInputs();
	void updateInteropState();

	// config

	std::wstring basePath;
	int maxCars = 0;

	float roadTemperature = 0;
	float ambientTemperature = 0;
	DynamicTempData dynamicTemp;
	Wind wind;

	float fuelConsumptionRate = 0;
	float tyreConsumptionRate = 0;
	float mechanicalDamageRate = 0;
	bool allowTyreBlankets = 0;
	bool isEngineStallEnabled = 0;

	float ffGyroWheelGain = 0;
	float ffFlatSpotGain = 0;
	float ffDamperGain = 0;
	float ffDamperMinValue = 0;
	SteerMzLowSpeedReduction mzLowSpeedReduction;

	int interopEnabled = 0;
	int interopSyncState = 0;
	int interopSyncInput = 0;

	// runtime

	IPhysicsEnginePtr physics;
	std::vector<CollisionData> dbgCollisions;

	TrackPtr track;
	std::vector<CarPtr> cars;
	std::vector<SlipStream*> slipStreams; // owned by cars

	std::unique_ptr<struct SharedMemory> interopState;
	std::unique_ptr<struct SharedMemory> interopInput;

	float deltaTime = 0;
	double physicsTime = 0;
	double gameTime = 0;
	unsigned int stepCounter = 0;
};

}
