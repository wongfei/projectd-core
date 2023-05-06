#include "PlaygrounD.h"
#include "Core/OS.h"

using namespace D;

int main(int argc, char** argv)
{
	log_printf(L"ENTER main");

	#ifndef DEBUG
	try {
	#endif

	auto exePath = osGetModuleFullPath();
	auto exeDir = osGetDirPath(exePath);
	auto baseDir = osCombinePath(exeDir, L"..\\");

	replace(baseDir, L'\\', L'/');
	if (!ends_with(baseDir, L'/')) { baseDir.append(L"/"); }

	const std::wstring logPath = baseDir + L"projectd.log";
	log_set_file(logPath.c_str(), true);

	auto app = std::make_unique<PlaygrounD>();
	app->run(baseDir, false, true);

	#ifndef DEBUG
	} catch (const std::exception& ex) {
		log_printf(L"main: UNHANDLED EXCEPTION: %S", ex.what());
	}
	#endif

	log_printf(L"LEAVE main");
	return 0;
}
