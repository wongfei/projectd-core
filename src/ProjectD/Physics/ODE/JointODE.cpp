#include "Physics/ODE/JointODE.h"

namespace D {

JointODE::JointODE(PhysicsEngineODEPtr _core, RigidBodyODEPtr _rb1, RigidBodyODEPtr _rb2)
:
	PhysicsChildODE(_core), 
	rb1(_rb1), 
	rb2(_rb2)
{
	//TRACE_CTOR(JointODE);
}

JointODE::~JointODE()
{
	//TRACE_DTOR(JointODE);

	ODE_CALL(dJointDestroy)(id);
}

FixedJointODE::FixedJointODE(PhysicsEngineODEPtr _core, RigidBodyODEPtr _rb1, RigidBodyODEPtr _rb2) 
:
	JointODE(_core, _rb1, _rb2)
{
	id = ODE_CALL(dJointCreateFixed)(core->world, 0);
	ODE_CALL(dJointAttach)(id, rb1->id, rb2->id);
	ODE_CALL(dJointSetFixed)(id);
}

BallJointODE::BallJointODE(PhysicsEngineODEPtr _core, RigidBodyODEPtr _rb1, RigidBodyODEPtr _rb2, const vec3f& pos) 
:
	JointODE(_core, _rb1, _rb2)
{
	id = ODE_CALL(dJointCreateBall)(core->world, 0);
	ODE_CALL(dJointAttach)(id, rb1->id, rb2->id);
	ODE_CALL(dJointSetBallAnchor)(id, ODE_V3(pos));
}

SliderJointODE::SliderJointODE(PhysicsEngineODEPtr _core, RigidBodyODEPtr _rb1, RigidBodyODEPtr _rb2, const vec3f& axis) 
:
	JointODE(_core, _rb1, _rb2)
{
	id = ODE_CALL(dJointCreateSlider)(core->world, 0);
	ODE_CALL(dJointAttach)(id, rb1->id, rb2->id);
	ODE_CALL(dJointSetSliderAxis)(id, ODE_V3(axis));
}

DistanceJointODE::DistanceJointODE(PhysicsEngineODEPtr _core, RigidBodyODEPtr _rb1, RigidBodyODEPtr _rb2, const vec3f& p1, const vec3f& p2) 
:
	JointODE(_core, _rb1, _rb2)
{
	id = ODE_CALL(dJointCreateDBall)(core->world, 0);
	ODE_CALL(dJointAttach)(id, rb1->id, rb2->id);
	ODE_CALL(dJointSetDBallAnchor1)(id, ODE_V3(p1));
	ODE_CALL(dJointSetDBallAnchor2)(id, ODE_V3(p2));
	distance = ODE_CALL(dJointGetDBallDistance)(id);
}

void SliderJointODE::setERPCFM(float erp, float cfm)
{
	if (erp > 0.0f)
		ODE_CALL(dJointSetSliderParam)(id, 13, erp);

	if (cfm > 0.0f)
		ODE_CALL(dJointSetSliderParam)(id, 8, cfm);
}

void DistanceJointODE::setERPCFM(float erp, float cfm)
{
	if (erp > 0.0f)
		ODE_CALL(dJointSetDBallParam)(id, 13, erp);

	if (cfm > 0.0f)
		ODE_CALL(dJointSetDBallParam)(id, 8, cfm);
}

void DistanceJointODE::reseatDistanceJointLocal(const vec3f& p1, const vec3f& p2)
{
	auto* b1 = ODE_CALL(dJointGetBody)(id, 0);
	auto* b2 = ODE_CALL(dJointGetBody)(id, 1);
	dVector3 rel1;
	dVector3 rel2;
	ODE_CALL(dBodyGetRelPointPos)(b1, ODE_V3(p1), rel1);
	ODE_CALL(dBodyGetRelPointPos)(b2, ODE_V3(p2), rel2);
	ODE_CALL(dJointSetDBallAnchor1)(id, rel1[0], rel1[1], rel1[2]);
	ODE_CALL(dJointSetDBallAnchor2)(id, rel2[0], rel2[1], rel2[2]);
	ODE_CALL(dJointSetDBallDistance)(id, distance);
}

}
