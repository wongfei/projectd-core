#pragma once

#include "Renderer/GLPrimitive.h"

namespace D {

struct Car;

struct GLCar : public IAvatar
{
	GLCar(Car* car);
	virtual ~GLCar();

	virtual void draw() override;

	Car* car = nullptr;
	GLPrimitive bodyMesh;
	GLPrimitive wheelMesh;
	bool drawBody = true;
	bool drawProbes = false;
	bool drawSplineLocation = true;
	bool drawSenseiPoints = false;
};

}
