#pragma once

#include "Car/CarCommon.h"
#include "Car/CarControls.h"
#include "Car/ISuspension.h"
#include "Core/Event.h"

namespace D {

enum class TorqueModeEX
{
	original = 0x0,
	reactionTorques = 0x1,
	driveTorques = 0x2,
};

struct CarCollisionBounds
{
	vec3f min;
	vec3f max;
	float length = 0;
	float width = 0;
	float lengthFront = 0;
	float lengthRear = 0;
};

struct OnStepCompleteEvent
{
	Car* car = nullptr;
	double physicsTime = 0;
};

struct OnCollisionEvent
{
	IRigidBody* body = nullptr;
	vec3f worldPos;
	vec3f relPos;
	float relativeSpeed = 0;
	unsigned long colliderGroup = 0;
};

struct Car : public virtual IObject
{
	Car(TrackPtr track);
	~Car();

	// init
	bool init(const std::wstring& modelName);
	void initCarData();
	void loadColliderBlob();
	void initColliderMesh(ITriMeshPtr mesh, const mat44f& bodyMatrix);

	// step
	void stepPreCacheValues(float dt);
	void step(float dt);
	void updateAirPressure();
	void updateBodyMass();
	float calcBodyMass();
	void stepThermalObjects(float dt);
	void stepComponents(float dt);
	void postStep(float dt);
	void updateCarState();

	// collision
	void onCollisionCallback(void* userData0, void* shape0, void* userData1, void* shape1, const vec3f& normal, const vec3f& pos, float depth);

	// controls
	void pollControls(float dt);
	void sendFF(float dt);
	float getSteerFF(float dt);

	// utils
	void teleport(const mat44f& m);
	vec3f getGroundWindVector() const;
	float getPointGroundHeight(const vec3f& pt) const;
	mat44f getGraphicsOffsetMatrix() const;
	bool isSleeping() const;
	float getEngineRpm() const;
	float getOptimalBrake();

	Event<OnStepCompleteEvent> evOnStepComplete;
	Event<OnCollisionEvent> evOnCollisionEvent;
	
	// CONFIG

	unsigned int physicsGUID = 0;

	std::wstring unixName;
	std::wstring configName;
	std::wstring carDataPath;
	std::wstring screenName;

	SuspensionType suspensionTypeF = SuspensionType(0);
	SuspensionType suspensionTypeR = SuspensionType(0);
	TorqueModeEX torqueModeEx = TorqueModeEX(0);
	float axleTorqueReaction = 1.0f;

	CarCollisionBounds bounds;
	vec3f bodyInertia;
	vec3f explicitInertia;
	vec3f fuelTankPos;
	vec3f ridePickupPoint[2];

	float mass = 0;
	float ballastKG = 0;
	float fuelKG = 0.74f;
	float requestedFuel = 30.0f;
	double maxFuel = 30.0f;
	double fuelConsumptionK = 0;

	// aero
	float slipStreamEffectGain = 1.0f;

	// force feedback
	float ffMult = 0.003f;
	float steerLock = 200.0f;
	float steerRatio = 12.0f;
	float steerLinearRatio = 0.003f;
	float steerAssist = 1.0f;
	float userFFGain = 1.0f;

	vec3f graphicsOffset;
	float graphicsPitchRotation = 0;

	// RUNTIME

	void* tag = nullptr; // weak
	Simulator* sim = nullptr; // weak
	ICarAudioRenderer* audioRenderer = nullptr; // weak
	ICarControlsProvider* controlsProvider = nullptr; // weak

	TrackPtr track;
	IRigidBodyPtr body;
	IRigidBodyPtr fuelTankBody;
	IRigidBodyPtr rigidAxle;
	IJointPtr fuelTankJoint;
	ITriMeshPtr collider;

	std::vector<std::unique_ptr<SuspensionBase>> suspensionsImpl;
	std::vector<ISuspension*> suspensions;
	std::vector<std::unique_ptr<HeaveSpring>> heaveSprings;
	std::vector<std::unique_ptr<AntirollBar>> antirollBars;
	std::vector<std::unique_ptr<Tyre>> tyres;
	std::vector<std::wstring> tyreCompounds;

	std::unique_ptr<CarColliderManager> colliderManager;
	std::unique_ptr<SlipStream> slipStream;
	std::unique_ptr<AeroMap> aeroMap;
	std::unique_ptr<ThermalObject> water;
	std::unique_ptr<BrakeSystem> brakeSystem;
	std::unique_ptr<SteeringSystem> steeringSystem;
	std::unique_ptr<Drivetrain> drivetrain;
	std::unique_ptr<GearChanger> gearChanger;
	std::unique_ptr<CarState> state;

	CarControls controls;
	float finalSteerAngleSignal = 0;

	mat44f pitPosition;
	Speed speed;
	vec3f lastVelocity;
	vec3f accG;
	double fuel = 0;

	double lastBodyMassUpdateTime = 0;
	double lastCollisionTime = 0;
	double lastCollisionWithCarTime = 0;
	float damageZoneLevel[5] = {};

	int framesToSleep = 50;
	int sleepingFrames = 0;

	// force feedback
	float vibrationPhase = 0;
	float slipVibrationPhase = 0;
	float mzCurrent = 0;
	float flatSpotPhase = 0;
	float lastSteerPosition = 0;
	float lastPureMZFF = 0;
	float lastGyroFF = 0;
	float lastFF = 0;
	float lastDamp = 0;
};

}
