#pragma once

#include "Renderer/GLCore.h"

namespace D {

struct ITriMesh;

void glxBox();
void glxTriMesh(ITriMesh* trimesh);

struct GLPrimitive
{
	void initBox();
	void initTriMesh(ITriMesh* trimesh);
	void draw();

	GLDisplayList list;
	vec3f color;
	bool wireframe = false;
};

}
