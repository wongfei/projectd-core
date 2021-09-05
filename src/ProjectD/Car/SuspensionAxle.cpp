#include "Car/SuspensionAxle.h"

namespace D {

SuspensionAxle::SuspensionAxle()
{
	baseCFM = 0.0000001f;
	attachRelativePos = 1.0f;
}

SuspensionAxle::~SuspensionAxle()
{
}

void SuspensionAxle::init(IPhysicsEnginePtr _core, IRigidBodyPtr _carBody, IRigidBodyPtr _axle, int _index, RigidAxleSide _side, const std::wstring& dataPath)
{
	type = SuspensionType::Axle;
	index = _index;
	side = _side;

	core = _core;
	carBody = _carBody;
	axle = _axle;

	auto ini(std::make_shared<INIReader>(dataPath + L"suspensions.ini"));
	GUARD_FATAL(ini->ready);

	int iVer = ini->getInt(L"HEADER", L"VERSION");

	float fWheelBase = ini->getFloat(L"BASIC", L"WHEELBASE");
	float fCgLocation = ini->getFloat(L"BASIC", L"CG_LOCATION");
	float fRearBaseY = ini->getFloat(L"REAR", L"BASEY");
	float fRearTrack = ini->getFloat(L"REAR", L"TRACK") * 0.5f;

	track = fRearTrack;
	referenceY = fRearBaseY;
	axleBasePos = vec3f(0.0f, fRearBaseY, -(fCgLocation * fWheelBase));

	if (iVer >= 4)
	{
		attachRelativePos = ini->getFloat(L"AXLE", L"ATTACH_REL_POS");
	}

	if (side == RigidAxleSide::Left)
	{
		float fMass = ini->getFloat(L"REAR", L"HUB_MASS");
		axle->setMassBox(fMass, track * 2.0f, 0.2f, 0.5f); // TODO: check
		attach();

		int iLinkCount = ini->getInt(L"AXLE", L"LINK_COUNT");
		for (int i = 0; i < iLinkCount; ++i)
		{
			auto strJoint(strwf(L"J%d", i));
			auto vBallCar = ini->getFloat3(L"AXLE", strJoint + L"_CAR");
			auto vBallAxle = ini->getFloat3(L"AXLE", strJoint + L"_AXLE");

			AxleJoint j;
			j.ballCar.relToAxle = vBallCar;
			j.ballCar.relToCar = carBody->worldToLocal(axle->localToWorld(vBallCar));
			j.ballAxle.relToAxle = vBallAxle;
			j.ballAxle.relToCar = carBody->worldToLocal(axle->localToWorld(vBallAxle));

			auto vJ0 = carBody->localToWorld(j.ballCar.relToCar);
			auto vJ1 = carBody->localToWorld(j.ballAxle.relToCar);
			j.ballAxle.joint = core->createDistanceJoint(carBody, axle, vJ0, vJ1);

			joints.emplace_back(std::move(j));
		}
	}

	bumpStopUp = ini->getFloat(L"REAR", L"BUMPSTOP_UP");
	bumpStopDn = -ini->getFloat(L"REAR", L"BUMPSTOP_DN");
	rodLength = ini->getFloat(L"REAR", L"ROD_LENGTH");
	toeOUT_Linear = ini->getFloat(L"REAR", L"TOE_OUT");
	k = ini->getFloat(L"REAR", L"SPRING_RATE");
	progressiveK = ini->getFloat(L"REAR", L"PROGRESSIVE_SPRING_RATE");

	damper.bumpSlow = ini->getFloat(L"REAR", L"DAMP_BUMP");
	damper.reboundSlow = ini->getFloat(L"REAR", L"DAMP_REBOUND");
	damper.bumpFast = ini->getFloat(L"REAR", L"DAMP_FAST_BUMP");
	damper.reboundFast = ini->getFloat(L"REAR", L"DAMP_FAST_REBOUND");
	damper.fastThresholdBump = ini->getFloat(L"REAR", L"DAMP_FAST_BUMPTHRESHOLD");
	damper.fastThresholdRebound = ini->getFloat(L"REAR", L"DAMP_FAST_REBOUNDTHRESHOLD");

	if (damper.fastThresholdBump == 0.0f)
		damper.fastThresholdBump = 0.2f;
	if (damper.fastThresholdRebound == 0.0f)
		damper.fastThresholdRebound = 0.2f;
	if (damper.bumpFast == 0.0f)
		damper.bumpFast = damper.bumpSlow;
	if (damper.reboundFast == 0.0f)
		damper.reboundFast = damper.reboundSlow;

	bumpStopRate = ini->getFloat(L"REAR", L"BUMP_STOP_RATE");
	if (bumpStopRate == 0.0f)
		bumpStopRate = 500000.0f;

	if (iVer >= 3)
	{
		leafSpringK.x = ini->getFloat(L"AXLE", L"LEAF_SPRING_LAT_K");
	}

	setERPCFM(0.3f, baseCFM);
}

void SuspensionAxle::attach()
{
	if (side == RigidAxleSide::Left)
	{
		axle->setRotation(carBody->getWorldMatrix(0));
		axle->setPosition(carBody->localToWorld(axleBasePos));
	}
}

void SuspensionAxle::stop()
{
	axle->stop();
}

void SuspensionAxle::step(float dt)
{
	mat44f mxBodyWorld = carBody->getWorldMatrix(0.0f);

	mat44f mxAxle = axle->getWorldMatrix(0.0f);
	vec3f vAxleM1(&mxAxle.M11);
	vec3f vAxleM4(&mxAxle.M41);

	float fSideSign = ((int)side ? -1.0f : 1.0f);
	vec3f vAxleWorld = (vAxleM1 * (fSideSign * track * attachRelativePos)) + vAxleM4;
	vec3f vAxleLocal = carBody->worldToLocal(vAxleWorld);

	vec3f vBase = getBasePosition();
	vBase.x *= attachRelativePos;
	vBase.y += 0.2f;
	vec3f vBaseWorld = carBody->localToWorld(vBase);

	vec3f vDelta = vBaseWorld - vAxleWorld;
	float fDeltaLen = vDelta.len();
	vDelta.norm(fDeltaLen);

	float fTravel = (0.2f - fDeltaLen) + rodLength;
	status.travel = fTravel;

	float fForce = -(((fTravel * progressiveK) + k) * fTravel);
	if (fForce < 0.0f)
	{
		addForceAtPos(vDelta * fForce, vAxleWorld, false, false);
		carBody->addForceAtPos(vDelta * -fForce, vBaseWorld);
	}

	if (leafSpringK.x != 0.0f)
	{
		fForce = (vAxleLocal.x - (attachRelativePos * getBasePosition().x)) * leafSpringK.x;
		addForceAtPos(vec3f(&mxBodyWorld.M11) * -fForce, vAxleWorld, false, false);
		carBody->addLocalForceAtLocalPos(vec3f(fForce, 0.0f, 0.0f), vAxleLocal);
	}

	float fBumpStopUp = bumpStopUp;
	float fRefY = vAxleLocal.y - referenceY;
	if (fBumpStopUp != 0.0f && fRefY > fBumpStopUp && 0.0f != k)
	{
		fForce = (fRefY - fBumpStopUp) * 500000.0f;
		addForceAtPos(vec3f(&mxBodyWorld.M21) * -fForce, vAxleWorld, false, false);
		carBody->addLocalForceAtLocalPos(vec3f(0.0f, fForce, 0.0f), vAxleLocal);
	}

	float fBumpStopDn = bumpStopDn;
	if (fBumpStopDn != 0.0f && fRefY < fBumpStopDn && 0.0f != k)
	{
		fForce = (fRefY - fBumpStopDn) * 500000.0f;
		addForceAtPos(vec3f(&mxBodyWorld.M21) * -fForce, vAxleWorld, false, false);
		carBody->addLocalForceAtLocalPos(vec3f(0.0f, fForce, 0.0f), vAxleLocal);
	}

	vec3f vPointVel = carBody->getPointVelocity(vBaseWorld);
	vec3f vDeltaVel = getVelocity() - vPointVel;

	float fDamperSpeed = vDeltaVel * vDelta;
	status.damperSpeedMS = fDamperSpeed;

	float fDamperForce = damper.getForce(fDamperSpeed);
	vec3f vForce = vDelta * fDamperForce;
	addForceAtPos(vForce, vAxleWorld, false, false);
	carBody->addForceAtPos(vForce * -1.0f, vBaseWorld);
}

void SuspensionAxle::addForceAtPos(const vec3f& force, const vec3f& pos, bool driven, bool addToSteerTorque)
{
	axle->addForceAtPos(force, pos);
}

void SuspensionAxle::addLocalForceAndTorque(const vec3f& force, const vec3f& torque, const vec3f& driveTorque)
{
	axle->addForceAtLocalPos(force, vec3f(0, 0, 0));
	axle->addTorque(torque);

	if (driveTorque.x != 0.0f || driveTorque.y != 0.0f || driveTorque.z != 0.0f)
		carBody->addTorque(driveTorque);
}

void SuspensionAxle::addTorque(const vec3f& torque)
{
	axle->addTorque(torque);
}

void SuspensionAxle::setERPCFM(float erp, float cfm)
{
	for (auto& iter : joints)
	{
		iter.ballAxle.joint->setERPCFM(erp, cfm);
	}
}

void SuspensionAxle::setSteerLengthOffset(float o)
{
	SHOULD_NOT_REACH_FATAL;
}

void SuspensionAxle::getSteerBasis(vec3f& centre, vec3f& axis)
{
	SHOULD_NOT_REACH_FATAL;
}

mat44f SuspensionAxle::getHubWorldMatrix()
{
	auto m = axle->getWorldMatrix(0);
	float t = (int)side ? -track : track;
	m.M41 += m.M11 * t;
	m.M42 += m.M12 * t;
	m.M43 += m.M13 * t;
	return m;
}

vec3f SuspensionAxle::getBasePosition()
{
	float t = (int)side ? -track : track;
	vec3f p(t, axleBasePos.y, axleBasePos.z);
	return p;
}

vec3f SuspensionAxle::getVelocity()
{
	float t = (int)side ? -track : track;
	return axle->getLocalPointVelocity(vec3f(t, 0, 0));
}

vec3f SuspensionAxle::getPointVelocity(const vec3f& p)
{
	return axle->getPointVelocity(p);
}

vec3f SuspensionAxle::getHubAngularVelocity()
{
	return axle->getAngularVelocity();
}

float SuspensionAxle::getMass()
{
	return axle->getMass() * 0.5f; // TODO: why half?
}

float SuspensionAxle::getSteerTorque()
{
	SHOULD_NOT_REACH_FATAL;
}

void SuspensionAxle::getDebugState(CarDebug* state)
{
	if (side == RigidAxleSide::Left)
	{
		for (auto& j : joints)
		{
			auto vBallCar = carBody->localToWorld(j.ballCar.relToCar);
			auto vBallAxle = axle->localToWorld(j.ballAxle.relToAxle);
			state->lines.emplace_back(ColorLine{vBallCar, vBallAxle, vec3f(1.0f, 0.0f, 1.0f)});
		}
	}
}

}
