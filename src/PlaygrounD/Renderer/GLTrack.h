#pragma once

#include "Renderer/GLCore.h"

namespace D {

struct Track;

struct GLTrack : public IAvatar
{
	GLTrack(Track* track);
	virtual ~GLTrack();
	
	virtual void draw() override;

	void setCamera(const vec3f& pos, const vec3f& dir) { camPos = pos, camDir = dir; }
	void drawSurfaces();

	Track* track = nullptr;
	vec3f camPos;
	vec3f camDir;
	GLDisplayList trackBatch;
	GLDisplayList wallsBatch;
	GLDisplayList fatPointsBatch;
	std::vector<size_t> nearbyPoints;
	bool wireframe = false;
	bool drawFatPoints = false;
	bool drawNearbyPoints = false;
};

}
