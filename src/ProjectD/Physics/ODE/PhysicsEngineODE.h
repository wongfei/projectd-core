#pragma once

#include "Physics/IPhysicsEngine.h"
#include <vector>
#include <map>
#include <ode/ode.h>

#if !defined(dSINGLE)
#error "Invalid ODE configuration"
#endif

#define ODE_CALL(func) func
#define ODE_V3(v) v.x, v.y, v.z

namespace D {

struct PhysicsEngineODE : public IPhysicsEngine, std::enable_shared_from_this<PhysicsEngineODE>
{
	PhysicsEngineODE();
	~PhysicsEngineODE();

	// IPhysicsEngine

	IRigidBodyPtr createRigidBody() override;
	IJointPtr createFixedJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2) override;
	IJointPtr createBallJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& pos) override;
	IJointPtr createSliderJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& axis) override;
	IJointPtr createDistanceJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& p1, const vec3f& p2) override;
	IJointPtr createBumpJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& p1, float rangeUp, float rangeDn) override;

	ITriMeshPtr createTriMesh() override;
	ICollisionObjectPtr createCollider(ITriMeshPtr trimesh, bool isDynamic, unsigned int spaceId, unsigned long category, unsigned long collideMask) override;
	IRayCasterPtr createRayCaster(float length) override;

	void setCollisionCallback(ICollisionCallback* callback) override;
	RayCastHit rayCast(const vec3f& pos, const vec3f& dir, float length) override;
	RayCastHit rayCast(const vec3f& pos, const vec3f& dir, IRayCasterPtr ray) override;

	void step(float dt) override;

	// Internals

	dxSpace* getStaticSubSpace(unsigned int index);
	dxSpace* getDynamicSubSpace(unsigned int index);
	RayCastHit rayCastImpl(const vec3f& pos, const vec3f& dir, dxGeom* dxray);

	void collisionStep(float dt);
	void onCollision(dContactGeom* contacts, int numContacts, dxGeom* o1, dxGeom* o2);

	dxWorld* world = nullptr;
	dxSpace* spaceStatic = nullptr;
	dxSpace* spaceDynamic = nullptr;
	dxJointGroup* contactGroup = nullptr;
	dxJointGroup* contactGroupDynamic = nullptr;
	dxJointGroup* currentContactGroup = nullptr;
	dxGeom* ray = nullptr;
	ICollisionCallback* collisionCallback = nullptr;
	std::map<unsigned int, dxSpace*> staticSubSpaces;
	std::map<unsigned int, dxSpace*> dynamicSubSpaces;
	unsigned int currentFrame = 0;
	int noCollisionCounter = 0;
};

DECL_SHARED_PTR(PhysicsEngineODE);

struct PhysicsChildODE
{
	inline PhysicsChildODE(PhysicsEngineODEPtr _core) : core(_core) {}

	PhysicsEngineODEPtr core;
};

}
