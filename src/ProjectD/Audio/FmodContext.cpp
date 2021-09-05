#include "Audio/FmodContext.h"

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
	FMOD::Studio::System::create(&system);
	GUARD_FATAL(system);

	auto rc = system->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
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

void FmodContext::loadBank(const std::string& fileName)
{
	GUARD_FATAL(system);

	log_printf(L"loadBank \"%S\"", fileName.c_str());

	FMOD::Studio::Bank* bank = nullptr;
	auto rc = system->loadBankFile(fileName.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
	GUARD_FATAL(rc == FMOD_OK);

	banks.emplace_back(bank);
}

void FmodContext::enumerate()
{
	FMOD_RESULT rc;
	const size_t PathLen = 1024;
	char path[PathLen] = {0};

	eventDescMap.clear();

	for (auto* bank : banks)
	{
		int stringCount = 0;
		rc = bank->getStringCount(&stringCount);
		GUARD_FATAL(rc == FMOD_OK);

		for (int stringId = 0; stringId < stringCount; ++stringId)
		{
			FMOD_GUID guid;
			rc = bank->getStringInfo(stringId, &guid, path, PathLen, nullptr);
			GUARD_FATAL(rc == FMOD_OK);

			//log_printf(L"String: %d %S", stringId, path);
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
				auto e = events[eventId];

				rc = e->getPath(path, PathLen, nullptr);
				GUARD_FATAL(rc == FMOD_OK);

				//log_printf(L"Event: %d %S", eventId, path);
				eventDescMap.insert({std::string(path), e});

				int paramCount = 0;
				e->getParameterCount(&paramCount);
				for (int paramId = 0; paramId < paramCount; ++paramId)
				{
					FMOD_STUDIO_PARAMETER_DESCRIPTION desc;
					rc = e->getParameterByIndex(paramId, &desc);
					GUARD_FATAL(rc == FMOD_OK);

					//log_printf(L"\tParam: %d %S", paramId, desc.name);
				}
			}
		}
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
