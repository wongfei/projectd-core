#pragma once

#include "Core/Common.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fmod_studio.hpp>

namespace D {

struct FmodContext : public virtual IObject
{
	FmodContext();
	~FmodContext();

	void init(const std::string& basePath);
	void loadGUIDs(const std::string& fileName);
	void loadBank(const std::string& fileName);
	void enumerateBankEvents(FMOD::Studio::Bank* bank, bool debug = false);
	void update();
	FMOD::Studio::EventDescription* getEventDesc(const std::string& name);
	FMOD::Studio::EventInstance* getUniqInstance(const std::string& name);

	FMOD::Studio::System* system = nullptr;
	std::set<std::string> loadedFiles;
	std::vector<FMOD::Studio::Bank*> banks;
	std::map<std::string, std::string> guidToEventMap;
	std::map<std::string, FMOD::Studio::EventDescription*> eventDescMap;
	std::map<std::string, FMOD::Studio::EventInstance*> eventInstMap;
	bool printEventDebugInfo = false;
};

}
