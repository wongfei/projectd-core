#pragma once

#include "Car/SuspensionBase.h"

namespace D {

struct SuspensionDataStrut
{
	vec3f carStrut;
	vec3f tyreStrut;
	vec3f carBottomWB_F;
	vec3f carBottomWB_R;
	vec3f tyreBottomWB;
	vec3f carSteer;
	vec3f tyreSteer;
	vec3f refPoint;
	vec3f hubInertiaBox;
	float hubMass = 0;
};

struct SuspensionStrut : public SuspensionBase
{
	SuspensionStrut();
	~SuspensionStrut();

	void init(IPhysicsEnginePtr core, IRigidBodyPtr carBody, int index, const std::wstring& dataPath);
	void setPositions();
	
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
	SuspensionDamage damageData;
	SuspensionDataStrut dataRelToWheel;
	SuspensionDataStrut dataRelToBody;
	vec3f basePosition;
	vec3f baseCarSteerPosition;
	float strutBodyLength = 0;

	// runtime
	IRigidBodyPtr hub;
	IRigidBodyPtr strutBody;
	IJointPtr bumpStopJoint;
	IJointPtr joints[5];
	float steerLinkBaseLength = 0;
	float strutBaseLength = 0;
	float steerTorque = 0;
	float steerAngle = 0;
};

}
