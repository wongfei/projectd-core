// https://pybind11.readthedocs.io/en/latest/basics.html
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include "PlaygrounD.h"
#include <unordered_map>

static int g_uniqSimId = 0;
static std::unordered_map<int, D::SimulatorPtr> g_simMap;

//
// CORE
//

void setSeed(unsigned int seed)
{
	srand(seed);
}

void initLogFile(const std::string& path)
{
	D::log_init(D::strw(path).c_str());
}

void closeLogFile()
{
	D::log_close();
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
	return nullptr;
}

inline D::SimulatorPtr getSimulatorShared(int simId)
{
	auto iter = g_simMap.find(simId);
	if (iter != g_simMap.end())
	{
		return iter->second;
	}
	return nullptr;
}

inline D::Car* getCar(int simId, int carId)
{
	auto* sim = getSimulator(simId);
	return sim ? sim->getCar(carId) : nullptr;
}

int createSimulator(const std::string& basePath)
{
	try
	{
		//D::INIReader::_debug = true;

		auto sim = std::make_shared<D::Simulator>();
		sim->init(D::strw(basePath));

		int id = g_uniqSimId++;
		g_simMap.insert({id, sim});

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
	auto iter = g_simMap.find(simId);
	if (iter != g_simMap.end())
	{
		g_simMap.erase(iter);
	}
}

void stepSimulator(int simId)
{
	auto* sim = getSimulator(simId);
	if (sim && sim->physics)
	{
		const double dt = 1.0 / 333.0;

		sim->step((float)dt, sim->physicsTime, sim->gameTime);

		sim->physicsTime += dt;
		sim->gameTime += dt;
	}
}

void stepSimulatorEx(int simId, double dt, double physicsTime, double gameTime)
{
	auto* sim = getSimulator(simId);
	if (sim && sim->physics)
	{
		sim->step((float)dt, physicsTime, gameTime);
	}
}

//
// TRACK
//

void loadTrack(int simId, const std::string &trackName)
{
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
		const auto& pits = car->sim->track->pits;
		if (pitId >= 0 && pitId < (int)pits.size())
		{
			car->teleportToPits(pits[pitId]);
		}
	}
}

void teleportCarToTrackLocation(int simId, int carId, float distanceNorm, float offsetY)
{
	auto* car = getCar(simId, carId);
	if (car)
	{
		car->teleportToTrackLocation(distanceNorm, offsetY);

		/*const auto& points = car->sim->track->fatPoints;
		if (pointId >= 0 && pointId < (int)points.size())
		{
			auto& pt = points[pointId];

			car->forceRotation(pt.forwardDir);
			car->forcePosition(D::vec3f(&pt.center.x), offsetY);
		}*/
	}
}

void setCarControls(int simId, int carId, const D::CarControls& controls)
{
	auto* car = getCar(simId, carId);
	if (car)
	{
		car->controls = controls;
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

static std::unique_ptr<D::PlaygrounD> g_playground;

void initPlayground(const std::string& basePath)
{
	if (!g_playground)
	{
		g_playground.reset(new D::PlaygrounD());
		g_playground->init(basePath, true);
	}
}

void shutPlayground()
{
	g_playground.reset();
}

void tickPlayground()
{
	if (g_playground)
	{
		g_playground->tick();
	}
}

bool isPlaygroundExited()
{
	return !g_playground || (g_playground && g_playground->exitFlag_);
}

void setActiveSimulator(int simId, bool simEnabled)
{
	if (g_playground)
	{
		auto sim = getSimulatorShared(simId);
		if (sim)
		{
			g_playground->setSimulator(sim);
			g_playground->simEnabled_ = simEnabled;
		}
	}
}

void setActiveCar(int carId, bool takeControls, bool enableSound)
{
	if (g_playground)
	{
		g_playground->setActiveCar(carId, takeControls, enableSound);
	}
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
		.def_readonly("controls", &D::CarState::controls)

		.def_readonly("engineRPM", &D::CarState::engineRPM)
		.def_readonly("speedMS", &D::CarState::speedMS)
		.def_readonly("gear", &D::CarState::gear)
		.def_readonly("gearGrinding", &D::CarState::gearGrinding)

		.def_readonly("bodyMatrix", &D::CarState::bodyMatrix)
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
		.def_readonly("trackLocation", &D::CarState::trackLocation)
		.def_readonly("agentScore", &D::CarState::agentScore)
		;

	m.def("setSeed", &setSeed, "");
	m.def("initLogFile", &initLogFile, "");
	m.def("closeLogFile", &closeLogFile, "");
	m.def("writeLog", &writeLog, "");

	m.def("createSimulator", &createSimulator, "");
	m.def("destroySimulator", &destroySimulator, "");
	m.def("stepSimulator", &stepSimulator, "");
	m.def("stepSimulatorEx", &stepSimulatorEx, "");

	m.def("loadTrack", &loadTrack, "");
	m.def("unloadTrack", &unloadTrack, "");

	m.def("addCar", &addCar, "");
	m.def("removeCar", &removeCar, "");
	m.def("teleportCarToLocation", &teleportCarToLocation, "");
	m.def("teleportCarToPits", &teleportCarToPits, "");
	m.def("teleportCarToTrackLocation", &teleportCarToTrackLocation, "");
	m.def("setCarControls", &setCarControls, "");
	m.def("getCarState", &getCarState, "");

	m.def("initPlayground", &initPlayground, "");
	m.def("shutPlayground", &shutPlayground, "");
	m.def("tickPlayground", &tickPlayground, "");
	m.def("isPlaygroundExited", &isPlaygroundExited, "");
	m.def("setActiveSimulator", &setActiveSimulator, "");
	m.def("setActiveCar", &setActiveCar, "");
}
