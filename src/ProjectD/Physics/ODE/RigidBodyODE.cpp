#include "Physics/ODE/RigidBodyODE.h"

namespace D {

RigidBodyODE::RigidBodyODE(PhysicsEngineODEPtr pcore)
:
	PhysicsChildODE(pcore)
{
	//TRACE_CTOR(RigidBodyODE);

	id = ODE_CALL(dBodyCreate)(core->world);
	GUARD_FATAL(id);

	ODE_CALL(dBodySetData)(id, static_cast<IRigidBody*>(this));
	ODE_CALL(dBodySetFiniteRotationMode)(id, 1);
	ODE_CALL(dBodySetFiniteRotationAxis)(id, 0, 0, 0);
	ODE_CALL(dBodySetLinearDamping)(id, 0);
	ODE_CALL(dBodySetAngularDamping)(id, 0);
}

RigidBodyODE::~RigidBodyODE()
{
	//TRACE_DTOR(RigidBodyODE);

	for (auto& g : geoms)
		dGeomDestroy(g);
	geoms.clear();

	collisionMeshes.clear();

	ODE_CALL(dBodyDestroy)(id);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RigidBodyODE::setEnabled(bool value)
{
	if (value)
		ODE_CALL(dBodyEnable)(id);
	else
		ODE_CALL(dBodyDisable)(id);
}

bool RigidBodyODE::isEnabled()
{
	return ODE_CALL(dBodyIsEnabled)(id) == 1;
}

void RigidBodyODE::setAutoDisable(bool mode)
{
	ODE_CALL(dBodySetAutoDisableFlag)(id, mode ? 1 : 0);
}

void RigidBodyODE::stop()
{
	ODE_CALL(dBodySetLinearVel)(id, 0, 0, 0);
	ODE_CALL(dBodySetAngularVel)(id, 0, 0, 0);
	ODE_CALL(dBodySetForce)(id, 0, 0, 0);
	ODE_CALL(dBodySetTorque)(id, 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RigidBodyODE::setMassBox(float m, float x, float y, float z)
{
	dMass d;
	ODE_CALL(dMassSetZero)(&d);
	ODE_CALL(dMassSetBoxTotal)(&d, m, x, y, z);
	ODE_CALL(dBodySetMass)(id, &d);
}

float RigidBodyODE::getMass()
{
	dMass d;
	ODE_CALL(dMassSetZero)(&d);
	ODE_CALL(dBodyGetMass)(id, &d);
	return d.mass;
}

void RigidBodyODE::setMassExplicitInertia(float totalMass, float x, float y, float z)
{
	dMass d;
	ODE_CALL(dMassSetZero)(&d);
	d.mass = totalMass;
	d.I[0] = x; // TODO: why not [0] [5] [10]?
	d.I[4] = y;
	d.I[8] = z;
	ODE_CALL(dBodySetMass)(id, &d);
}

vec3f RigidBodyODE::getLocalInertia()
{
	dMass d;
	ODE_CALL(dMassSetZero)(&d);
	ODE_CALL(dBodyGetMass)(id, &d);
	return vec3f(d.I[0], d.I[5], d.I[10]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

vec3f RigidBodyODE::localToWorld(const vec3f& p)
{
	dVector3 res;
	ODE_CALL(dBodyGetRelPointPos)(id, ODE_V3(p), res);
	return vec3f(res);
}

vec3f RigidBodyODE::worldToLocal(const vec3f& p)
{
	dVector3 res;
	ODE_CALL(dBodyGetPosRelPoint)(id, ODE_V3(p), res);
	return vec3f(res);
}

vec3f RigidBodyODE::localToWorldNormal(const vec3f& p)
{
	dVector3 res;
	ODE_CALL(dBodyVectorToWorld)(id, ODE_V3(p), res);
	return vec3f(res);
}

vec3f RigidBodyODE::worldToLocalNormal(const vec3f& p)
{
	dVector3 res;
	ODE_CALL(dBodyVectorFromWorld)(id, ODE_V3(p), res);
	return vec3f(res);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RigidBodyODE::setPosition(const vec3f& pos)
{
	ODE_CALL(dBodySetPosition)(id, ODE_V3(pos));
}

vec3f RigidBodyODE::getPosition(float interpolationT)
{
	const dReal* res = ODE_CALL(dBodyGetPosition)(id);
	return vec3f(res);
}

void RigidBodyODE::setRotation(const mat44f& mat)
{
	dMatrix3 r;
	ODE_CALL(dRSetIdentity)(r);
	r[0] = mat.M11;
	r[1] = mat.M21;
	r[2] = mat.M31;
	r[4] = mat.M12;
	r[5] = mat.M22;
	r[6] = mat.M32;
	r[8] = mat.M13;
	r[9] = mat.M23;
	r[10] = mat.M33;
	ODE_CALL(dBodySetRotation)(id, r);
}

mat44f RigidBodyODE::getWorldMatrix(float interpolationT)
{
	const dReal* r = ODE_CALL(dBodyGetRotation)(id);
	const dReal* p = ODE_CALL(dBodyGetPosition)(id);
	mat44f m;
	m.M11 = r[0];
	m.M12 = r[4];
	m.M13 = r[8];
	m.M14 = 0;
	m.M21 = r[1];
	m.M22 = r[5];
	m.M23 = r[9];
	m.M24 = 0;
	m.M31 = r[2];
	m.M32 = r[6];
	m.M33 = r[10];
	m.M34 = 0;
	m.M41 = p[0];
	m.M42 = p[1];
	m.M43 = p[2];
	m.M44 = 1.0;
	return m;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RigidBodyODE::setVelocity(const vec3f& vel)
{
	ODE_CALL(dBodySetLinearVel)(id, ODE_V3(vel));
}

vec3f RigidBodyODE::getVelocity()
{
	dVector3 res;
	ODE_CALL(dBodyGetRelPointVel)(id, 0, 0, 0, res);
	return vec3f(res);
}

vec3f RigidBodyODE::getLocalVelocity()
{
	return worldToLocalNormal(getVelocity());
}

vec3f RigidBodyODE::getPointVelocity(const vec3f& p)
{
	dVector3 res;
	ODE_CALL(dBodyGetPointVel)(id, ODE_V3(p), res);
	return vec3f(res);
}

vec3f RigidBodyODE::getLocalPointVelocity(const vec3f& p)
{
	dVector3 res;
	ODE_CALL(dBodyGetRelPointVel)(id, ODE_V3(p), res);
	return vec3f(res);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RigidBodyODE::setAngularVelocity(const vec3f& vel)
{
	ODE_CALL(dBodySetAngularVel)(id, ODE_V3(vel));
}

vec3f RigidBodyODE::getAngularVelocity()
{
	const dReal* res = ODE_CALL(dBodyGetAngularVel)(id);
	return vec3f(res);
}

vec3f RigidBodyODE::getLocalAngularVelocity()
{
	return worldToLocalNormal(getAngularVelocity());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RigidBodyODE::addForceAtPos(const vec3f& f, const vec3f& p)
{
	ODE_CALL(dBodyAddForceAtPos)(id, ODE_V3(f), ODE_V3(p));
}

void RigidBodyODE::addForceAtLocalPos(const vec3f& f, const vec3f& p)
{
	ODE_CALL(dBodyAddForceAtRelPos)(id, ODE_V3(f), ODE_V3(p));
}

void RigidBodyODE::addLocalForce(const vec3f& f)
{
	ODE_CALL(dBodyAddRelForceAtRelPos)(id, ODE_V3(f), 0, 0, 0);
}

void RigidBodyODE::addLocalForceAtPos(const vec3f& f, const vec3f& p)
{
	ODE_CALL(dBodyAddRelForceAtPos)(id, ODE_V3(f), ODE_V3(p));
}

void RigidBodyODE::addLocalForceAtLocalPos(const vec3f& f, const vec3f& p)
{
	ODE_CALL(dBodyAddRelForceAtRelPos)(id, ODE_V3(f), ODE_V3(p));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RigidBodyODE::addTorque(const vec3f& t)
{
	ODE_CALL(dBodyAddTorque)(id,  ODE_V3(t));
}

void RigidBodyODE::addLocalTorque(const vec3f& t)
{
	ODE_CALL(dBodyAddRelTorque)(id,  ODE_V3(t));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RigidBodyODE::addBoxCollider(const vec3f& pos, const vec3f& size, unsigned int spaceId, unsigned int category, unsigned long collideMask)
{
	auto pSpace = core->getDynamicSubSpace(spaceId);
	GUARD_FATAL(pSpace);

	auto pGeom = ODE_CALL(dCreateBox)(pSpace, ODE_V3(size));
	GUARD_FATAL(pGeom);

	ODE_CALL(dGeomSetBody)(pGeom, id);

	ODE_CALL(dGeomSetOffsetPosition)(pGeom, ODE_V3(pos));
	const dReal* pRotation = ODE_CALL(dBodyGetRotation)(id);
	ODE_CALL(dGeomSetRotation)(pGeom, pRotation);

	ODE_CALL(dGeomSetCategoryBits)(pGeom, category);
	ODE_CALL(dGeomSetCollideBits)(pGeom, collideMask);

	geoms.emplace_back(pGeom);
}

void RigidBodyODE::addMeshCollider(ITriMeshPtr trimesh, const mat44f& offset, unsigned int spaceId, unsigned long category, unsigned long collideMask)
{
	auto pCollider = std::dynamic_pointer_cast<CollisionMeshODE>(core->createCollider(trimesh, true, spaceId, category, collideMask));

	ODE_CALL(dGeomSetBody)(pCollider->geom, id);

	dMatrix3 r;
	ODE_CALL(dRSetIdentity)(r);
	r[0] = offset.M11;
	r[1] = offset.M21;
	r[2] = offset.M31;
	r[4] = offset.M12;
	r[5] = offset.M22;
	r[6] = offset.M32;
	r[8] = offset.M13;
	r[9] = offset.M23;
	r[10] = offset.M33;

	ODE_CALL(dGeomSetOffsetRotation)(pCollider->geom, r);
	//ODE_CALL(dGeomSetOffsetPosition)(pCollider->geom, 0, 0, 0); // TODO: check
	ODE_CALL(dGeomSetOffsetPosition)(pCollider->geom, offset.M41, offset.M42, offset.M43);

	collisionMeshes.emplace_back(std::move(pCollider));
}

}
