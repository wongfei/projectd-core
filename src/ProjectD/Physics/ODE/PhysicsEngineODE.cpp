#include "Physics/ODE/PhysicsEngineODE.h"

#include "Physics/ODE/RigidBodyODE.h"
#include "Physics/ODE/JointODE.h"
#include "Physics/ODE/RayCasterODE.h"
#include "Physics/ODE/TriMeshODE.h"

namespace D {

PhysicsEngineODE::PhysicsEngineODE()
{
	TRACE_CTOR(PhysicsEngineODE);

	int rc = ODE_CALL(dInitODE2)(0);
	GUARD_FATAL(rc != 0);

	ODE_CALL(dAllocateODEDataForThread)(0xFFFFFFFF);
	log_printf(L"ODE BUILD FLAGS: %S", ODE_CALL(dGetConfiguration)());

	world = ODE_CALL(dWorldCreate)();
	GUARD_FATAL(world);

	ODE_CALL(dWorldSetGravity)(world, 0.0f, -9.80665f, 0.0f);
	ODE_CALL(dWorldSetERP)(world, 0.3f);
	ODE_CALL(dWorldSetCFM)(world, 1.0e-7f);
	ODE_CALL(dWorldSetContactMaxCorrectingVel)(world, 3.0f);
	ODE_CALL(dWorldSetContactSurfaceLayer)(world, 0.0f);
	ODE_CALL(dWorldSetDamping)(world, 0.0f, 0.0f);
	ODE_CALL(dWorldSetQuickStepNumIterations)(world, 48);

	contactGroup = ODE_CALL(dJointGroupCreate)(0);
	GUARD_FATAL(contactGroup);

	contactGroupDynamic = ODE_CALL(dJointGroupCreate)(0);
	GUARD_FATAL(contactGroupDynamic);

	currentContactGroup = contactGroup;

	spaceStatic = ODE_CALL(dSimpleSpaceCreate)(nullptr);
	GUARD_FATAL(spaceStatic);

	spaceDynamic = ODE_CALL(dSimpleSpaceCreate)(nullptr);
	GUARD_FATAL(spaceDynamic);

	ray = ODE_CALL(dCreateRay)(nullptr, 100.0f);
	GUARD_FATAL(ray);

	ODE_CALL(dGeomRaySetFirstContact)(ray, 1);
	ODE_CALL(dGeomRaySetBackfaceCull)(ray, 1);
}

PhysicsEngineODE::~PhysicsEngineODE()
{
	TRACE_DTOR(PhysicsEngineODE);

	ODE_CALL(dGeomDestroy)(ray);

	// When a space is destroyed, if its cleanup mode is 1 (the default) 
	// then all the geoms in that space are automatically destroyed as well.

	for (auto& pair : dynamicSubSpaces)
		ODE_CALL(dSpaceDestroy)(pair.second);
	ODE_CALL(dSpaceDestroy)(spaceDynamic);

	for (auto& pair : staticSubSpaces)
		ODE_CALL(dSpaceDestroy)(pair.second);
	ODE_CALL(dSpaceDestroy)(spaceStatic);

	ODE_CALL(dJointGroupDestroy)(contactGroupDynamic);
	ODE_CALL(dJointGroupDestroy)(contactGroup);

	ODE_CALL(dWorldDestroy)(world);
	ODE_CALL(dCloseODE)();
}

dxSpace* PhysicsEngineODE::getStaticSubSpace(unsigned int index)
{
	auto iter = staticSubSpaces.find(index);
	if (iter != staticSubSpaces.end())
		return iter->second;

	auto space = ODE_CALL(dSimpleSpaceCreate)(spaceStatic);
	GUARD_FATAL(space);

	staticSubSpaces.insert({index, space});
	return space;
}

dxSpace* PhysicsEngineODE::getDynamicSubSpace(unsigned int index)
{
	auto iter = dynamicSubSpaces.find(index);
	if (iter != dynamicSubSpaces.end())
		return iter->second;

	auto space = ODE_CALL(dSimpleSpaceCreate)(spaceDynamic);
	GUARD_FATAL(space);

	dynamicSubSpaces.insert({index, space});
	return space;
}

IRigidBodyPtr PhysicsEngineODE::createRigidBody()
{
	return std::make_shared<RigidBodyODE>(shared_from_this());
}

IJointPtr PhysicsEngineODE::createFixedJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2)
{
	return std::make_shared<FixedJointODE>(shared_from_this(), std::dynamic_pointer_cast<RigidBodyODE>(rb1), std::dynamic_pointer_cast<RigidBodyODE>(rb2));
}

