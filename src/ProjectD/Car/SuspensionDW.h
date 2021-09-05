#pragma once

#include "Car/SuspensionBase.h"
#include "Core/PIDController.h"

namespace D {

struct SuspensionDataDW
{
	vec3f carTopWB_F;
	vec3f carTopWB_R;
	vec3f carBottomWB_F;
	vec3f carBottomWB_R;
	vec3f tyreTopWB;
	vec3f tyreBottomWB;
	vec3f carSteer;
	vec3f tyreSteer;
	vec3f refPoint;
	vec3f hubInertiaBox;
	float hubMass = 0;
};

struct ActiveActuator
{
	PIDController pid;
	float targetTravel = 0;
	float eval(float dt, float travel) { return pid.eval(targetTravel, travel, dt); }
};

struct SuspensionDW : public SuspensionBase
{
	SuspensionDW();
	~SuspensionDW();

	void init(IPhysicsEnginePtr core, IRigidBodyPtr carBody, AntirollBar* arb1, AntirollBar* arb2, int index, const std::wstring& dataPath);
	
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
	SuspensionDataDW dataRelToWheel;
	SuspensionDataDW dataRelToBody;
	vec3f basePosition;
	vec3f baseCarSteerPosition;
	float steerLinkBaseLength = 0;
	bool useActiveActuator = false; // TODO: ???

	// runtime
	IRigidBodyPtr hub;
	IJointPtr bumpStopJoint;
	IJointPtr joints[5];
	AntirollBar* arb[2] = {};
	ActiveActuator activeActuator;
	float steerTorque = 0;
	float steerAngle = 0; // TODO: ???
};

}
