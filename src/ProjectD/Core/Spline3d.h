#pragma once

#include "Core/Math.h"
#include <vector>

// https://github.com/chen0040/cpp-spline

namespace D {

struct Spline3dPointInfo
{
	int id = 0;
	float dist = 0;
	vec3f pos;
};

struct Spline3d
{
	std::vector<vec3f> _points;
	std::vector<vec3f> _nodes;
	std::vector<float> _distances;
	int _steps = 100;
	bool _closed_loop = false;
	
	Spline3d() {}
	~Spline3d() {}

	void clear();
	void add_node(const vec3f& node);
	bool find_nearest_point(const vec3f& pos, Spline3dPointInfo& info, int seg1 = 0, int seg2 = 0);

	inline bool is_empty() const { return _nodes.empty(); }
	inline int node_count() const {  return (int)_nodes.size(); }
	inline const vec3f& node(int i) const { return _nodes[i]; }
	
	inline float length_at_point(int i) const { return _distances[i]; }
	inline float total_length() const { return _distances[_distances.size() - 1]; }

	inline void set_steps(int steps) { _steps = steps; }
}; 

struct BSpline3d : public Spline3d
{
	BSpline3d() {}
	~BSpline3d() {}

	void init_from_array(std::vector<vec3f>& points, bool closed_loop);
	vec3f interpolate(float u, const vec3f& P0, const vec3f& P1, const vec3f& P2, const vec3f& P3);
}; 

}
