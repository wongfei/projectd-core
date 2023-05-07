#include "Sim/Track.h"
#include "Sim/Simulator.h"

#define TRACK_DEBUG_DRAW 0

#if (TRACK_DEBUG_DRAW)
#include <SDL_opengl.h>
#endif

namespace D {

Track::Track(Simulator* _sim)
{
	TRACE_CTOR(Track);

	sim = _sim;
}

Track::~Track()
{
	TRACE_DTOR(Track);
}

bool Track::init(const std::wstring& trackName)
{
	log_printf(L"Track: init: trackName=\"%s\"", trackName.c_str());

	GUARD_FATAL(sim);
	GUARD_FATAL(sim->physics);

	name = trackName;
	dataFolder = sim->basePath + L"content/tracks/" + name + L"/";
	log_printf(L"trackData: \"%s\"", dataFolder.c_str());

	auto ini(std::make_unique<INIReader>(sim->basePath + L"cfg/sim.ini"));
	if (ini->ready)
	{
		ini->tryGetFloat(L"ENVIRONMENT", L"TRACK_GRIP", dynamicGripLevel);
	}

	loadSurfaceBlob();
	loadPits();
	initTrackPoints();

	log_printf(L"Track: init: DONE");
	return true;
}

void Track::step(float dt) // TODO?
{
}

IRayCasterPtr Track::createRayCaster(float length)
{
	return sim->physics->createRayCaster(length);
}

bool Track::rayCast(const vec3f& org, const vec3f& dir, float length, TrackRayCastHit& result)
{
	auto hit = sim->physics->rayCast(org, dir, length);
	if (hit.hasContact)
	{
		result.surface = (Surface*)hit.collisionObject->getUserPointer();
		result.collisionObject = hit.collisionObject;
		result.pos = hit.pos;
		result.normal = hit.normal;
		result.hasContact = hit.hasContact;
	}
	else
	{
		memzero(result);
	}
	return hit.hasContact;
}

bool Track::rayCastWithRayCaster(const vec3f& org, const vec3f& dir, IRayCasterPtr rayCaster, TrackRayCastHit& result)
{
	auto hit = rayCaster->rayCast(org, dir);
	if (hit.hasContact)
	{
		result.surface = (Surface*)hit.collisionObject->getUserPointer();
		result.collisionObject = hit.collisionObject;
		result.pos = hit.pos;
		result.normal = hit.normal;
		result.hasContact = hit.hasContact;
	}
	else
	{
		memzero(result);
	}
	return hit.hasContact;
}

void Track::loadSurfaceBlob()
{
	colliders.clear();
	surfaces.clear();

	FileHandle file;
	auto strPath = dataFolder + L"surfaces.bin";
	log_printf(L"loadSurfaceBlob: %s", strPath.c_str());

	const bool fileValid = file.open(strPath.c_str(), L"rb");
	GUARD_FATAL(fileValid);

	BlobSurface blob;
	while (fread(&blob, sizeof(blob), 1, file.fd) == 1)
	{
		GUARD_FATAL(blob.magic == 0xAABBCCDD);
		GUARD_FATAL(blob.numVertices > 0 && blob.numIndices > 0);

		if (!(blob.collisionCategory == C_CATEGORY_TRACK || blob.collisionCategory == C_CATEGORY_WALL))
		{
			log_printf(L"UNKNOWN collisionCategory=%u", blob.collisionCategory);
		}

		auto trimesh = sim->physics->createTriMesh();
		trimesh->resize(blob.numVertices, blob.numIndices);
		GUARD_FATAL(fread(trimesh->getVB(), blob.numVertices * sizeof(TriMeshVertex), 1, file.fd) == 1);
		GUARD_FATAL(fread(trimesh->getIB(), blob.numIndices * sizeof(TriMeshIndex), 1, file.fd) == 1);

		auto pSurf = std::make_shared<Surface>();
		pSurf->trimesh = trimesh;
		pSurf->sectorID = blob.sectorID;
		pSurf->collisionCategory = blob.collisionCategory;
		pSurf->gripMod = blob.gripMod;
		pSurf->damping = blob.damping;
		pSurf->sinHeight = blob.sinHeight;
		pSurf->sinLength = blob.sinLength;
		pSurf->granularity = blob.granularity;
		pSurf->dirtAdditiveK = blob.dirtAdditiveK;
		pSurf->vibrationGain = blob.vibrationGain;
		pSurf->vibrationLength = blob.vibrationLength;
		pSurf->wavPitchSpeed = blob.wavPitchSpeed;
		pSurf->isValidTrack = blob.isValidTrack;
		pSurf->isPitlane = blob.isPitlane;

		//log_printf(L"Surface: sectorID=%u collisionCategory=%u gripMod=%.3f damping=%.3f", pSurf->sectorID, pSurf->collisionCategory, pSurf->gripMod, pSurf->damping);

		auto pCollider = sim->physics->createCollider(pSurf->trimesh, false, pSurf->sectorID, pSurf->collisionCategory, C_MASK_SURFACE);
		pCollider->setUserPointer(pSurf.get());

		surfaces.emplace_back(std::move(pSurf));
		colliders.emplace_back(std::move(pCollider));
	}
}

void Track::loadPits()
{
	pits.clear();

	auto ini(std::make_unique<INIReader>(dataFolder + L"pits.ini"));
	if (ini->ready)
	{
		for (int i = 0; ; ++i)
		{
			auto strSection = strwf(L"AC_PIT_%d", i);
			if (!ini->hasSection(strSection))
				break;

			auto vPos = ini->getFloat3(strSection, L"POS");
			auto vRot = ini->getFloat3(strSection, L"ROT");

			mat44f m;
			m = mat44f::createFromAxisAngle(vec3f(0, 1, 0), vRot.x * 0.01745329251994329576923690768489f);
			m.M41 = vPos.x;
			m.M42 = vPos.y;
			m.M43 = vPos.z;

			pits.emplace_back(m);
		}
	}
}

void Track::initTrackPoints()
{
	traceBadSectors.clear();

	auto ini(std::make_unique<INIReader>(dataFolder + L"spline.ini"));
	if (ini->ready)
	{
		traceSides = ini->getInt(L"SPLINE", L"TRACE_SIDES") != 0;

		ini->tryGetFloat(L"SPLINE", L"TRACE_RAY_OFFSET_Y", traceRayOffsetY);
		ini->tryGetFloat(L"SPLINE", L"TRACE_RAY_LENGTH", traceRayLength);
		ini->tryGetFloat(L"SPLINE", L"TRACE_SIDE_MAX", traceSideMax);
		ini->tryGetFloat(L"SPLINE", L"TRACE_DIFF_HEIGHT_MAX", traceDiffHeightMax);
		ini->tryGetFloat(L"SPLINE", L"TRACE_DIFF_GRIP_MAX", traceDiffGripMax);
		ini->tryGetFloat(L"SPLINE", L"TRACE_STEP", traceStep);

		if (ini->hasKey(L"SPLINE", L"TRACE_BAD_SECTORS"))
		{
			std::wstring list = ini->getString(L"SPLINE", L"TRACE_BAD_SECTORS");
			auto items = split(list, L"|");
			for (const auto& item : items)
			{
				auto id = std::stoi(item);
				traceBadSectors.insert(id);
			}
		}
	}

	loadSlimPoints();
	loadFatPoints();

	if (fatPoints.size() != slimPoints.size())
	{
		fatPoints.clear();
	}

	if (fatPoints.empty() && !slimPoints.empty())
	{
		computeFatPoints();
		saveFatPoints();
	}

	computedTrackWidth = 0.1f;
	computedTrackLength = 0.1f;

	if (!fatPoints.empty())
	{
		float cellSize = 50;
		int tableSize = 4096;

		auto simIni(std::make_unique<INIReader>(sim->basePath + L"cfg/sim.ini"));
		if (simIni->ready)
		{
			simIni->tryGetFloat(L"VERTEX_HASH", L"CELL_SIZE", cellSize);
			simIni->tryGetInt(L"VERTEX_HASH", L"TABLE_SIZE", tableSize);
		}

		fatPointsHash.init(cellSize, (size_t)tableSize); // allows to query points around specific location
		pointCachePos = vec3f(0, -10000, 0);

		const size_t numPoints = fatPoints.size();
		for (size_t id = 0; id < numPoints; ++id)
		{
			fatPointsHash.add(fatPoints[id].center, id);

			const float dist = (fatPoints[id].left - fatPoints[id].right).len();
			if (computedTrackWidth < dist)
				computedTrackWidth = dist;

			if (id + 1 < numPoints)
			{
				computedTrackLength += (fatPoints[id].center - fatPoints[id + 1].center).len();
			}
		}
	}
}

void Track::loadSlimPoints()
{
	slimPoints.clear();

	FileHandle file;
	auto strPath = dataFolder + L"spline.bin";
	log_printf(L"loadSlimPoints: %s", strPath.c_str());

	if (file.open(strPath.c_str(), L"rb"))
	{
		const size_t size = file.size();
		const size_t numPoints = size / sizeof(slimPoints[0]);
		if (numPoints > 0)
		{
			slimPoints.resize(numPoints);
			fread(slimPoints.data(), sizeof(slimPoints[0]) * numPoints, 1, file.fd);
		}
	}
}

void Track::loadFatPoints()
{
	fatPoints.clear();

	FileHandle file;
	auto strPath = dataFolder + L"spline.cache";
	log_printf(L"loadFatPoints: %s", strPath.c_str());

	if (file.open(strPath.c_str(), L"rb"))
	{
		const size_t size = file.size();
		const size_t numPoints = size / sizeof(fatPoints[0]);
		if (numPoints > 0)
		{
			fatPoints.resize(numPoints);
			fread(fatPoints.data(), sizeof(fatPoints[0]) * numPoints, 1, file.fd);
		}
	}
}

void Track::saveFatPoints()
{
	FileHandle file;
	auto strPath = dataFolder + L"spline.cache";
	log_printf(L"saveFatPoints: %s", strPath.c_str());

	if (file.open(strPath.c_str(), L"wb"))
	{
		const auto numPoints = fatPoints.size();
		if (numPoints > 0)
		{
			fwrite(fatPoints.data(), sizeof(fatPoints[0]) * numPoints, 1, file.fd);
		}
	}
}

void Track::computeFatPoints()
{
	fatPoints.clear();

	const auto numPoints = slimPoints.size();
	if (!numPoints)
		return;

	fatPoints.resize(numPoints);
	memset(fatPoints.data(), 0, sizeof(fatPoints[0]) * numPoints);

	const vec3f rayOff(0, traceRayOffsetY, 0);
	const int numTraceSteps = (int)(traceSideMax / traceStep);

	TrackRayCastHit hit;

	for (size_t i = 0; i < numPoints; ++i)
	{
		const auto& slim = slimPoints[i];
		auto& fat = fatPoints[i];

		const auto& rayStart = slim.best + rayOff;

		if (rayCast(rayStart, vec3f(0, -1, 0), traceRayLength, hit))
		{
			const auto roadCategory = hit.surface->collisionCategory;
			fat.best = hit.pos;

			const float baseGrip = hit.surface->gripMod;

			/*auto nextI = i + 1;
			if (nextI >= numPoints)
				nextI = 0;
			fat.forwardDir = (slimPoints[nextI].best - slim.best).get_norm();*/

			if (i + 1 < numPoints)
				fat.forwardDir = (slimPoints[i + 1].best - slim.best).get_norm();
			else if (i > 0)
				fat.forwardDir = (slim.best - slimPoints[i - 1].best).get_norm();

			auto leftDir = fat.forwardDir.cross(vec3f(0, -1, 0)).get_norm();
			auto rightDir = leftDir * -1.0f;

			if (!traceSides)
			{
				fat.left = fat.best + leftDir * slimPoints[i].sides[0];
				fat.right = fat.best + rightDir * slimPoints[i].sides[1];

				if (rayCast(fat.left + rayOff, vec3f(0, -1, 0), traceRayLength, hit))
				{
					fat.left = hit.pos;
				}

				if (rayCast(fat.right + rayOff, vec3f(0, -1, 0), traceRayLength, hit))
				{
					fat.right = hit.pos;
				}
			}
			else // ray trace sides (slow)
			{
				fat.left = computeSideLocation(slim, fat, hit, rayStart, leftDir, numTraceSteps);
				fat.right = computeSideLocation(slim, fat, hit, rayStart, rightDir, numTraceSteps);
			}

			fat.center = (fat.left + fat.right) * 0.5f;
		}
	}
}

vec3f Track::computeSideLocation(const SlimTrackPoint& slim, FatTrackPoint& fat, const TrackRayCastHit& origHit, const vec3f& rayStart, const vec3f& traceDir, int numSteps)
{
	vec3f result = origHit.pos;
	vec3f prevHit = origHit.pos;
	float prevGrip = origHit.surface->gripMod;

	TrackRayCastHit hit;

	for (int traceId = 1; traceId < numSteps; ++traceId)
	{
		const auto rayEnd = origHit.pos + traceDir * ((float)(traceId) * traceStep);
		const auto rayN = (rayEnd - rayStart).get_norm();

		if (rayCast(rayStart, rayN, traceRayLength, hit))
		{
			if (hit.surface->isValidTrack && 
				hit.surface->collisionCategory == origHit.surface->collisionCategory && 
				fabsf(hit.pos.y - prevHit.y) < traceDiffHeightMax && 
				fabsf(hit.surface->gripMod - prevGrip) < traceDiffGripMax &&
				traceBadSectors.find(hit.surface->sectorID) == traceBadSectors.end()
			)
			{
				result = hit.pos;
				prevHit = hit.pos;
				prevGrip = hit.surface->gripMod;
			}
			else
				break;
		}
	}

	return result;
}

inline bool getLineIntersection(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float &i_x, float &i_y)
{
	// https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect

	float s1_x, s1_y, s2_x, s2_y;
	s1_x = p1_x - p0_x; s1_y = p1_y - p0_y;
	s2_x = p3_x - p2_x; s2_y = p3_y - p2_y;

	float s, t;
	s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
	t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		i_x = p0_x + (t * s1_x);
		i_y = p0_y + (t * s1_y);
		return true;
	}

