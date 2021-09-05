#pragma once

#include "Physics/ODE/PhysicsEngineODE.h"

namespace D {

struct CollisionMeshODE : public ICollisionObject, PhysicsChildODE
{
	CollisionMeshODE(
		PhysicsEngineODEPtr core, ITriMeshPtr trimesh, 
		dxSpace* space, unsigned long category, unsigned long collideMask);
	~CollisionMeshODE();

	void setUserPointer(void* data) override;
	void* getUserPointer() override;
	unsigned long getGroup() override;
	unsigned long getMask() override;

	ITriMeshPtr trimesh;
	dxTriMeshData* geomTrimesh = nullptr;
	dxGeom* geom = nullptr;
	void* userPointer = nullptr;
};

}
