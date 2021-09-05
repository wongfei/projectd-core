#pragma once

#include "Physics/PhysicsCommon.h"

namespace D {

struct IJoint : public virtual IObject
{
	virtual void setERPCFM(float erp, float cfm) {}
	virtual void reseatDistanceJointLocal(const vec3f& p1, const vec3f& p2) {}
};

DECL_SHARED_PTR(IJoint);

}
