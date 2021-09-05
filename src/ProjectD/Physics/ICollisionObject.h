#pragma once

#include "Physics/PhysicsCommon.h"

namespace D {

struct ICollisionObject : public virtual IObject
{
	virtual void setUserPointer(void* data) = 0;
	virtual void* getUserPointer() = 0;
	virtual unsigned long getGroup() = 0;
	virtual unsigned long getMask() = 0;
};

DECL_SHARED_PTR(ICollisionObject);

}
