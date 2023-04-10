#pragma once

#include "Sim/SimulatorCommon.h"

namespace D {

struct Simulator;
struct SlipStream;
struct Track;
struct Surface;

struct Car;
struct CarState;
struct CarControls;
struct ICarControlsProvider;
struct ICarAudioRenderer;

struct CarColliderManager;
struct ThermalObject;
struct BrakeSystem;
struct SteeringSystem;
struct GearChanger;

struct ITorqueGenerator;
struct ICoastGenerator;
struct Drivetrain;
struct Engine;
struct Turbo;

struct Damper;
struct ISuspension;
struct SuspensionBase;
struct SuspensionDW;
struct SuspensionStrut;
struct SuspensionAxle;
struct SuspensionML;
struct HeaveSpring;
struct AntirollBar;

struct Tyre;
struct TyreCompound;
struct ITyreModel;
struct SCTM;
struct TyreThermalModel;
struct BrushTyreModel;
struct BrushSlipProvider;

struct AeroMap;
struct Wing;

struct DynamicController;
struct TurboDynamicController;
struct WingDynamicController;

struct AutoClutch;
struct AutoBlip;
struct AutoShifter;

}
