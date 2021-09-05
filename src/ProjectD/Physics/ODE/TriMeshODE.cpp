#include "Physics/ODE/TriMeshODE.h"

namespace D {

TriMeshODE::TriMeshODE()
{
	//TRACE_CTOR(TriMeshODE);
}

TriMeshODE::~TriMeshODE()
{
	//TRACE_DTOR(TriMeshODE);
}

void TriMeshODE::resize(size_t vertexCount, size_t indexCount)
{
	GUARD_FATAL(vertexCount > 0);
	GUARD_FATAL(indexCount > 0);

	vertices.resize(vertexCount);
	indices.resize(indexCount);
}

TriMeshVertex* TriMeshODE::getVB()
{
	return vertices.data();
}

size_t TriMeshODE::getVertexCount()
{
	return vertices.size();
}

TriMeshIndex* TriMeshODE::getIB()
{
	return indices.data();
}

size_t TriMeshODE::getIndexCount()
{
	return indices.size();
}

}
