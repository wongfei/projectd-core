#pragma once

#include <memory>

namespace D {

struct IPhysicsEngine;

struct PhysicsFactory
{
	static std::shared_ptr<IPhysicsEngine> createPhysicsEngine();
};

};
