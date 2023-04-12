#pragma once

#include "Renderer/GLCore.h"

namespace D {

struct Track;

struct GLTrack
{
	void init(Track* track);
	
	void drawOrigin();
	void drawTrack(bool wireframe);
	void drawRacePoints();

	void setLight(const vec3f& pos, const vec3f& dir);
	void beginEnv(bool wireframe);
	void endEnv(bool wireframe);

	Track* track = nullptr;
	vec3f lightPos;
	vec3f lightDir;
	GLDisplayList trackBatch;
	GLDisplayList wallsBatch;
	GLDisplayList racePoints;
};

}