IJointPtr PhysicsEngineODE::createBallJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& pos)
{
	return std::make_shared<BallJointODE>(shared_from_this(), std::dynamic_pointer_cast<RigidBodyODE>(rb1), std::dynamic_pointer_cast<RigidBodyODE>(rb2), pos);
}

IJointPtr PhysicsEngineODE::createSliderJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& axis)
{
	return std::make_shared<SliderJointODE>(shared_from_this(), std::dynamic_pointer_cast<RigidBodyODE>(rb1), std::dynamic_pointer_cast<RigidBodyODE>(rb2), axis);
}

IJointPtr PhysicsEngineODE::createDistanceJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& p1, const vec3f& p2)
{
	return std::make_shared<DistanceJointODE>(shared_from_this(), std::dynamic_pointer_cast<RigidBodyODE>(rb1), std::dynamic_pointer_cast<RigidBodyODE>(rb2), p1, p2);
}

IJointPtr PhysicsEngineODE::createBumpJoint(IRigidBodyPtr rb1, IRigidBodyPtr rb2, const vec3f& p1, float rangeUp, float rangeDn)
{
	return nullptr; // TODO?
}

ITriMeshPtr PhysicsEngineODE::createTriMesh()
{
	return std::make_shared<TriMeshODE>();
}

ICollisionObjectPtr PhysicsEngineODE::createCollider(ITriMeshPtr trimesh, bool isDynamic, unsigned int spaceId, unsigned long category, unsigned long collideMask)
{
	dxSpace* space = isDynamic ? getDynamicSubSpace(spaceId) : getStaticSubSpace(spaceId);
	GUARD_FATAL(space);
	return std::make_shared<CollisionMeshODE>(shared_from_this(), trimesh, space, category, collideMask);
}

IRayCasterPtr PhysicsEngineODE::createRayCaster(float length)
{
	return std::make_shared<RayCasterODE>(shared_from_this(), length);
}

void PhysicsEngineODE::setCollisionCallback(ICollisionCallback* callback)
{
	collisionCallback = callback;
}

RayCastHit PhysicsEngineODE::rayCast(const vec3f& pos, const vec3f& dir, float length)
{
	ODE_CALL(dGeomRaySetLength)(ray, length); 
	return rayCastImpl(pos, dir, ray);
}

RayCastHit PhysicsEngineODE::rayCast(const vec3f& pos, const vec3f& dir, IRayCasterPtr pray)
{
	auto* dxray = std::dynamic_pointer_cast<RayCasterODE>(pray)->id;
	return rayCastImpl(pos, dir, dxray);
}

static void rayNearCallback(void* data, dxGeom* o1, dxGeom* o2);

RayCastHit PhysicsEngineODE::rayCastImpl(const vec3f& pos, const vec3f& dir, dxGeom* dxray)
{
	RayCastHit hit;

	dContactGeom cg;
	cg.depth = -1.0f;

	ODE_CALL(dGeomRaySet)(dxray, ODE_V3(pos), ODE_V3(dir));
	ODE_CALL(dSpaceCollide2)(dxray, (dGeomID)spaceStatic, &cg, rayNearCallback);

	if (cg.depth >= 0.0f)
	{
		hit.pos = vec3f(cg.pos);
		hit.normal = vec3f(cg.normal);
		hit.collisionObject = (ICollisionObject*)ODE_CALL(dGeomGetData)(cg.g2);
		hit.hasContact = true;
	}

	return hit;
}

static void rayNearCallback(void* data, dxGeom* o1, dxGeom* o2)
{
	if (ODE_CALL(dGeomIsSpace)(o1) || ODE_CALL(dGeomIsSpace)(o2))
	{
		ODE_CALL(dSpaceCollide2)(o1, o2, data, rayNearCallback);
	}
	else if (!ODE_CALL(dGeomGetBody)(o1) && !ODE_CALL(dGeomGetBody)(o2))
	{
		auto* result = (dContactGeom*)data;

		const int maxContacts = 32;
		dContactGeom contacts[maxContacts];

		const int n = ODE_CALL(dCollide)(o1, o2, 1i64, &contacts[0], sizeof(dContactGeom));

		for (int i = 0; i < n; ++i)
		{
			const dContactGeom& cg = contacts[i];

			if (result->depth < 0.0f || result->depth > cg.depth)
			{
				*result = cg;
			}
		}
	}
}

void PhysicsEngineODE::step(float dt)
{
	if (noCollisionCounter)
		noCollisionCounter--;
	else
		collisionStep(dt);

	ODE_CALL(dWorldStep)(world, dt);
}

static void collisionNearCallback(void* data, dxGeom* o1, dxGeom* o2);

