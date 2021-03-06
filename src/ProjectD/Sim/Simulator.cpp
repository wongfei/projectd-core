#include "Sim/Simulator.h"
#include "Physics/PhysicsFactory.h"
#include "Sim/Track.h"
#include "Car/Car.h"

namespace D {

Simulator::Simulator()
{
	TRACE_CTOR(Simulator);
}

Simulator::~Simulator()
{
	TRACE_DTOR(Simulator);
}

bool Simulator::init(const std::wstring& _basePath)
{
	log_printf(L"Simulator: init: basePath=\"%s\"", _basePath.c_str());
	basePath = _basePath;

	float defT = 20.0f;
	roadTemperature = defT;
	ambientTemperature = defT;
	dynamicTemp.baseRoad = defT;
	dynamicTemp.baseAir = defT;

	fuelConsumptionRate = 1.0f;
	tyreConsumptionRate = 1.0f;
	mechanicalDamageRate = 1.0f;
	allowTyreBlankets = false;
	isEngineStallEnabled = true;

	ffGyroWheelGain = 0; // 0.004f
	ffFlatSpotGain = 0.05f;
	ffDamperGain = 1.0f;
	ffDamperMinValue = 0;
	mzLowSpeedReduction.speedKMH = 3.0f;
	mzLowSpeedReduction.minValue = 0.01f;

	physics = PhysicsFactory::createPhysicsEngine();
	physics->setCollisionCallback(this);

	return true;
}

TrackPtr Simulator::initTrack(const std::wstring& trackName)
{
	auto newTrack = std::make_shared<Track>(shared_from_this());
	newTrack->init(trackName);

	// weak
	track = newTrack.get();

	return newTrack;
}

void Simulator::unregisterTrack(Track* obj)
{
	if (track == obj)
		track = nullptr;
}

CarPtr Simulator::initCar(TrackPtr _track, const std::wstring& modelName)
{
	auto newCar = std::make_shared<Car>(_track);
	newCar->init(modelName);

	// weak
	cars.emplace_back(newCar.get());
	slipStreams.emplace_back(newCar->slipStream.get());

	return newCar;
}

template<typename T>
inline void remove_item(std::vector<T>& container, const T& value) { container.erase(std::remove(container.begin(), container.end(), value), container.end()); }

void Simulator::unregisterCar(Car* obj)
{
	remove_item(cars, obj);
	remove_item(slipStreams, obj->slipStream.get());
}

void Simulator::step(float dt, double physicsTime, double gameTime)
{
	if (!track)
		return;

	this->deltaTime = dt;
	this->physicsTime = physicsTime;
	this->gameTime = gameTime;
	this->stepCounter++;

	evOnPreStep.fire(dt);

	track->step(dt);
	stepWind(dt);
	stepCars(dt);
	physics->step(dt);

	evOnStepCompleted.fire(dt);

	if (collisions.size() > 100)
		collisions.clear();
}

void Simulator::stepWind(float dt)
{
	#if 0

	if (this->wind.speed.value >= 0.01f)
	{
		float fChange = (float)sin((this->physicsTime - this->sessionInfo.startTimeMS) * 0.0001);
		float fSpeed = this->wind.speed.value * ((fChange * 0.1f) + 1.0f);
		vec3f vWind = this->wind.vector.get_norm() * fSpeed;

		if (isfinite(vWind.x) && isfinite(vWind.y) && isfinite(vWind.z))
		{
			this->wind.vector = vWind;
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
	for (auto* pCar : this->cars)
	{
		pCar->stepPreCacheValues(dt);
	}

	for (auto* pCar : this->cars)
	{
		pCar->step(dt);
	}
}

void Simulator::onCollisionCallback(
	IRigidBody* rb0, ICollisionObject* shape0, 
	IRigidBody* rb1, ICollisionObject* shape1, 
	const vec3f& normal, const vec3f& pos, float depth)
{
	bool bFlag0 = false;
	bool bFlag1 = false;

	collisions.push_back({pos, normal, depth, (float)physicsTime});

	for (auto* pCar : this->cars) // TODO: check
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

	for (auto* pCar : this->cars)
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
	return 1.2922f - (this->ambientTemperature * 0.0041f);
}

}
