#pragma once

#include "Physics/PhysicsCommon.h"

namespace D {

struct TriMeshVertex
{
	float x, y, z;
};

using TriMeshIndex = unsigned short;

struct ITriMesh : public virtual IObject
{
	virtual void resize(size_t vertexCount, size_t indexCount) = 0;
	virtual TriMeshVertex* getVB() = 0;
	virtual size_t getVertexCount() = 0;
	virtual TriMeshIndex* getIB() = 0;
	virtual size_t getIndexCount() = 0;
};

DECL_SHARED_PTR(ITriMesh);

}