	return false;
}

inline bool getLineIntersection(const vec2f& p0, const vec2f& p1, const vec2f& p2, const vec2f& p3, vec2f& i)
{
	return getLineIntersection(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, i.x, i.y);
}

// many tracks don't have guardrails/walls, fake them by tracing against track side splines
float Track::rayCastTrackBounds(const vec3f& pos, const vec3f& dir, float maxDistance)
{
	if (maxDistance <= 0.0f)
		maxDistance = fatPointsHash.cellSize;

	float result = maxDistance;

	// allow points to be cached by query position
	const float cacheDistanceTolerance = 1.0f;
	if ((pointCachePos - pos).sqlen() > cacheDistanceTolerance * cacheDistanceTolerance)
	{
		pointCachePos = pos;
		nearbyPoints.clear();
		fatPointsHash.queryNeighbours(pos, nearbyPoints, maxDistance);
	}

	if (!nearbyPoints.empty())
	{
		const auto rayEnd = pos + dir * (maxDistance * 1.1f); // make ray a little longer then point cutoff distance

		const auto rayA = vec2f(pos.x, pos.z);
		const auto rayB = vec2f(rayEnd.x, rayEnd.z);

		vec2f inter(0, 0);
		float bestDist = FLT_MAX;
		bool interFlag = false;

		const size_t maxPoints = fatPoints.size();

		for (size_t id : nearbyPoints)
		{
			const size_t other = id + 1 < maxPoints ? id + 1 : 0;

			auto sideA = fatPoints[id].left;
			auto sideB = fatPoints[other].left;

			if (getLineIntersection(rayA, rayB, vec2f(sideA.x, sideA.z), vec2f(sideB.x, sideB.z), inter))
			{
				bestDist = tmin(bestDist, (rayA - inter).len());
				interFlag = true;

				#if (TRACK_DEBUG_DRAW)
				glBegin(GL_LINES);
					glColor3f(1.0f, 0.0f, 0.0f);
					glVertex3fv(&sideA.x);
					glVertex3fv(&sideB.x);
				glEnd();
				#endif
			}

			sideA = fatPoints[id].right;
			sideB = fatPoints[other].right;

			if (getLineIntersection(rayA, rayB, vec2f(sideA.x, sideA.z), vec2f(sideB.x, sideB.z), inter))
			{
				bestDist = tmin(bestDist, (rayA - inter).len());
				interFlag = true;

				#if (TRACK_DEBUG_DRAW)
				glBegin(GL_LINES);
					glColor3f(1.0f, 0.0f, 0.0f);
					glVertex3fv(&sideA.x);
					glVertex3fv(&sideB.x);
				glEnd();
				#endif
			}
		}

		if (interFlag)
			result = bestDist;
	}

	return result;
}

size_t Track::getPointIdAtDistance(float distanceNorm) const
{
	const size_t numPoints = fatPoints.size();
	if (!numPoints)
		return 0;

	if (distanceNorm < 0.0f)
		distanceNorm += 1.0f;
	else if (distanceNorm > 1.0f)
		distanceNorm -= 1.0f;

	const size_t pointId = (size_t)(tclamp(distanceNorm, 0.0f, 1.0f) * (float)(numPoints - 1));
	return pointId;
}

vec3f Track::getTrackDirectionAtDistance(float distanceNorm) const
{
	const size_t pointId = getPointIdAtDistance(distanceNorm);
	if (pointId < fatPoints.size())
	{
		return fatPoints[pointId].forwardDir;
	}

	return vec3f(0, 0, 0);
}

}
