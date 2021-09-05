#pragma once

#include "Car/CarCommon.h"

namespace D {

struct CarDebug;

enum class SuspensionType
{
	DoubleWishbone = 0x0,
	Strut = 0x1,
	Axle = 0x2,
	Multilink = 0x3,
};

struct SuspensionStatus
{
	float travel = 0;
	float damperSpeedMS = 0;
};

struct ISuspension : public virtual IObject
{
	virtual void attach() = 0;
	virtual void stop() = 0;
	virtual void step(float dt) = 0;
	virtual SuspensionType getType() const = 0;
	virtual SuspensionStatus getStatus() const = 0;
	virtual void addForceAtPos(const vec3f& force, const vec3f& pos, bool driven, bool addToSteerTorque) = 0;
	virtual void addLocalForceAndTorque(const vec3f& force, const vec3f& torque, const vec3f& driveTorque) = 0;
	virtual void addTorque(const vec3f& torque) = 0;
	virtual void setERPCFM(float erp, float cfm) = 0;
	virtual void setSteerLengthOffset(float o) = 0;
	virtual void getSteerBasis(vec3f& centre, vec3f& axis) = 0;
	virtual mat44f getHubWorldMatrix() = 0;
	virtual vec3f getBasePosition() = 0;
	virtual vec3f getVelocity() = 0;
	virtual vec3f getPointVelocity(const vec3f& p) = 0;
	virtual vec3f getHubAngularVelocity() = 0;
	virtual float getMass() = 0;
	virtual float getSteerTorque() = 0;
	virtual void getDebugState(CarDebug* state) = 0;
};

}
