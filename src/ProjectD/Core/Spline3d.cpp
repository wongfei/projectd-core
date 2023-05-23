#include "Core/Spline3d.h"
#include "Core/Diag.h"
#include "Core/DebugGL.h"

namespace D {

inline int wrap(int id, int n) { return id < n ? id : id - n; }

//=============================================================================

void Spline3d::clear()
{
	_points.clear();
	_nodes.clear();
	_distances.clear();
}

void Spline3d::add_node(const vec3f& node)
{
	_nodes.push_back(node);

	if (_nodes.size() == 1)
	{
		_distances.push_back(0);
	}
	else
	{
		const size_t pt = _nodes.size() - 1;
		const float segment_distance = (_nodes[pt] - _nodes[pt - 1]).len();
		_distances.push_back(segment_distance + _distances[pt - 1]);
	}
}

bool Spline3d::find_nearest_point(const vec3f& pos, Spline3dPointInfo& info, int seg1, int seg2) // TODO: use quick search
{
	int best_id = 0;
	float best_dist = FLT_MAX;
	float spline_dist = 0;
	vec3f best_pos(0, 0, 0);
	bool found = false;

	const int npoints = (int)_nodes.size();

	if (seg1 >= npoints) seg1 = 0;
	if (seg2 >= npoints) seg2 = 0;

	if (!seg1 && !seg2)
		seg2 = npoints;

	int id = seg1;
	while (id != seg2)
	{
		const auto& pt = _nodes[id];
		const float d = (pos - pt).sqlen();
		if (best_dist >= d)
		{
			best_dist = d;
			best_pos = pt;
			best_id = id;
			spline_dist = length_at_point(id);
			found = true;
		}

		//DebugGL::get().line(pos, pt, vec3f(1, 0, 0));

		++id;
		if (id >= npoints) id = 0;
	}

	info.id = best_id;
	info.dist = spline_dist;
	info.pos = best_pos;

	return found;
}

//=============================================================================

void BSpline3d::init_from_array(std::vector<vec3f>& points, bool closed_loop)
{
	clear();

	const int npoints = (int)points.size();
	if (npoints < 4)
	{
		SHOULD_NOT_REACH_WARN;
		return;
	}

	std::swap(_points, points);
	_closed_loop = closed_loop;

	#if 1
	if (!closed_loop)
	{
		const float d0 = (_points[1] - _points[0]).len();
		const vec3f n0 = (_points[1] - _points[0]) / d0;

		for (int i = 0; i < _steps; i++)
		{
			float u = (float)i / (float)_steps;
			add_node(_points[0] + n0 * (u * d0));
		}
	}
	#endif

	#if 1
	for (int pt = 0; pt + 4 < npoints; ++pt)
	{
		for (int i = 0; i < _steps; i++)
		{
			float u = (float)i / (float)_steps;
			add_node(interpolate(u, _points[pt], _points[pt + 1], _points[pt + 2], _points[pt + 3]));
		}
	}
	#endif

	if (closed_loop)
	{
		#if 1
		for (int pt = npoints - 4; pt < npoints; ++pt)
		{
			for (int i = 0; i < _steps; i++)
			{
				float u = (float)i / (float)_steps;
				add_node(interpolate(u, _points[pt], _points[wrap(pt + 1, npoints)], _points[wrap(pt + 2, npoints)], _points[wrap(pt + 3, npoints)]));
			}
		}
		#endif
	}
	else
	{
		#if 1
		const int tail_off = 3;
		const int tail = npoints - tail_off;

		for (int pt = tail; pt + 1 < npoints; ++pt)
		{
			const float dx = (_points[pt + 1] - _points[pt]).len();
			const vec3f nx = (_points[pt + 1] - _points[pt]) / dx;

			for (int i = 0; i < _steps; i++)
			{
				float u = (float)i / (float)_steps;
				add_node(_points[pt] + nx * (u * dx));
			}
		}

		add_node(_points[npoints - 1]);
		#endif
	}
}

vec3f BSpline3d::interpolate(float u, const vec3f& P0, const vec3f& P1, const vec3f& P2, const vec3f& P3)
{
	vec3f point;
	point = u * u * u * ((-1.0f) * P0 + 3.0f * P1 - 3.0f * P2 + P3) / 6.0f;
	point += u * u * (3.0f * P0 - 6.0f * P1 + 3.0f * P2) / 6.0f;
	point += u * (-3.0f * P0 + 3.0f * P2) / 6.0f;
	point += (P0 + 4.0f * P1 + P2) / 6.0f;
	return point;
} 

}
