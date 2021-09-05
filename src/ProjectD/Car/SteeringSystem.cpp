#include "Car/SteeringSystem.h"
#include "Car/Car.h"

namespace D {

SteeringSystem::SteeringSystem()
{}

SteeringSystem::~SteeringSystem()
{}

void SteeringSystem::init(Car* _car)
{
	car = _car;
}

void SteeringSystem::step(float dt)
{
	float steer = -car->finalSteerAngleSignal * linearRatio;
	car->suspensions[0]->setSteerLengthOffset(steer);
	car->suspensions[1]->setSteerLengthOffset(steer);

	// TODO: ctrl4ws
}

}
