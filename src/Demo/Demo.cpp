#include "PlaygrounD.h"

using namespace D;

int main(int argc, char** argv)
{
	log_printf(L"ENTER main");

	#ifndef DEBUG
	try {
	#endif

	auto app = std::make_unique<PlaygrounD>();
	app->runDemo();

	#ifndef DEBUG
	} catch (const std::exception& ex) {
		log_printf(L"main: UNHANDLED EXCEPTION: %S", ex.what());
	}
	#endif

	log_printf(L"LEAVE main");
	return 0;
}
