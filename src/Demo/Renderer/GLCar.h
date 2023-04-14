#pragma once

#include "Renderer/GLPrimitive.h"

namespace D {

struct Car;

struct GLCar
{
	void init(Car* car);
	void draw(bool drawBody = true, bool drawProbes = false);
	void drawInstance(Car* instance, bool drawBody = true, bool drawProbes = false);

	Car* car = nullptr;
	GLPrimitive bodyMesh;
	GLPrimitive wheelMesh;
};

}
