#pragma once

#include "Renderer/GLPrimitive.h"

namespace D {

struct Car;

struct GLCar
{
	void init(Car* car);
	void draw(bool drawBody = true);
	void drawInstance(Car* instance, bool drawBody = true);

	Car* car = nullptr;
	GLPrimitive bodyMesh;
	GLPrimitive wheelMesh;
};

}
