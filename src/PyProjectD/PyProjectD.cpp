// https://pybind11.readthedocs.io/en/latest/basics.html
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include "PlaygrounD.h"
#include "Core/OS.h"

#include <unordered_map>
#include <thread>
#include <mutex>

// TODO: lame globals

static std::mutex g_simMux;
#define SIM_LOCK std::lock_guard<std::mutex> lock(g_simMux)

static std::atomic<int> g_uniqSimId;
static std::unordered_map<int, D::SimulatorPtr> g_simMap;

struct PySimulatorManager : public D::ISimulatorManager
{
	PySimulatorManager() { TRACE_CTOR(PySimulatorManager); }
	~PySimulatorManager() { TRACE_DTOR(PySimulatorManager); }

	int getSimCount() const override
	{
		SIM_LOCK;
		return (int)g_simMap.size();
	}

	D::SimulatorPtr getSim(int simId) override
	{
		SIM_LOCK;
		auto iter = g_simMap.find(simId);
		if (iter != g_simMap.end())
			return iter->second;
		return nullptr;
	}
};
static PySimulatorManager g_simManager;

static std::unique_ptr<D::PlaygrounD> g_playground;

//
// CORE
//

void setSeed(unsigned int seed)
{
	srand(seed);
}

void setLogFile(const std::string& path, bool overwrite = true)
{
	D::log_set_file(D::strw(path).c_str(), overwrite);
}

void clearLogFile()
{
	D::log_clear_file();
}

void writeLog(const std::string& msg)
{
	D::log_printf(L"%S", msg.c_str());
}

//
// SIMULATOR
//

inline D::Simulator* getSimulator(int simId)
{
	auto iter = g_simMap.find(simId);
	if (iter != g_simMap.end())
	{
		return iter->second.get();
	}

	//D::log_printf(L"FAILED: getSimulator simId=%d", simId);
	//SHOULD_NOT_REACH_FATAL;
	return nullptr;
}

inline D::SimulatorPtr getSimulatorShared(int simId)
{
	auto iter = g_simMap.find(simId);
	if (iter != g_simMap.end())
	{
		return iter->second;
	}

	//D::log_printf(L"FAILED: getSimulatorShared simId=%d", simId);
	//SHOULD_NOT_REACH_FATAL;
	return nullptr;
}

inline D::Car* getCar(int simId, int carId)
{
	auto* sim = getSimulator(simId);
	if (sim)
		return sim->getCar(carId);

	//D::log_printf(L"FAILED: getCar simId=%d carId=%d", simId, carId);
	//SHOULD_NOT_REACH_FATAL;
	return nullptr;
}

int createSimulator(const std::string& basePath)
{
	try
	{
		//D::INIReader::_debug = true;

		const int id = g_uniqSimId++;

		D::log_printf(L"[PY] createSimulator simId=%d", id);

		auto sim = std::make_shared<D::Simulator>();
		sim->simulatorId = id;
		sim->init(D::strw(basePath));

		{
			SIM_LOCK;
			g_simMap.insert({id, sim});
		}

		return id;
	}
	catch (const std::exception& ex)
	{
		D::log_printf(L"EXCEPTION: %S", ex.what());
	}
	return -1;
}

void destroySimulator(int simId)
{
	D::log_printf(L"[PY] destroySimulator simId=%d", simId);

	SIM_LOCK;
	auto iter = g_simMap.find(simId);
	if (iter != g_simMap.end())
	{
		g_simMap.erase(iter);
	}
}

void destroyAllSimulators()
{
	D::log_printf(L"[PY] destroyAllSimulators");

	SIM_LOCK;
	g_simMap.clear();
	g_uniqSimId = 0;
}

void stepSimulator(int simId, double dt = (1.0 / 333.0))
{
	auto* sim = getSimulator(simId);
	if (sim && sim->physics)
	{
		sim->step((float)dt, sim->physicsTime, sim->gameTime);

		if (g_playground && g_playground->sim_.get() == sim)
		{
			g_playground->updateSimStats((float)dt, (float)sim->gameTime);
		}

		sim->physicsTime += dt;
		sim->gameTime += dt;
	}
}

//
// TRACK
//

