#pragma once

#include "Core/Math.h"
#include "Core/Curve.h"
#include <map>

namespace D {

struct INISection
{
	std::map<std::wstring, std::wstring> keys;
};

struct INIReader : public NonCopyable
{
	static std::map<std::wstring, std::wstring> _iniCache;
	static void flushCache();

	INIReader();
	~INIReader();

	INIReader(const std::wstring& filename);
	bool load(const std::wstring& filename);

	bool hasSection(const std::wstring& section) const;
	bool hasKey(const std::wstring& section, const std::wstring& key) const;

	const std::wstring& getString(const std::wstring& section, const std::wstring& key, bool required = true) const;
	int getInt(const std::wstring& section, const std::wstring& key, bool required = true) const;
	float getFloat(const std::wstring& section, const std::wstring& key, bool required = true) const;
	vec2f getFloat2(const std::wstring& section, const std::wstring& key, bool required = true) const;
	vec3f getFloat3(const std::wstring& section, const std::wstring& key, bool required = true) const;
	vec4f getFloat4(const std::wstring& section, const std::wstring& key, bool required = true) const;
	Curve getCurve(const std::wstring& section, const std::wstring& key, bool required = true) const;

	bool getInt(const std::wstring& section, const std::wstring& key, int& outValue) const;
	bool getFloat(const std::wstring& section, const std::wstring& key, float& outValue) const;

	std::wstring dataFilename;
	std::wstring data;
	std::map<std::wstring, INISection> sections;
	bool ready = false;
};

}
