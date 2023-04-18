#pragma once

#include "Renderer/GLCore.h"

namespace D {

struct Track;

struct GLTrack
{
	void init(Track* track);
	
	void drawOrigin();
	void drawTrack(bool wireframe);
	void drawTrackPoints();
	void drawNearbyPoints(const vec3f& cameraPos);

	void setLight(const vec3f& pos, const vec3f& dir);
	void beginSurface(bool wireframe);
	void endSurface(bool wireframe);

	Track* track = nullptr;
	vec3f lightPos;
	vec3f lightDir;
	GLDisplayList trackBatch;
	GLDisplayList wallsBatch;
	GLDisplayList fatPointsBatch;
	std::vector<size_t> nearbyPoints;
};

}
