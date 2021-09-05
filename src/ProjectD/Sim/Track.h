#pragma once

#include "Sim/SimulatorCommon.h"
#include "Sim/ITrackRayCastProvider.h"

namespace D {

struct Track : public ITrackRayCastProvider
{
	Track(SimulatorPtr sim);
	~Track();

	bool init(const std::wstring& trackName);
	void loadSurfaceBlob();
	void loadPits();
	void step(float dt);

	// ITrackRayCastProvider
	IRayCasterPtr createRayCaster(float length) override;
	bool rayCast(const vec3f& pos, const vec3f& dir, float length, TrackRayCastHit& result) override;
	bool rayCastWithRayCaster(const vec3f& pos, const vec3f& dir, IRayCasterPtr ray, TrackRayCastHit& result) override;

	std::wstring name;
	std::wstring dataFolder;
	float dynamicGripLevel = 1.0f;

	SimulatorPtr sim;
	std::vector<SurfacePtr> surfaces;
	std::vector<ICollisionObjectPtr> colliders;
	std::vector<mat44f> pits;
};
	
}
