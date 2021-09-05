#include "Physics/PhysicsFactory.h"
#include "Physics/ODE/PhysicsEngineODE.h"

namespace D {

std::shared_ptr<IPhysicsEngine> PhysicsFactory::createPhysicsEngine()
{
	return std::make_shared<PhysicsEngineODE>();
}

}
