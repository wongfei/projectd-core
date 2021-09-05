#include "Physics/ODE/CollisionMeshODE.h"

namespace D {

CollisionMeshODE::CollisionMeshODE(
	PhysicsEngineODEPtr _core, ITriMeshPtr _trimesh, 
	dxSpace* space, unsigned long category, unsigned long collideMask)
:
	PhysicsChildODE(_core), 
	trimesh(_trimesh)
{
	//TRACE_CTOR(CollisionMeshODE);

	GUARD_FATAL(trimesh->getVertexCount() > 0);
	GUARD_FATAL(trimesh->getIndexCount() > 0);

	int iVertexSize = 3 * sizeof(float);
	int iIndexSize = sizeof(unsigned short);
	int iTriangleSize = 3 * iIndexSize;

	geomTrimesh = ODE_CALL(dGeomTriMeshDataCreate)();
	GUARD_FATAL(geomTrimesh);

	ODE_CALL(dGeomTriMeshDataBuildSingle)(geomTrimesh, 
		trimesh->getVB(), iVertexSize, (int)trimesh->getVertexCount(),
		trimesh->getIB(), (int)trimesh->getIndexCount(), iTriangleSize);

	geom = ODE_CALL(dCreateTriMesh)(space, geomTrimesh, nullptr, nullptr, nullptr);
	GUARD_FATAL(geom);

	ODE_CALL(dGeomSetData)(geom, static_cast<ICollisionObject*>(this));
	ODE_CALL(dGeomSetCategoryBits)(geom, category);
	ODE_CALL(dGeomSetCollideBits)(geom, collideMask);
}

CollisionMeshODE::~CollisionMeshODE()
{
	//TRACE_DTOR(CollisionMeshODE);

	ODE_CALL(dGeomDestroy)(geom);
	ODE_CALL(dGeomTriMeshDataDestroy)(geomTrimesh);
}

void CollisionMeshODE::setUserPointer(void* data)
{
	userPointer = data;
}

void* CollisionMeshODE::getUserPointer()
{
	return userPointer;
}

unsigned long CollisionMeshODE::getGroup()
{
	return ODE_CALL(dGeomGetCategoryBits)(geom);
}

unsigned long CollisionMeshODE::getMask()
{
	return ODE_CALL(dGeomGetCollideBits)(geom);
}

}
