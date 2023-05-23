#include "Core/INIReader.h"
#include "Core/String.h"
#include "Core/Diag.h"
#include "Core/OS.h"
#include <sstream>
#include <fstream>

namespace D {

std::map<std::wstring, std::wstring> INIReader::_iniCache;
bool INIReader::_debug = false;

void INIReader::flushCache()
{
	_iniCache.clear();
}

INIReader::INIReader()
{
}

INIReader::~INIReader()
{
}

INIReader::INIReader(const std::wstring& filename)
{
	load(filename);
}

bool INIReader::load(const std::wstring& filename)
{
	if (_debug)
		log_printf(L"INIReader: load: \"%s\"", filename.c_str());

	if (ready && (dataFilename == filename))
	{
		//log_printf(L"already loaded");
		return true;
	}

	ready = false;
	dataFilename = filename;
	data.clear();
	sections.clear();

	auto iter = _iniCache.find(filename);
	if (iter == _iniCache.end()) // not cached
	{
		std::wifstream fs(filename);
		if (!fs.is_open())
		{
			SHOULD_NOT_REACH_WARN;
			return false;
		}

		std::wstringstream ss;
		ss << fs.rdbuf();
		data = ss.str();

		_iniCache.insert({filename, data});
	}
	else
	{
		data = iter->second;
	}

	std::wistringstream ss(data);
	std::wstring line;
	std::wstring secName;
	INISection* sec = nullptr;

	while (std::getline(ss, line))
	{
		auto comm = line.find(L';');
		auto s1 = line.find(L'[');
		if (s1 != std::wstring::npos) // [section_name] ; comment
		{
			s1++;
			auto s2 = line.find(L']');
			if (s2 != std::wstring::npos && s2 > s1 && s2 < comm)
			{
				secName = line.substr(s1, s2 - s1);
				auto result = sections.insert({secName, INISection()});
				sec = &(result.first->second);
			}
		}
		else if (!secName.empty()) // key=value ; comment
		{
			auto split = line.find(L'=');
			if (split != std::wstring::npos && split > 0 && split < comm)
			{
				auto eol = line.find_first_of(L";\n", split);
				if (eol != std::wstring::npos)
					eol -= split;

				auto key = line.substr(0, split);
				auto val = line.substr(split + 1, eol);
				sec->keys.insert({key, val});
			}
		}
	}

	//log_printf(L"INIReader: load: DONE");
	ready = true;
	return true;
}

bool INIReader::hasSection(const std::wstring& section) const
{
	return sections.find(section) != sections.end();
}

bool INIReader::hasKey(const std::wstring& section, const std::wstring& key) const
{
	auto isec = sections.find(section);
	if (isec != sections.end())
	{
		auto& keys = isec->second.keys;
		auto ikey = keys.find(key);
		return (ikey != keys.end());
	}
	return false;
}

void INIReader::traceKeyNotFound(const std::wstring& section, const std::wstring& key) const
{
	if (_debug)
		log_printf(L"ERROR: INIReader: not found: [%s] -> %s", section.c_str(), key.c_str());
}

static const std::wstring _empty;

const std::wstring& INIReader::getString(const std::wstring& section, const std::wstring& key, bool required) const
{
	auto isec = sections.find(section);
	if (isec != sections.end())
	{
		auto& keys = isec->second.keys;
		auto ikey = keys.find(key);
		if (ikey != keys.end())
			return ikey->second;
	}
	if (required)
		traceKeyNotFound(section, key);
	return _empty;
}

int INIReader::getInt(const std::wstring& section, const std::wstring& key, bool required) const
{
	auto s = getString(section, key, required);
	if (!s.empty())
		return std::stoi(s);
	return 0;
}

float INIReader::getFloat(const std::wstring& section, const std::wstring& key, bool required) const
{
	auto s = getString(section, key, required);
	if (!s.empty())
		return std::stof(s);
	return 0;
}

bool INIReader::tryGetString(const std::wstring& section, const std::wstring& key, std::wstring& outValue) const
{
	if (hasKey(section, key))
	{
		outValue = getString(section, key, true);
		return true;
	}
	return false;
}

bool INIReader::tryGetInt(const std::wstring& section, const std::wstring& key, int& outValue) const
{
	if (hasKey(section, key))
	{
		outValue = getInt(section, key, true);
		return true;
	}
	return false;
}

bool INIReader::tryGetFloat(const std::wstring& section, const std::wstring& key, float& outValue) const
{
	if (hasKey(section, key))
	{
		outValue = getFloat(section, key, true);
		return true;
	}
	return false;
}

bool INIReader::tryGetFloat(const std::wstring& section, const std::wstring& key, double& outValue) const
{
	if (hasKey(section, key))
	{
		outValue = getFloat(section, key, true);
		return true;
	}
	return false;
}

vec2f INIReader::getFloat2(const std::wstring& section, const std::wstring& key, bool required) const
{
	auto s = getString(section, key, required);
	auto v = split(s, L",");
	if (v.size() == 2)
		return vec2f(std::stof(v[0]), std::stof(v[1]));
	return vec2f{};
}

vec3f INIReader::getFloat3(const std::wstring& section, const std::wstring& key, bool required) const
{
	auto s = getString(section, key, required);
	auto v = split(s, L",");
	if (v.size() == 3)
		return vec3f(std::stof(v[0]), std::stof(v[1]), std::stof(v[2]));
	return vec3f{};
}

vec4f INIReader::getFloat4(const std::wstring& section, const std::wstring& key, bool required) const
{
	auto s = getString(section, key, required);
	auto v = split(s, L",");
	if (v.size() == 4)
		return vec4f{std::stof(v[0]), std::stof(v[1]), std::stof(v[2]), std::stof(v[3])};
	return vec4f{};
}

Curve INIReader::getCurve(const std::wstring& section, const std::wstring& key, bool required) const
{
	Curve curve;
	auto keyval = getString(section, key, required);

	if (keyval.find(L".lut") != std::wstring::npos)
	{
		auto path = osGetDirPath(dataFilename) + keyval;
		curve.load(path);
	}
	else
	{
		auto p1 = keyval.find(L"(");
		auto p2 = keyval.find(L")");
		if (p1 != std::wstring::npos && p2 != std::wstring::npos && p1 < p2)
		{
			curve.parseInline(keyval);
		}
	}

	return curve;
}

}