void loadTrack(int simId, const std::string &trackName)
{
	D::log_printf(L"[PY] loadTrack simId=%d trackName=%S", simId, trackName.c_str());

	try
	{
		auto* sim = getSimulator(simId);
		if (sim && sim->physics)
		{
			sim->loadTrack(D::strw(trackName));
		}
	}
	catch (const std::exception& ex)
	{
		D::log_printf(L"EXCEPTION: %S", ex.what());
	}
}

void unloadTrack(int simId)
{
	D::log_printf(L"[PY] unloadTrack simId=%d", simId);

	auto* sim = getSimulator(simId);
	if (sim)
	{
		sim->unloadTrack();
	}
}

//
// CAR
//

int addCar(int simId, const std::string &modelName)
{
	D::log_printf(L"[PY] addCar simId=%d modelName=%S", simId, modelName.c_str());

	try
	{
		auto* sim = getSimulator(simId);
		if (sim)
		{
			auto* car = sim->addCar(D::strw(modelName));
			return car->physicsGUID;
		}
	}
	catch (const std::exception& ex)
	{
		D::log_printf(L"EXCEPTION: %S", ex.what());
	}
	return -1;
}

void removeCar(int simId, int carId)
{
	D::log_printf(L"[PY] removeCar simId=%d carId=%d", simId, carId);

	auto* sim = getSimulator(simId);
	if (sim)
	{
		sim->removeCar(carId);
	}
}

void teleportCarToLocation(int simId, int carId, float x, float y, float z)
{
	auto* car = getCar(simId, carId);
	if (car)
	{
		car->forcePosition(D::vec3f(x, y, z));
	}
}

void teleportCarToPits(int simId, int carId, int pitId)
{
	auto* car = getCar(simId, carId);
	if (car)
	{
		car->teleportToPits(pitId);
	}
}

void teleportCarToSpline(int simId, int carId, float distanceNorm)
{
	auto* car = getCar(simId, carId);
	if (car)
	{
		car->teleportToSpline(distanceNorm);
	}
}

void teleportCarByMode(int simId, int carId, int mode)
{
	auto* car = getCar(simId, carId);
	if (car)
	{
		car->teleportByMode((D::TeleportMode)mode);
	}
}

void setCarAutoTeleport(int simId, int carId, bool collision, bool badLoc, int teleportMode = (int)D::TeleportMode::Start)
{
	auto* car = getCar(simId, carId);
	if (car)
	{
		car->teleportOnCollision = collision;
		car->teleportOnBadLocation = badLoc;
		car->teleportMode = (int)teleportMode;
	}
}

void setCarControls(int simId, int carId, bool smooth, const D::CarControls& controls)
{
	auto* car = getCar(simId, carId);
	if (car)
	{
		car->controls = controls;
		car->smoothSteer = smooth;
	}
}

void getCarState(int simId, int carId, D::CarState& state)
{
	auto* car = getCar(simId, carId);
	if (car)
	{
		state = *(car->state.get());
	}
}

//
// PLAYGROUND
//

static std::unique_ptr<std::thread> g_playgroundThread;

void launchPlaygroundInOwnThread(const std::string& basePath)
{
	D::log_printf(L"[PY] launchPlaygroundInOwnThread");

	if (g_playgroundThread || g_playground)
		return;

	auto runner = [basePath]()
	{
		g_playground.reset(new D::PlaygrounD());
		g_playground->simManager_ = &g_simManager;
		g_playground->simEnabled_ = false;
		g_playground->run(D::strw(basePath), true, false);
	};

	g_playgroundThread.reset(new std::thread(runner));
}

void initPlayground(const std::string& basePath)
{
	D::log_printf(L"[PY] initPlayground");

	if (!g_playground)
	{
		g_playground.reset(new D::PlaygrounD());
		g_playground->simManager_ = &g_simManager;
		g_playground->init(D::strw(basePath), true);
	}
}

void shutPlayground()
{
	D::log_printf(L"[PY] shutPlayground");

	if (g_playground)
		g_playground->exitFlag_ = true;

	if (g_playgroundThread)
		g_playgroundThread->join();

	g_playgroundThread.reset();
	g_playground.reset();
}

