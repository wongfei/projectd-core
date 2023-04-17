#include "Audio/FmodContext.h"
#include <fstream>
#include <sstream>

extern "C" 
{
    typedef FMOD_DSP_DESCRIPTION* (F_CALL *FMODGetDSPDescription_Proto)();
}

namespace D {

FmodContext::FmodContext()
{
	TRACE_CTOR(FmodContext);
}

FmodContext::~FmodContext()
{
	TRACE_DTOR(FmodContext);

	if (system)
	{
		system->unloadAll();
		system->flushCommands();
		system->release();
		system = nullptr;
	}
}

void FmodContext::init()
{
	FMOD_RESULT rc;

	unsigned int debugFlags = FMOD_DEBUG_LEVEL_LOG;

	#if defined(DEBUG)
		debugFlags |= (FMOD_DEBUG_TYPE_TRACE | FMOD_DEBUG_TYPE_CODEC);
		//debugFlags |= (FMOD_DEBUG_TYPE_MEMORY | FMOD_DEBUG_TYPE_FILE);
	#endif

	rc = FMOD::Debug_Initialize(debugFlags, FMOD_DEBUG_MODE_FILE, nullptr, "fmod.log");

	rc = FMOD::Studio::System::create(&system);
	GUARD_FATAL(system);

	rc = system->initialize(128, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
	GUARD_FATAL(rc == FMOD_OK);

	auto plugin = osLoadLibraryW(L"fmod_distance_filter64.dll");
	GUARD_FATAL(plugin);

	auto getDesc = (FMODGetDSPDescription_Proto)osGetProcAddress(plugin, "FMODGetDSPDescription");
	GUARD_FATAL(getDesc);

	auto desc = getDesc();
	GUARD_FATAL(desc);

	rc = system->registerPlugin(desc);
	GUARD_FATAL(rc == FMOD_OK);
}

void FmodContext::loadGUIDs(const std::string& fileName)
{
	log_printf(L"loadGUIDs \"%S\"", fileName.c_str());

	if (!osFileExists(strw(fileName)))
	{
		log_printf(L"File not found");
		return;
	}

	std::ifstream infile(fileName);
	std::string line;

	while (std::getline(infile, line))
	{
		auto parts = split(line, " ");
		if (parts.size() == 2)
		{
			if (guidToEventMap.find(parts[0]) == guidToEventMap.end())
			{
				guidToEventMap.insert({ parts[0], parts[1] });
			}
		}
	}
}

void FmodContext::clearGUIDs()
{
	guidToEventMap.clear();
}

void FmodContext::loadBank(const std::string& fileName)
{
	GUARD_FATAL(system);

	log_printf(L"loadBank \"%S\"", fileName.c_str());

	if (!osFileExists(strw(fileName)))
	{
		log_printf(L"File not found");
		return;
	}

	FMOD::Studio::Bank* bank = nullptr;
	auto rc = system->loadBankFile(fileName.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
	GUARD_FATAL(rc == FMOD_OK);

	banks.emplace_back(bank);
}

void FmodContext::enumerate(bool debug)
{
	FMOD_RESULT rc;
	const size_t PathLen = 1024;
	char path[PathLen] = {0};

	eventDescMap.clear();

	int bankId = 0;
	for (auto* bank : banks)
	{
		if (debug)
			log_printf(L"BANK: %d", bankId);

		int stringCount = 0;
		rc = bank->getStringCount(&stringCount);
		GUARD_FATAL(rc == FMOD_OK);

		for (int stringId = 0; stringId < stringCount; ++stringId)
		{
			FMOD_GUID guid;
			rc = bank->getStringInfo(stringId, &guid, path, PathLen, nullptr);
			GUARD_FATAL(rc == FMOD_OK);

			if (debug)
				log_printf(L"  String: %d %S", stringId, path);
		}

		int eventCount = 0;
		rc = bank->getEventCount(&eventCount);
		GUARD_FATAL(rc == FMOD_OK);

		if (eventCount != 0)
		{
			std::vector<FMOD::Studio::EventDescription*> events;
			events.resize(eventCount);

			rc = bank->getEventList(events.data(), eventCount, &eventCount);
			GUARD_FATAL(rc == FMOD_OK);

			for (int eventId = 0; eventId < eventCount; ++eventId)
			{
				if (debug)
					log_printf(L"  Event: %d", eventId);

				auto e = events[eventId];

				FMOD_GUID eID = {};
				rc = e->getID(&eID);
				if (rc == FMOD_OK)
				{
					std::string guidStr = straf("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
						eID.Data1, eID.Data2, eID.Data3,
						eID.Data4[0], eID.Data4[1], eID.Data4[2], eID.Data4[3], eID.Data4[4], eID.Data4[5], eID.Data4[6], eID.Data4[7]);

					if (debug)
						log_printf(L"    EventGUID: %S", guidStr.c_str());

					auto iter = guidToEventMap.find(guidStr);
					if (iter != guidToEventMap.end())
					{
						if (debug)
							log_printf(L"    EventPath: %S", iter->second.c_str());

						eventDescMap.insert({iter->second, e});
					}
				}

				#if 0
				int retrieved = 0;
				rc = e->getPath(path, PathLen, &retrieved);
				if (rc == FMOD_OK)
				{
					log_printf(L"    EventPath: %S", path);
					eventDescMap.insert({std::string(path), e});
				}
				else
				{
					//log_printf(L"ERROR: EventDescription::getPath failed: eventId=%d", eventId);
				}
				#endif

				int paramCount = 0;
				rc = e->getParameterCount(&paramCount);
				for (int paramId = 0; paramId < paramCount; ++paramId)
				{
					FMOD_STUDIO_PARAMETER_DESCRIPTION desc;
					rc = e->getParameterByIndex(paramId, &desc);
					GUARD_FATAL(rc == FMOD_OK);

					if (debug)
						log_printf(L"    EventParam: %d %S", paramId, desc.name);
				}
			}
		}

		++bankId;
	}
}

void FmodContext::update()
{
	if (system)
	{
		auto rc = system->update();
		GUARD_FATAL(rc == FMOD_OK);
	}
}

FMOD::Studio::EventDescription* FmodContext::getEventDesc(const std::string& name)
{
	auto iter = eventDescMap.find(name);
	if (iter != eventDescMap.end())
		return iter->second;

	SHOULD_NOT_REACH_WARN;
	return nullptr;
}

FMOD::Studio::EventInstance* FmodContext::getUniqInstance(const std::string& name)
{
	auto iter = eventInstMap.find(name);
	if (iter != eventInstMap.end())
		return iter->second;

	auto e = getEventDesc(name);
	if (e)
	{
		FMOD::Studio::EventInstance* inst = nullptr;
		if (e->createInstance(&inst) == FMOD_OK)
		{
			eventInstMap.insert({name, inst});
			return inst;
		}
	}

	SHOULD_NOT_REACH_WARN;
	return nullptr;
}

}
