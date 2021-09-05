#pragma once

#include "Car/SuspensionBase.h"

namespace D {

enum class RigidAxleSide
{
	Left = 0x0,
	Right = 0x1,
};

struct AxleBall
{
	vec3f relToAxle;
	vec3f relToCar;
	IJointPtr joint;
};

struct AxleJoint
{
	AxleBall ballCar;
	AxleBall ballAxle;
};

struct SuspensionAxle : public SuspensionBase
{
	SuspensionAxle();
	~SuspensionAxle();

	void init(IPhysicsEnginePtr core, IRigidBodyPtr carBody, IRigidBodyPtr axle, int index, RigidAxleSide side, const std::wstring& dataPath);

	// ISuspension
	void attach() override;
	void stop() override;
	void step(float dt) override;
	void addForceAtPos(const vec3f& force, const vec3f& pos, bool driven, bool addToSteerTorque) override;
	void addLocalForceAndTorque(const vec3f& force, const vec3f& torque, const vec3f& driveTorque) override;
	void addTorque(const vec3f& torque) override;
	void setERPCFM(float erp, float cfm) override;
	void setSteerLengthOffset(float o) override;
	void getSteerBasis(vec3f& centre, vec3f& axis) override;
	mat44f getHubWorldMatrix() override;
	vec3f getBasePosition() override;
	vec3f getVelocity() override;
	vec3f getPointVelocity(const vec3f& p) override;
	vec3f getHubAngularVelocity() override;
	float getMass() override;
	float getSteerTorque() override;
	void getDebugState(CarDebug* state) override;

	// config
	std::vector<AxleJoint> joints;
	RigidAxleSide side = RigidAxleSide(0);
	float track = 0;
	float referenceY = 0;
	float attachRelativePos = 0;
	vec3f axleBasePos;
	vec3f leafSpringK;

	// runtime
	IRigidBodyPtr axle;
};

}
