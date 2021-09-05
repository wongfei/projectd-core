#pragma once

#include "Physics/ODE/RigidBodyODE.h"

namespace D {

struct JointODE : public IJoint, PhysicsChildODE
{
	JointODE(PhysicsEngineODEPtr core, RigidBodyODEPtr rb1, RigidBodyODEPtr rb2);
	~JointODE();

	RigidBodyODEPtr rb1;
	RigidBodyODEPtr rb2;
	dJointID id = nullptr;
};

struct FixedJointODE : public JointODE
{
	FixedJointODE(PhysicsEngineODEPtr core, RigidBodyODEPtr rb1, RigidBodyODEPtr rb2);
};

struct BallJointODE : public JointODE
{
	BallJointODE(PhysicsEngineODEPtr core, RigidBodyODEPtr rb1, RigidBodyODEPtr rb2, const vec3f& pos);
};

struct SliderJointODE : public JointODE
{
	SliderJointODE(PhysicsEngineODEPtr core, RigidBodyODEPtr rb1, RigidBodyODEPtr rb2, const vec3f& axis);

	void setERPCFM(float erp, float cfm) override;
};

struct DistanceJointODE : public JointODE
{
	DistanceJointODE(PhysicsEngineODEPtr core, RigidBodyODEPtr rb1, RigidBodyODEPtr rb2, const vec3f& p1, const vec3f& p2);

	void setERPCFM(float erp, float cfm) override;
	void reseatDistanceJointLocal(const vec3f& p1, const vec3f& p2) override;

	float distance;
};

}
