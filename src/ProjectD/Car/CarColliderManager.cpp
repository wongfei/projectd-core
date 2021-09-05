#include "Car/CarColliderManager.h"
#include "Car/Car.h"

namespace D {

CarColliderManager::CarColliderManager()
{}

CarColliderManager::~CarColliderManager()
{}

void CarColliderManager::init(Car* car)
{
	auto ini(std::make_unique<INIReader>(car->carDataPath + L"colliders.ini"));
	GUARD_FATAL(ini->ready);

	for (int id = 0; ; id++)
	{
		auto sectionName = strwf(L"COLLIDER_%d", id);
		if (!ini->hasSection(sectionName))
			break;

		vec3f vCentre = ini->getFloat3(sectionName, L"CENTRE");
		vec3f vSize = ini->getFloat3(sectionName, L"SIZE");

		CarCollisionBox box;
		box.id = id;
		box.centre = vCentre;
		box.size = vSize;
		boxes.emplace_back(box);

		car->body->addBoxCollider(vCentre, vSize, car->physicsGUID + 1, C_CATEGORY_CAR, C_MASK_CAR_BOX);
	}
}

}
