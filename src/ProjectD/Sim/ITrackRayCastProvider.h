#pragma once

#include "Sim/Surface.h"

namespace D {

struct TrackRayCastHit : public RayCastHit
{
	Surface* surface = nullptr;
};

struct ITrackRayCastProvider : public virtual IObject
{
	virtual IRayCasterPtr createRayCaster(float length) = 0;
	virtual bool rayCast(const vec3f& pos, const vec3f& dir, float length, TrackRayCastHit& hit) = 0;
	virtual bool rayCastWithRayCaster(const vec3f& pos, const vec3f& dir, IRayCasterPtr ray, TrackRayCastHit& hit) = 0;
};

}
