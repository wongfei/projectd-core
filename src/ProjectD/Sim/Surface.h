#pragma once

#include "Core/Core.h"

namespace D {

struct Surface : public virtual IObject
{
	ITriMeshPtr trimesh;
	uint32_t sectorID = 0;
	uint32_t collisionCategory = 0;
	float gripMod = 0;
	float damping = 0;
	float sinHeight = 0;
	float sinLength = 0;
	float granularity = 0;
	float dirtAdditiveK = 0;
	float vibrationGain = 0;
	float vibrationLength = 0;
	float wavPitchSpeed = 0;
	uint8_t isValidTrack = 0;
	uint8_t isPitlane = 0;
};

#pragma pack(push, 1)
struct BlobSurface
{
	uint32_t magic = 0;
	uint32_t numVertices = 0;
	uint32_t numIndices = 0;
	uint32_t sectorID = 0;
	uint32_t collisionCategory = 0;
	float gripMod = 0;
	float damping = 0;
	float sinHeight = 0;
	float sinLength = 0;
	float granularity = 0;
	float dirtAdditiveK = 0;
	float vibrationGain = 0;
	float vibrationLength = 0;
	float wavPitchSpeed = 0;
	uint8_t isValidTrack = 0;
	uint8_t isPitlane = 0;
};
#pragma pack(pop)

}