void shutAll()
{
	D::log_printf(L"[PY] shutAll");

	shutPlayground();
	destroyAllSimulators();
}

void tickPlayground()
{
	if (g_playground && !g_playgroundThread)
	{
		g_playground->tick();
	}
}

bool isPlaygroundInitialized()
{
	return g_playground && g_playground->initializedFlag_;
}

bool isPlaygroundExited()
{
	return !g_playground || (g_playground && g_playground->exitFlag_);
}

void moveWindow(int x, int y)
{
	if (g_playground)
	{
		g_playground->moveWindow(x, y);
	}
}

void resizeWindow(int w, int h)
{
	if (g_playground)
	{
		g_playground->resizeWindow(w, h);
	}
}

void setRenderHz(int hz, bool enableSleep)
{
	if (g_playground)
	{
		g_playground->drawHz_ = hz;
		g_playground->enableSleep_ = enableSleep;
	}
}

void setActiveSimulator(int simId, bool simEnabled)
{
	D::log_printf(L"[PY] setActiveSimulator simId=%d simEnabled=%d", simId, (int)simEnabled);

	if (g_playground)
	{
		auto sim = getSimulatorShared(simId);
		if (sim)
		{
			g_playground->newSimId_ = simId;
			g_playground->simEnabled_ = simEnabled;
		}
	}
}

void setActiveCar(int carId, bool takeControls, bool enableSound)
{
	D::log_printf(L"[PY] setActiveCar carId=%d takeControls=%d enableSound=%d", carId, (int)takeControls, (int)enableSound);

	if (g_playground)
	{
		g_playground->newCarControls_ = takeControls;
		g_playground->newCarSound_ = enableSound;
		g_playground->newCarId_ = carId;
	}
}

int getActiveSimulator()
{
	if (g_playground && g_playground->sim_)
	{
		return g_playground->sim_->simulatorId;
	}
	return -1;
}

int getActiveCar()
{
	if (g_playground && g_playground->car_)
	{
		return g_playground->car_->physicsGUID;
	}
	return -1;
}

//
// MODULE
//

