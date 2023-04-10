#include "Sim/Track.h"
#include "Sim/Simulator.h"

namespace D {

Track::Track(SimulatorPtr _sim)
	:
	sim(_sim)
{
	TRACE_CTOR(Track);
}

Track::~Track()
{
	TRACE_DTOR(Track);

	sim->unregisterTrack(this);
}

bool Track::init(const std::wstring& trackName)
{
	log_printf(L"Track: init: trackName=\"%s\"", trackName.c_str());

	name = trackName;
	dataFolder = sim->basePath + L"content/tracks/" + name + L"/";
	log_printf(L"dataFolder=\"%s\"", dataFolder.c_str());

	loadSurfaceBlob();
	loadPits();

	return true;
}

void Track::loadSurfaceBlob()
{
	colliders.clear();
	surfaces.clear();

	FileHandle file;
	auto strPath = dataFolder + L"surfaces.bin";
	GUARD_FATAL(file.open(strPath.c_str(), L"rb"));

	BlobSurface blob;
	while (fread(&blob, sizeof(blob), 1, file.fd) == 1)
	{
		GUARD_FATAL(blob.magic == 0xAABBCCDD);
		GUARD_FATAL(blob.numVertices > 0 && blob.numIndices > 0);

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

		log_printf(L"Surface: sectorID=%u collisionCategory=%u gripMod=%.3f damping=%.3f", pSurf->sectorID, pSurf->collisionCategory, pSurf->gripMod, pSurf->damping);

		auto pCollider = sim->physics->createCollider(pSurf->trimesh, false, pSurf->sectorID, pSurf->collisionCategory, C_MASK_SURFACE);
		pCollider->setUserPointer(pSurf.get());

		if (!(pSurf->collisionCategory == C_CATEGORY_TRACK || pSurf->collisionCategory == C_CATEGORY_WALL))
		{
			log_printf(L"UNKNOWN collisionCategory=%d", pSurf->collisionCategory);
		}

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

void Track::step(float dt) // TODO?
{
}

}
