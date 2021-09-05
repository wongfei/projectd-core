#pragma once

#include "Physics/ICollisionObject.h"

namespace D {

struct RayCastHit
{
	ICollisionObject* collisionObject = nullptr;
	vec3f pos;
	vec3f normal;
	bool hasContact = false;
};

struct IRayCaster : public virtual IObject
{
	virtual RayCastHit rayCast(const vec3f& pos, const vec3f& dir) = 0;
};

DECL_SHARED_PTR(IRayCaster);

}