void PhysicsEngineODE::collisionStep(float dt)
{
	if (currentFrame & 1)
	{
		ODE_CALL(dJointGroupEmpty)(contactGroupDynamic);
		currentContactGroup = contactGroupDynamic;
		ODE_CALL(dSpaceCollide2)((dGeomID)spaceDynamic, (dGeomID)spaceStatic, this, collisionNearCallback);
	}
	else
	{
		ODE_CALL(dJointGroupEmpty)(contactGroup);
		currentContactGroup = contactGroup;
		ODE_CALL(dSpaceCollide)(spaceDynamic, this, collisionNearCallback);
	}

	currentFrame++;
}

static void collisionNearCallback(void* data, dxGeom* o1, dxGeom* o2)
{
	auto s1 = ODE_CALL(dGeomIsSpace)(o1);
	auto s2 = ODE_CALL(dGeomIsSpace)(o2);

	if (s1 || s2)
	{
		ODE_CALL(dSpaceCollide2)(o1, o2, data, collisionNearCallback);
	}
	else
	{
		auto cat1 = ODE_CALL(dGeomGetCategoryBits)(o1);
		auto mask1 = ODE_CALL(dGeomGetCollideBits)(o1);

		auto cat2 = ODE_CALL(dGeomGetCategoryBits)(o2);
		auto mask2 = ODE_CALL(dGeomGetCollideBits)(o2);

		bool bMatch = (cat1 & mask2) && (cat2 & mask1);
		if (bMatch)
		{
			auto b1 = ODE_CALL(dGeomGetBody)(o1);
			auto b2 = ODE_CALL(dGeomGetBody)(o2);

			const int flags = (b1 && b2) ? 4 : 32;

			const int maxContacts = 32;
			dContactGeom contacts[maxContacts];

			const int n = ODE_CALL(dCollide)(o1, o2, flags, &contacts[0], sizeof(dContactGeom));

			if (n > 0)
			{
				((PhysicsEngineODE*)data)->onCollision(&contacts[0], n, o1, o2);
			}
		}
	}
}

void PhysicsEngineODE::onCollision(dContactGeom* contacts, int numContacts, dxGeom* o1, dxGeom* o2)
{
	dBodyID body0 = ODE_CALL(dGeomGetBody)(o1);
	dBodyID body1 = ODE_CALL(dGeomGetBody)(o2);
	int geomClass0 = ODE_CALL(dGeomGetClass)(o1);
	int geomClass1 = ODE_CALL(dGeomGetClass)(o2);
	auto* shape0 = (ICollisionObject*)ODE_CALL(dGeomGetData)(o1);
	auto* shape1 = (ICollisionObject*)ODE_CALL(dGeomGetData)(o2);
	auto* userData0 = body0 ? (IRigidBody*)ODE_CALL(dBodyGetData)(body0) : nullptr;
	auto* userData1 = body1 ? (IRigidBody*)ODE_CALL(dBodyGetData)(body1) : nullptr;

	for (int i = 0; i < numContacts; ++i)
	{
		const dContactGeom& cg = contacts[i];

		dContact cj;
		memzero(cj);
		cj.geom = cg;

		if ((geomClass0 != dBoxClass || geomClass1 != dTriMeshClass) 
			&& (geomClass0 != dTriMeshClass || geomClass1 != dBoxClass))
		{
			cj.surface.mode = 28692; // dContactBounce | dContactSoftCFM | dContactApprox1
			cj.surface.mu = 0.25f;
			cj.surface.bounce = 0.01f; // 3C23D70A
			cj.surface.soft_cfm = 0.0001f; // 38D1B717
		}
		else
		{
			if (body0 || body1)
			{
				dVector3 loc_normal;
				ODE_CALL(dBodyVectorFromWorld)((body0 ? body0 : body1), 
					cg.normal[0], cg.normal[1], cg.normal[2], loc_normal);

				if (loc_normal[1] < 0.9f)
					continue;
			}

			cj.surface.mode = 28700; // dContactBounce | dContactSoftERP | dContactSoftCFM | dContactApprox1
			cj.surface.mu = 0.1f;
			cj.surface.bounce = 0;
			cj.surface.soft_cfm = 0.000952380942f; // 3A79A934
			cj.surface.soft_erp = 0.714285731f; // 3F36DB6E
		}

		dJointID j = ODE_CALL(dJointCreateContact)(world, currentContactGroup, &cj);
		ODE_CALL(dJointAttach)(j, body0, body1);

		if (collisionCallback)
		{
			collisionCallback->onCollisionCallback(
				userData0, shape0, 
				userData1, shape1, 
				vec3f(cg.normal), vec3f(cg.pos), cg.depth);
		}
	}
}

}
