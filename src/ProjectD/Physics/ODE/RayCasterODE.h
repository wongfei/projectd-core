#pragma once

#include "Physics/ODE/PhysicsEngineODE.h"

namespace D {

struct RayCasterODE : public IRayCaster, PhysicsChildODE
{
	RayCasterODE(PhysicsEngineODEPtr core, float length);
	~RayCasterODE();

	RayCastHit rayCast(const vec3f& pos, const vec3f& dir) override;

	dxGeom* id = nullptr;
};

}
