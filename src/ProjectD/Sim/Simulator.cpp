#include "Sim/Simulator.h"
#include "Physics/PhysicsFactory.h"
#include "Sim/Track.h"
#include "Car/Car.h"
#include "Car/CarState.h"
#include "Core/SharedMemory.h"

namespace D {

Simulator::Simulator()
{
	TRACE_CTOR(Simulator);
}

Simulator::~Simulator()
{
	TRACE_DTOR(Simulator);

	unloadTrack();
}

bool Simulator::init(const std::wstring& _basePath)
{
	log_printf(L"Simulator: init: basePath=\"%s\"", _basePath.c_str());
	GUARD_FATAL(osDirExists(_basePath));

	basePath = _basePath;
	replace(basePath, L'\\', L'/');
	if (!ends_with(basePath, L'/'))
		basePath.append(L"/");

	maxCars = 1;
	roadTemperature = 20.0;
	ambientTemperature = 20.0;

	fuelConsumptionRate = 0.0f;
	tyreConsumptionRate = 0.0f;
	mechanicalDamageRate = 1.0f;
	allowTyreBlankets = false;
	isEngineStallEnabled = true;

	//ffGyroWheelGain = 0;
	ffGyroWheelGain = 0.004f;
	ffFlatSpotGain = 0.05f;
	ffDamperGain = 1.0f;
	ffDamperMinValue = 0;
	mzLowSpeedReduction.speedKMH = 3.0f;
	mzLowSpeedReduction.minValue = 0.01f;

	interopEnabled = 0;
	interopSyncState = 0;
	interopSyncInput = 0;

	auto ini(std::make_unique<INIReader>(basePath + L"cfg/sim.ini"));
	if (ini->ready)
	{
		ini->tryGetInt(L"SIM", L"MAX_CARS", maxCars);
		maxCars = tclamp(maxCars, 1, 100);

		ini->tryGetFloat(L"ENVIRONMENT", L"ROAD_TEMP", roadTemperature);
		ini->tryGetFloat(L"ENVIRONMENT", L"AMBIENT_TEMP", ambientTemperature);

		ini->tryGetInt(L"INTEROP", L"ENABLED", interopEnabled);
		ini->tryGetInt(L"INTEROP", L"SYNC_STATE", interopSyncState);
		ini->tryGetInt(L"INTEROP", L"SYNC_INPUT", interopSyncInput);
	}

	dynamicTemp.baseRoad = roadTemperature;
	dynamicTemp.baseAir = ambientTemperature;

	physics = PhysicsFactory::createPhysicsEngine();
	physics->setCollisionCallback(this);

	if (interopEnabled)
	{
		interopState.reset(new SharedMemory());
		interopInput.reset(new SharedMemory());

		const size_t stateSize = (sizeof(SimInteropHeader) + (sizeof(CarState) * maxCars));
		const size_t inputSize = (sizeof(SimInteropHeader) + (sizeof(CarControls) * maxCars));

		interopState->allocate(L"Local\\projectd_state", stateSize);
		interopInput->allocate(L"Local\\projectd_input", inputSize);
	}

	log_printf(L"Simulator: init: DONE");
	return true;
}

Track* Simulator::loadTrack(const std::wstring& trackName)
{
	log_printf(L"Simulator: loadTrack: trackName=\"%s\"", trackName.c_str());

	track = std::make_shared<Track>(this);
	track->init(trackName);

	return track.get();
}

void Simulator::unloadTrack()
{
	log_printf(L"Simulator: unloadTrack");

	cars.clear();
	carMap.clear();

	track.reset();
}

Car* Simulator::addCar(const std::wstring& modelName)
{
	log_printf(L"Simulator: addCar: modelName=\"%s\"", modelName.c_str());

	if (!track)
	{
		log_printf(L"addCar failed: track is null");
		return nullptr;
	}

	auto car = std::make_shared<Car>(track.get());
	
	int carId = 0;
	if (!freeCarIds.empty())
	{
		carId = freeCarIds.back();
		freeCarIds.pop_back();
	}
	else
	{
		carId = carIdGenerator++;
	}

	car->physicsGUID = carId;
	car->init(modelName);

	auto* rawCar = car.get();
	carMap.insert({car->physicsGUID, car});
	cars.push_back(rawCar);

	return rawCar;
}

Car* Simulator::getCar(int carId)
{
	auto iter = carMap.find(carId);
	if (iter != carMap.end())
	{
		return iter->second.get();
	}
	return nullptr;
}

template<typename T>
inline void removeItem(std::vector<T>& container, const T& value) { container.erase(std::remove(container.begin(), container.end(), value), container.end()); }

void Simulator::removeCar(int carId)
{
	log_printf(L"Simulator: removeCar: carId=%d", carId);

	auto iter = carMap.find(carId);
	if (iter != carMap.end())
	{
		removeItem(cars, iter->second.get());
		carMap.erase(iter);
		freeCarIds.push_back(carId);
	}
}

void Simulator::step(float dt, double physicsTime, double gameTime)
{
	if (!track)
		return;

	deltaTime = dt;
	physicsTime = physicsTime;
	gameTime = gameTime;
	stepCounter++;

	readInteropInputs();

	evOnPreStep.fire(dt);

	track->step(dt);
	stepWind(dt);
	stepCars(dt);
	physics->step(dt);

	evOnStepCompleted.fire(dt);

	// should be called after evOnStepCompleted
	updateInteropState();

	if (dbgCollisions.size() > 500)
		dbgCollisions.clear();
}

void Simulator::stepWind(float dt)
{
	#if 0

	if (wind.speed.value >= 0.01f)
	{
		float fChange = (float)sin((physicsTime - sessionInfo.startTimeMS) * 0.0001);
		float fSpeed = wind.speed.value * ((fChange * 0.1f) + 1.0f);
		vec3f vWind = wind.vector.get_norm() * fSpeed;

		if (isfinite(vWind.x) && isfinite(vWind.y) && isfinite(vWind.z))
		{
			wind.vector = vWind;
		}
		else
		{
			SHOULD_NOT_REACH_WARN;
		}
	}

	#endif
}

void Simulator::stepCars(float dt)
{
	for (auto* pCar : cars)
	{
		pCar->stepPreCacheValues(dt);
	}

	for (auto* pCar : cars)
	{
		pCar->step(dt);
	}
}

void Simulator::readInteropInputs() // sim is consumer
{
	if (!interopInput || !interopInput->isValid())
		return;

	auto* sharedData = (uint8_t*)interopInput->data();
	size_t pos = 0;

	auto* header = (SimInteropHeader*)sharedData; pos += sizeof(SimInteropHeader);
	const auto producerId = header->producerId;

	// check if new frame available
	if (interopSyncInput && (header->consumerId == producerId))
		return;

	const int actualCars = tmin((int)cars.size(), (int)maxCars);
	const int numCars =  tclamp((int)header->numCars, 0, actualCars);

	for (int carId = 0; carId < numCars; ++carId)
	{
		auto* pCar = cars[carId];

		memcpy(&pCar->controls, sharedData + pos, sizeof(CarControls)); pos += sizeof(CarControls);
		pCar->externalControls = true;
	}

	_mm_sfence();

	header->consumerId = producerId;
}

void Simulator::updateInteropState() // sim is producer
{
	if (!interopState || !interopState->isValid())
		return;

	auto* sharedData = (uint8_t*)interopState->data();
	size_t pos = 0;

	auto* header = (SimInteropHeader*)sharedData; pos += sizeof(SimInteropHeader);
	const auto consumerId = header->consumerId;

	// check if consumer received last frame
	if (interopSyncState && (header->producerId != consumerId))
		return;

	const int numCars = tmin((int)cars.size(), (int)maxCars);
	header->numCars = (int32_t)numCars;

	for (int carId = 0; carId < numCars; ++carId)
	{
		auto* pCar = cars[carId];

		memcpy(sharedData + pos, pCar->state.get(), sizeof(CarState)); pos += sizeof(CarState);
	}

	_mm_sfence();

	++header->producerId;
}

void Simulator::onCollisionCallback(
	IRigidBody* rb0, ICollisionObject* shape0, 
	IRigidBody* rb1, ICollisionObject* shape1, 
	const vec3f& normal, const vec3f& pos, float depth)
{
	bool bFlag0 = false;
	bool bFlag1 = false;

	dbgCollisions.push_back({pos, normal, depth, (float)physicsTime});

	for (auto* pCar : cars) // TODO: check
	{
		if (pCar->body.get() == rb0)
			bFlag0 = true;

		if (pCar->body.get() == rb1)
			bFlag1 = true;
	}

	if (!bFlag0 && bFlag1)
	{
		std::swap(rb0, rb1);
		std::swap(shape0, shape1);
	}

	for (auto* pCar : cars)
	{
		pCar->onCollisionCallback(rb0, shape0, rb1, shape1, normal, pos, depth);
	}
}

void Simulator::setDynamicTempData(const DynamicTempData& data)
{
	dynamicTemp = data;
}

void Simulator::setWind(Speed speed, float directionDEG)
{
	vec3f vAxis(0, 1, 0);
	mat44f mxWind = mat44f::createFromAxisAngle(vAxis, -(directionDEG * 0.017453f));

	wind.vector = vec3f(&mxWind.M31) * speed.value;
	wind.speed = speed;
	wind.directionDeg = directionDEG;
}

float Simulator::getAirDensity() const
{
	return 1.2922f - (ambientTemperature * 0.0041f);
}

}