PYBIND11_MODULE(PyProjectD, m)
{
	m.doc() = "PyProjectD";

	py::class_<D::vec3f> py_vec3f(m, "vec3f");
	py_vec3f.def(py::init<>())
		.def_readwrite("x", &D::vec3f::x)
		.def_readwrite("y", &D::vec3f::y)
		.def_readwrite("z", &D::vec3f::z);

	py::class_<D::mat44f> py_mat44f(m, "mat44f");
	py_mat44f.def(py::init<>())
		.def_readwrite("M11", &D::mat44f::M11)
		.def_readwrite("M12", &D::mat44f::M12)
		.def_readwrite("M13", &D::mat44f::M13)
		.def_readwrite("M14", &D::mat44f::M14)
		.def_readwrite("M21", &D::mat44f::M21)
		.def_readwrite("M22", &D::mat44f::M22)
		.def_readwrite("M23", &D::mat44f::M23)
		.def_readwrite("M24", &D::mat44f::M24)
		.def_readwrite("M31", &D::mat44f::M31)
		.def_readwrite("M32", &D::mat44f::M32)
		.def_readwrite("M33", &D::mat44f::M33)
		.def_readwrite("M34", &D::mat44f::M34)
		.def_readwrite("M41", &D::mat44f::M41)
		.def_readwrite("M42", &D::mat44f::M42)
		.def_readwrite("M43", &D::mat44f::M43)
		.def_readwrite("M44", &D::mat44f::M44);

	py::class_<D::CarControls> py_CarControls(m, "CarControls");
	py_CarControls.def(py::init<>())
		.def_readwrite("steer", &D::CarControls::steer)
		.def_readwrite("clutch", &D::CarControls::clutch)
		.def_readwrite("brake", &D::CarControls::brake)
		.def_readwrite("handBrake", &D::CarControls::handBrake)
		.def_readwrite("gas", &D::CarControls::gas)
		.def_readwrite("isShifterSupported", &D::CarControls::isShifterSupported)
		.def_readwrite("requestedGearIndex", &D::CarControls::requestedGearIndex)
		.def_readwrite("gearUp", &D::CarControls::gearUp)
		.def_readwrite("gearDn", &D::CarControls::gearDn);

	py::class_<D::CarState> py_CarState(m, "CarState");
	py_CarState.def(py::init<>())
		.def_readonly("carId", &D::CarState::carId)
		.def_readonly("simId", &D::CarState::simId)
		.def_readonly("controls", &D::CarState::controls)

		.def_readonly("engineRPM", &D::CarState::engineRPM)
		.def_readonly("speedMS", &D::CarState::speedMS)
		.def_readonly("gear", &D::CarState::gear)
		.def_readonly("gearGrinding", &D::CarState::gearGrinding)

		.def_readonly("trackLocation", &D::CarState::trackLocation)
		.def_readonly("trackPointId", &D::CarState::trackPointId)
		.def_readonly("collisionFlag", &D::CarState::collisionFlag)
		.def_readonly("outOfTrackFlag", &D::CarState::outOfTrackFlag)

		.def_readonly("bodyVsTrack", &D::CarState::bodyVsTrack)
		.def_readonly("velocityVsTrack", &D::CarState::velocityVsTrack)
		.def_readonly("agentDriftReward", &D::CarState::agentDriftReward)

		.def_readonly("bodyMatrix", &D::CarState::bodyMatrix)
		.def_readonly("bodyPos", &D::CarState::bodyPos)
		.def_readonly("bodyEuler", &D::CarState::bodyEuler)
		.def_readonly("accG", &D::CarState::accG)
		.def_readonly("velocity", &D::CarState::velocity)
		.def_readonly("localVelocity", &D::CarState::localVelocity)
		.def_readonly("angularVelocity", &D::CarState::angularVelocity)
		.def_readonly("localAngularVelocity", &D::CarState::localAngularVelocity)

		.def_readonly("hubMatrix", &D::CarState::hubMatrix)
		.def_readonly("tyreContacts", &D::CarState::tyreContacts)
		.def_readonly("tyreSlip", &D::CarState::tyreSlip)
		.def_readonly("tyreLoad", &D::CarState::tyreLoad)
		.def_readonly("tyreAngularSpeed", &D::CarState::tyreAngularSpeed)

		.def_readonly("probes", &D::CarState::probes)
	;

	m.def("setSeed", &setSeed, "");
	m.def("setLogFile", &setLogFile, "");
	m.def("clearLogFile", &clearLogFile, "");
	m.def("writeLog", &writeLog, "");

	m.def("createSimulator", &createSimulator, "");
	m.def("destroySimulator", &destroySimulator, "");
	m.def("stepSimulator", &stepSimulator, "");

	m.def("loadTrack", &loadTrack, "");
	m.def("unloadTrack", &unloadTrack, "");

	m.def("addCar", &addCar, "");
	m.def("removeCar", &removeCar, "");
	m.def("teleportCarToLocation", &teleportCarToLocation, "");
	m.def("teleportCarToPits", &teleportCarToPits, "");
	m.def("teleportCarToSpline", &teleportCarToSpline, "");
	m.def("teleportCarByMode", &teleportCarByMode, "");
	m.def("setCarAutoTeleport", &setCarAutoTeleport, "");
	m.def("setCarControls", &setCarControls, "");
	m.def("getCarState", &getCarState, "");

	m.def("launchPlaygroundInOwnThread", &launchPlaygroundInOwnThread, "");
	m.def("initPlayground", &initPlayground, "");
	m.def("shutPlayground", &shutPlayground, "");
	m.def("shutAll", &shutAll, "");
	m.def("tickPlayground", &tickPlayground, "");
	m.def("isPlaygroundInitialized", &isPlaygroundInitialized, "");
	m.def("isPlaygroundExited", &isPlaygroundExited, "");
	m.def("moveWindow", &moveWindow, "");
	m.def("resizeWindow", &resizeWindow, "");
	m.def("setRenderHz", &setRenderHz, "");
	m.def("setActiveSimulator", &setActiveSimulator, "");
	m.def("setActiveCar", &setActiveCar, "");
	m.def("getActiveSimulator", &getActiveSimulator, "");
	m.def("getActiveCar", &getActiveCar, "");
}
