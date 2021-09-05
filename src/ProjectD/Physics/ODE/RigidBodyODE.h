#pragma once

#include "Physics/ODE/CollisionMeshODE.h"

namespace D {

struct RigidBodyODE : public IRigidBody, PhysicsChildODE
{
	RigidBodyODE(PhysicsEngineODEPtr core);
	~RigidBodyODE();

	void setEnabled(bool value) override;
	bool isEnabled() override;
	void setAutoDisable(bool mode) override;
	void stop() override;

	void setMassBox(float m, float x, float y, float z) override;
	float getMass() override;
	void setMassExplicitInertia(float totalMass, float x, float y, float z) override;
	vec3f getLocalInertia() override;

	vec3f localToWorld(const vec3f& p) override;
	vec3f worldToLocal(const vec3f& p) override;
	vec3f localToWorldNormal(const vec3f& p) override;
	vec3f worldToLocalNormal(const vec3f& p) override;

	void setPosition(const vec3f& pos) override;
	vec3f getPosition(float interpolationT) override;
	void setRotation(const mat44f& mat) override;
	mat44f getWorldMatrix(float interpolationT) override;

	void setVelocity(const vec3f& vel) override;
	vec3f getVelocity() override;
	vec3f getLocalVelocity() override;
	vec3f getPointVelocity(const vec3f& p) override;
	vec3f getLocalPointVelocity(const vec3f& p) override;

	void setAngularVelocity(const vec3f& vel) override;
	vec3f getAngularVelocity() override;
	vec3f getLocalAngularVelocity() override;
	
	void addForceAtPos(const vec3f& f, const vec3f& p) override;
	void addForceAtLocalPos(const vec3f& f, const vec3f& p) override;
	void addLocalForce(const vec3f& f) override;
	void addLocalForceAtPos(const vec3f& f, const vec3f& p) override;
	void addLocalForceAtLocalPos(const vec3f& f, const vec3f& p) override;
	void addTorque(const vec3f& t) override;
	void addLocalTorque(const vec3f& t) override;
	
	void addBoxCollider(const vec3f& pos, const vec3f& size, unsigned int spaceId, unsigned int category, unsigned long collideMask) override;
	void addMeshCollider(ITriMeshPtr trimesh, const mat44f& offset, unsigned int spaceId, unsigned long category, unsigned long collideMask) override;

	dxBody* id = nullptr;
	std::vector<dxGeom*> geoms;
	std::vector<std::shared_ptr<CollisionMeshODE>> collisionMeshes;
};

DECL_SHARED_PTR(RigidBodyODE);

}
