#pragma once

#include "Sim/SimulatorCommon.h"
#include "Sim/ITrackRayCastProvider.h"
#include "Core/VertexHash.h"
#include <unordered_set>

namespace D {

#pragma pack(push, 1)
struct SlimTrackPoint
{
	vec3f best;
	float sides[2];
};
struct FatTrackPoint
{
	vec3f best;
	vec3f left;
	vec3f right;
	vec3f center;
	vec3f forwardDir;
};
#pragma pack(pop)

struct Track : public ITrackRayCastProvider
{
	Track(Simulator* sim);
	~Track();

	bool init(const std::wstring& trackName);
	void step(float dt);

	// ITrackRayCastProvider
	IRayCasterPtr createRayCaster(float length) override;
	bool rayCast(const vec3f& pos, const vec3f& dir, float length, TrackRayCastHit& result) override;
	bool rayCastWithRayCaster(const vec3f& pos, const vec3f& dir, IRayCasterPtr ray, TrackRayCastHit& result) override;

	void loadSurfaceBlob();
	void loadPits();
	
	void initTrackPoints();
	void loadSlimPoints();
	void loadFatPoints();
	void saveFatPoints();
	void computeFatPoints();
	vec3f computeSideLocation(const SlimTrackPoint& slim, FatTrackPoint& fat, const TrackRayCastHit& origHit, const vec3f& rayStart, const vec3f& traceDir, int numSteps);
	float rayCastTrackBounds(const vec3f& pos, const vec3f& dir, float maxDistance = 0.0f);

	std::wstring name;
	std::wstring dataFolder;
	float dynamicGripLevel = 1.0f;

	Simulator* sim = nullptr;
	std::vector<SurfacePtr> surfaces;
	std::vector<ICollisionObjectPtr> colliders;
	std::vector<mat44f> pits;

	std::vector<SlimTrackPoint> slimPoints;
	std::vector<FatTrackPoint> fatPoints;
	VertexHash fatPointsHash;
	std::vector<size_t> nearbyPoints;
	vec3f pointCachePos;

	bool traceSides = false;
	float traceRayOffsetY = 20.0f;
	float traceRayLength = 100.0f;
	float traceSideMax = 10.0f;
	float traceDiffHeightMax = 0.01f;
	float traceDiffGripMax = 0.1f;
	float traceStep = 0.01f;
	std::unordered_set<int> traceBadSectors;

	std::unique_ptr<IAvatar> avatar;
};
	
}
