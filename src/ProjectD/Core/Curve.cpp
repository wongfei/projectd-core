#include "Core/Curve.h"
#include "Core/String.h"
#include "Core/Diag.h"
#include <fstream>

namespace D {

Curve::Curve()
{}

Curve::~Curve()
{}

Curve::Curve(const Curve& other)
{
	init(other);
}

Curve& Curve::operator=(const Curve& other)
{
	init(other);
	return *this;
}

Curve::Curve(Curve&& other) noexcept
{
	init(other);
}

Curve& Curve::operator=(Curve&& other) noexcept
{
	init(other);
	return *this;
}

void Curve::init(const Curve& other)
{
	references = other.references;
	values = other.values;
	splineReady = false;
}

void Curve::init(Curve&& other) noexcept
{
	references = std::move(other.references);
	values = std::move(other.values);
	splineReady = false;
}

void Curve::reset()
{
	references.clear();
	values.clear();
	splineReady = false;
}

void Curve::addValue(float ref, float val)
{
	references.emplace_back(ref);
	values.emplace_back(val);
	splineReady = false;
}

void Curve::scale(float scale)
{
	for (auto& v : values)
		v *= scale;
	splineReady = false;
}

int Curve::getCount() const
{
	return (int)references.size();
}

float Curve::getMaxReference() const
{
	if (!references.empty())
		return references.back();

	SHOULD_NOT_REACH_WARN;
	return 0;
}

std::pair<float, float> Curve::getPairAtIndex(int index) const
{
	if (index >= 0 && index < (int)references.size())
		return {references[index], values[index]};

	SHOULD_NOT_REACH_WARN;
	return {0.0f, 0.0f};
}

float Curve::getValue(float ref)
{
	const auto& R = references;
	const auto& V = values;

	if (R.empty())
		return 0.0f;

	if (ref <= R[0])
		return V[0];

	const size_t n = R.size();
	for (size_t id = 1; id < n; ++id)
	{
		if (ref <= R[id])
		{
			return (((V[id] - V[id - 1]) * (ref - R[id - 1])) / (R[id] - R[id - 1])) + V[id - 1];
		}
	}

	return V[n - 1];
}

float Curve::getCubicSplineValue(float ref)
{
	if (!splineReady)
	{
		spline.setPoints(references, values);
		splineReady = true;
	}

	return spline.getValue(ref);
}

bool Curve::load(const std::wstring& filename)
{
	log_printf(L"Curve: load: \"%s\"", filename.c_str());

	reset();

	std::wifstream file(filename);
	if (!file.is_open())
	{
		SHOULD_NOT_REACH_WARN;
		return false;
	}

	std::wstring line;
	while (std::getline(file, line))
	{
		auto comm = line.find(L';');
		if (comm != std::wstring::npos)
		{
			line = line.substr(0, comm);
		}

		// "0|191"
		// "500|272"
		// ...
		if (!line.empty())
		{
			auto kv = split(line, L"|");
			if (kv.size() == 2)
			{
				addValue(std::stof(kv[0]), std::stof(kv[1]));
			}
			else
			{
				SHOULD_NOT_REACH_WARN;
			}
		}
	}

	return !references.empty();
}

bool Curve::parseInline(const std::wstring& str)
{
	reset();

	// "(0=0.0|4000=0.0|4500=1.5)" OR "(|0=0.0|4000=0.0|4500=1.5|)"
	auto p1 = str.find(L"(");
	auto p2 = str.find(L")");
	if (p1 != std::wstring::npos && p2 != std::wstring::npos && p1 < p2)
	{
		p1++;
		auto inner = str.substr(p1, p2 - p1);
		auto pairs = split(inner, L"|");
		for (auto& pair : pairs)
		{
			if (!pair.empty())
			{
				auto kv = split(pair, L"=");
				if (kv.size() == 2)
				{
					addValue(std::stof(kv[0]), std::stof(kv[1]));
				}
				else
				{
					SHOULD_NOT_REACH_WARN;
				}
			}
		}
	}
	
	return !references.empty();
}

}
