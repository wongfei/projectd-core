#pragma once

#include "Physics/ITriMesh.h"

namespace D {

struct IRigidBody : public virtual IObject
{
	virtual void setEnabled(bool value) = 0;
	virtual bool isEnabled() = 0;
	virtual void setAutoDisable(bool mode) = 0;
	virtual void stop() = 0;

	virtual void setMassBox(float m, float x, float y, float z) = 0;
	virtual float getMass() = 0;
	virtual void setMassExplicitInertia(float totalMass, float x, float y, float z) = 0;
	virtual vec3f getLocalInertia() = 0;

	virtual vec3f localToWorld(const vec3f& p) = 0;
	virtual vec3f worldToLocal(const vec3f& p) = 0;
	virtual vec3f localToWorldNormal(const vec3f& p) = 0;
	virtual vec3f worldToLocalNormal(const vec3f& p) = 0;

	virtual void setPosition(const vec3f& pos) = 0;
	virtual vec3f getPosition(float interpolationT) = 0;
	virtual void setRotation(const mat44f& mat) = 0;
	virtual mat44f getWorldMatrix(float interpolationT) = 0;

	virtual void setVelocity(const vec3f& vel) = 0;
	virtual vec3f getVelocity() = 0;
	virtual vec3f getLocalVelocity() = 0;
	virtual vec3f getPointVelocity(const vec3f& p) = 0;
	virtual vec3f getLocalPointVelocity(const vec3f& p) = 0;

	virtual void setAngularVelocity(const vec3f& vel) = 0;
	virtual vec3f getAngularVelocity() = 0;
	virtual vec3f getLocalAngularVelocity() = 0;
	
	virtual void addForceAtPos(const vec3f& f, const vec3f& p) = 0;
	virtual void addForceAtLocalPos(const vec3f& f, const vec3f& p) = 0;
	virtual void addLocalForce(const vec3f& f) = 0;
	virtual void addLocalForceAtPos(const vec3f& f, const vec3f& p) = 0;
	virtual void addLocalForceAtLocalPos(const vec3f& f, const vec3f& p) = 0;
	virtual void addTorque(const vec3f& t) = 0;
	virtual void addLocalTorque(const vec3f& t) = 0;
	
	virtual void addBoxCollider(const vec3f& pos, const vec3f& size, unsigned int spaceId, unsigned int category, unsigned long collideMask) = 0;
	virtual void addMeshCollider(ITriMeshPtr trimesh, const mat44f& offset, unsigned int spaceId, unsigned long category, unsigned long collideMask) = 0;
};

DECL_SHARED_PTR(IRigidBody);

}
