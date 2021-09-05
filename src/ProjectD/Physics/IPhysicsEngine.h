#pragma once

#include "Physics/IRigidBody.h"
#include "Physics/IJoint.h"
#include "Physics/IRayCaster.h"
#include "Physics/ICollisionCallback.h"

namespace D {

struct IPhysicsEngine : public virtual IObject
{
	virtual IRigidBodyPtr createRigidBody() = 0;
	virtual IJointPtr createFixedJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2) = 0;
	virtual IJointPtr createBallJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& pos) = 0;
	virtual IJointPtr createSliderJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& axis) = 0;
	virtual IJointPtr createDistanceJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& p1, const vec3f& p2) = 0;
	virtual IJointPtr createBumpJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& p1, float rangeUp, float rangeDn) = 0;

	virtual ITriMeshPtr createTriMesh() = 0;
	virtual ICollisionObjectPtr createCollider(ITriMeshPtr trimesh, bool isDynamic, unsigned int spaceId, unsigned long category, unsigned long collideMask) = 0;
	virtual IRayCasterPtr createRayCaster(float length) = 0;

	virtual void setCollisionCallback(ICollisionCallback* callback) = 0;
	virtual RayCastHit rayCast(const vec3f& pos, const vec3f& dir, float length) = 0;
	virtual RayCastHit rayCast(const vec3f& pos, const vec3f& dir, IRayCasterPtr ray) = 0;

	virtual void step(float dt) = 0;
};

DECL_SHARED_PTR(IPhysicsEngine);

}
