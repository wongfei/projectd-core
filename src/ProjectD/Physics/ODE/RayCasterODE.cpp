#include "Physics/ODE/RayCasterODE.h"

namespace D {

RayCasterODE::RayCasterODE(PhysicsEngineODEPtr pcore, float length)
:
	PhysicsChildODE(pcore)
{
	//TRACE_CTOR(RayCasterODE);

	id = ODE_CALL(dCreateRay)(nullptr, length);
	GUARD_FATAL(id);

	ODE_CALL(dGeomRaySetFirstContact)(id, 1);
	ODE_CALL(dGeomRaySetBackfaceCull)(id, 1);
}

RayCasterODE::~RayCasterODE()
{
	//TRACE_DTOR(RayCasterODE);

	ODE_CALL(dGeomDestroy)(id);
}

RayCastHit RayCasterODE::rayCast(const vec3f& pos, const vec3f& dir)
{
	return core->rayCastImpl(pos, dir, id);
}

}
