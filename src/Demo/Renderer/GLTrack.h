#pragma once

#include "Renderer/GLCore.h"

namespace D {

struct Track;

struct GLTrack
{
	void init(Track* track);
	void drawSurfaces(const vec3f& lightPos, const vec3f& lightDir);
	void drawWalls();
	void drawOrigin();

	Track* track = nullptr;
	GLDisplayList surfaces;
	GLDisplayList walls;
};

}
