#include "Car/SuspensionML.h"

namespace D {

SuspensionML::SuspensionML()
{
	baseCFM = 0.0000001f;
	damageData.minVelocity = 15.0f;
	damageData.damageDirection = randDamageDirection();
}

SuspensionML::~SuspensionML()
{
}

void SuspensionML::init(IPhysicsEnginePtr _core, IRigidBodyPtr _carBody, int _index, const std::wstring& dataPath)
{
	type = SuspensionType::Multilink;
	index = _index;

	core = _core;
	carBody = _carBody;

	auto ini(std::make_shared<INIReader>(dataPath + L"suspensions.ini"));
	GUARD_FATAL(ini->ready);

	std::wstring arrSelector[4] = {L"FRONT", L"FRONT", L"REAR", L"REAR"};
	auto strId = arrSelector[index];

	float fWheelBase = ini->getFloat(L"BASIC", L"WHEELBASE");
	float fCgLocation = ini->getFloat(L"BASIC", L"CG_LOCATION");
	float fFrontBaseY = ini->getFloat(L"FRONT", L"BASEY");
	float fFrontTrack = ini->getFloat(L"FRONT", L"TRACK") * 0.5f;
	float fRearBaseY = ini->getFloat(L"REAR", L"BASEY");
	float fRearTrack = ini->getFloat(L"REAR", L"TRACK") * 0.5f;

	vec3f vRefPoints[4];
	vRefPoints[0] = vec3f(fFrontTrack, fFrontBaseY, (1.0f - fCgLocation) * fWheelBase);
	vRefPoints[1] = vec3f(-fFrontTrack, fFrontBaseY, (1.0f - fCgLocation) * fWheelBase);
	vRefPoints[2] = vec3f(fRearTrack, fRearBaseY, -(fCgLocation * fWheelBase));
	vRefPoints[3] = vec3f(-fRearTrack, fRearBaseY, -(fCgLocation * fWheelBase));

	basePosition = vRefPoints[index];

	hubMass = ini->getFloat(strId, L"HUB_MASS");
	hub = core->createRigidBody();
	hub->setMassBox(hubMass, 0.2f, 0.6f, 0.6f); // TODO: check

	attach();

	for (int i = 0; i < 5; ++i)
	{
		auto strJoint(strwf(L"JOINT%d", i));

		MLJoint j;
		j.ballCar.relToTyre = ini->getFloat3(strId, strJoint + L"_CAR");
		j.ballTyre.relToTyre = ini->getFloat3(strId, strJoint + L"_TYRE");

		if (basePosition.x > 0.0f)
		{
			j.ballCar.relToTyre.x *= -1.0f;
			j.ballTyre.relToTyre.x *= -1.0f;
		}

		j.ballCar.relToCar = carBody->worldToLocal(hub->localToWorld(j.ballCar.relToTyre));
		j.ballTyre.relToCar = carBody->worldToLocal(hub->localToWorld(j.ballTyre.relToTyre));

		auto vBallCar = carBody->localToWorld(j.ballCar.relToCar);
		auto vBallTyre = carBody->localToWorld(j.ballTyre.relToCar);
		j.joint = core->createDistanceJoint(carBody, hub, vBallCar, vBallTyre);

		joints.emplace_back(std::move(j));
	}

	rodLength = ini->getFloat(strId, L"ROD_LENGTH");
	toeOUT_Linear = ini->getFloat(strId, L"TOE_OUT");
	k = ini->getFloat(strId, L"SPRING_RATE");
	progressiveK = ini->getFloat(strId, L"PROGRESSIVE_SPRING_RATE");

	damper.bumpSlow = ini->getFloat(strId, L"DAMP_BUMP");
	damper.reboundSlow = ini->getFloat(strId, L"DAMP_REBOUND");
	damper.bumpFast = ini->getFloat(strId, L"DAMP_FAST_BUMP");
	damper.reboundFast = ini->getFloat(strId, L"DAMP_FAST_REBOUND");
	damper.fastThresholdBump = ini->getFloat(strId, L"DAMP_FAST_BUMPTHRESHOLD");
	damper.fastThresholdRebound = ini->getFloat(strId, L"DAMP_FAST_REBOUNDTHRESHOLD");

	staticCamber = -ini->getFloat(strId, L"STATIC_CAMBER") * 0.017453f;
	if (index % 2)
		staticCamber *= -1.0f;

	baseCarSteerPosition = joints[4].ballCar.relToCar; // TODO: check
}

void SuspensionML::attach()
{
	hub->setRotation(carBody->getWorldMatrix(0));
	hub->setPosition(carBody->localToWorld(basePosition));
}

void SuspensionML::stop()
{
	hub->stop();
}

void SuspensionML::step(float dt)
{
	steerTorque = 0;

	mat44f mxBodyWorld = carBody->getWorldMatrix(0.0f);
	vec3f vM2(&mxBodyWorld.M21);

	vec3f vHubWorld = hub->getPosition(0.0f);
	vec3f vHubLocal = carBody->worldToLocal(vHubWorld);

	float fTravel = (vHubLocal.y - basePosition.y) + rodLength;
	status.travel = fTravel;

	float fForce = (fTravel * progressiveK + k) * fTravel;
	if (packerRange != 0.0f && fTravel > packerRange && k != 0.0f)
	{
		fForce += ((fTravel - packerRange) * bumpStopRate);
	}

	addForceAtPos(vM2 * -fForce, vHubWorld, false, false);
	carBody->addLocalForceAtLocalPos(vec3f(0.0f, fForce, 0.0f), basePosition);

	vec3f vPointVel = carBody->getLocalPointVelocity(basePosition);
	vec3f vDeltaVel = hub->getVelocity() - vPointVel;

	float fDamperSpeed = vDeltaVel * vM2;
	status.damperSpeedMS = fDamperSpeed;

	float fDamperForce = damper.getForce(fDamperSpeed);
	vec3f vForce = vM2 * fDamperForce;
	addForceAtPos(vForce, vHubWorld, false, false);
	carBody->addForceAtLocalPos(vForce * -1.0f, basePosition);
}

void SuspensionML::addForceAtPos(const vec3f& force, const vec3f& pos, bool driven, bool addToSteerTorque)
{
	hub->addForceAtPos(force, pos);

	if (addToSteerTorque)
	{
		vec3f vCenter, vAxis;
		getSteerBasis(vCenter, vAxis);

		steerTorque += (pos - vCenter).cross(force) * vAxis;
	}
}

void SuspensionML::addLocalForceAndTorque(const vec3f& force, const vec3f& torque, const vec3f& driveTorque)
{
	hub->addForceAtLocalPos(force, vec3f(0, 0, 0));
	hub->addTorque(torque);

	if (driveTorque.x != 0.0f || driveTorque.y != 0.0f || driveTorque.z != 0.0f)
		carBody->addTorque(driveTorque);
}

void SuspensionML::addTorque(const vec3f& torque)
{
	hub->addTorque(torque);

	vec3f vCenter, vAxis;
	getSteerBasis(vCenter, vAxis);

	steerTorque += torque * vAxis;
}

void SuspensionML::setERPCFM(float erp, float cfm)
{
	// empty
}

void SuspensionML::setSteerLengthOffset(float o)
{
	float sx = signf(basePosition.x);
	float d = (damageData.damageDirection * damageData.damageAmount);
	float offx = d + o + (sx * toeOUT_Linear);

	auto& mj = joints[4];
	const auto& base = baseCarSteerPosition;
	mj.ballCar.relToCar = vec3f(base.x + offx, base.y, base.z);

	mj.joint->reseatDistanceJointLocal(mj.ballCar.relToCar, mj.ballTyre.relToTyre);
}

void SuspensionML::getSteerBasis(vec3f& centre, vec3f& axis)
{
	auto vTop = hub->localToWorld(joints[0].ballTyre.relToTyre);
	auto vBottom = hub->localToWorld(joints[2].ballTyre.relToTyre);
	centre = (vTop + vBottom) * 0.5f;
	axis = (vTop - vBottom).get_norm();
}

mat44f SuspensionML::getHubWorldMatrix()
{
	return mat44f::rotate(hub->getWorldMatrix(0), vec3f(0, 0, 1), staticCamber);
}

vec3f SuspensionML::getBasePosition()
{
	return basePosition;
}

vec3f SuspensionML::getVelocity()
{
	return hub->getVelocity();
}

vec3f SuspensionML::getPointVelocity(const vec3f& p)
{
	return hub->getPointVelocity(p);
}

vec3f SuspensionML::getHubAngularVelocity()
{
	return hub->getAngularVelocity();
}

float SuspensionML::getMass()
{
	return hub->getMass();
}

float SuspensionML::getSteerTorque()
{
	return steerTorque;
}

void SuspensionML::getDebugState(CarDebug* state)
{
}

}
