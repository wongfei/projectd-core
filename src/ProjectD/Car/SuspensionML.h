#pragma once

#include "Car/SuspensionBase.h"

namespace D {

struct MLBall
{
	vec3f relToTyre;
	vec3f relToCar;
};

struct MLJoint
{
	MLBall ballCar;
	MLBall ballTyre;
	IJointPtr joint;
};

struct SuspensionML : public SuspensionBase
{
	SuspensionML();
	~SuspensionML();

	void init(IPhysicsEnginePtr core, IRigidBodyPtr carBody, int index, const std::wstring& dataPath);
	
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
	std::vector<MLJoint> joints;
	SuspensionDamage damageData;
	vec3f basePosition;
	vec3f baseCarSteerPosition;
	float hubMass = 0;

	// runtime
	IRigidBodyPtr hub = nullptr;
	float steerTorque = 0;
};

}
