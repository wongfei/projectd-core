#pragma once

#include "Physics/IRigidBody.h"
#include "Physics/ICollisionObject.h"

namespace D {

struct ICollisionCallback : public virtual IObject
{
	virtual void onCollisionCallback(
		IRigidBody* rb0, ICollisionObject* shape0, 
		IRigidBody* rb1, ICollisionObject* shape1, 
		const vec3f& normal, const vec3f& pos, float depth) = 0;
};

}
