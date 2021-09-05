#include "Car/HeaveSpring.h"
#include "Car/SuspensionDW.h"

namespace D {

HeaveSpring::HeaveSpring()
{}

HeaveSpring::~HeaveSpring()
{}

void HeaveSpring::init(IRigidBody* _carBody, SuspensionDW* s1, SuspensionDW* s2, bool _isFront, const std::wstring& dataPath)
{
	carBody = _carBody;
	suspensions[0] = s1;
	suspensions[1] = s2;
	isFront = _isFront;

	auto ini(std::make_unique<INIReader>(dataPath + L"suspensions.ini"));
	GUARD_FATAL(ini->ready);

	std::wstring strId = isFront ? L"HEAVE_FRONT" : L"HEAVE_REAR";
	if (ini->hasSection(strId))
	{
		isPresent = true;
		bumpStopUp = ini->getFloat(strId, L"BUMPSTOP_UP");
		bumpStopDn = -ini->getFloat(strId, L"BUMPSTOP_DN");
		rodLength = ini->getFloat(strId, L"ROD_LENGTH");
		k = ini->getFloat(strId, L"SPRING_RATE");
		progressiveK = ini->getFloat(strId, L"PROGRESSIVE_SPRING_RATE");

		damper.bumpSlow = ini->getFloat(strId, L"DAMP_BUMP");
		damper.reboundSlow = ini->getFloat(strId, L"DAMP_REBOUND");
		damper.bumpFast = ini->getFloat(strId, L"DAMP_FAST_BUMP");
		damper.reboundFast = ini->getFloat(strId, L"DAMP_FAST_REBOUND");
		damper.fastThresholdBump = ini->getFloat(strId, L"DAMP_FAST_BUMPTHRESHOLD");
		damper.fastThresholdRebound = ini->getFloat(strId, L"DAMP_FAST_REBOUNDTHRESHOLD");

		if (damper.fastThresholdBump == 0.0f)
			damper.fastThresholdBump = 0.2f;
		if (damper.fastThresholdRebound == 0.0f)
			damper.fastThresholdRebound = 0.2f;
		if (damper.bumpFast == 0.0f)
			damper.bumpFast = damper.bumpSlow;
		if (damper.reboundFast == 0.0f)
			damper.reboundFast = damper.reboundSlow;

		bumpStopRate = ini->getFloat(strId, L"BUMP_STOP_RATE");
		if (bumpStopRate == 0.0f)
			bumpStopRate = 500000.0f;

		packerRange = ini->getFloat(strId, L"PACKER_RANGE");
	}
}

void HeaveSpring::step(float dt)
{
	mat44f mxBodyWorld = carBody->getWorldMatrix(0.0f);
	vec3f vM2 = vec3f(&mxBodyWorld.M21);

	auto* pSusp0 = suspensions[0];
	vec3f vRefPoint0 = pSusp0->dataRelToWheel.refPoint;
	vec3f vHubPos0 = pSusp0->hub->getPosition(0.0f);
	vec3f vHubLoc0 = carBody->worldToLocal(vHubPos0);

	auto* pSusp1 = suspensions[1];
	vec3f vRefPoint1 = pSusp1->dataRelToWheel.refPoint;
	vec3f vHubPos1 = pSusp1->hub->getPosition(0.0f);
	vec3f vHubLoc1 = carBody->worldToLocal(vHubPos1);

	//

	if (pSusp0->k != 0.0f || pSusp1->k != 0.0f)
		rodLength = (pSusp1->rodLength + pSusp0->rodLength) * 0.5f;

	float fAvgY = (vHubLoc0.y + vHubLoc1.y) * 0.5f;
	float fTravel = (fAvgY - pSusp0->dataRelToWheel.refPoint.y) + rodLength;
	status.travel = fTravel;

	//

	float v12 = ((fTravel * progressiveK) + k) * fTravel;

	if (packerRange != 0.0f && fTravel > packerRange)
		v12 += ((fTravel - packerRange) * bumpStopRate);

	vec3f vForce = vM2 * -v12;
	pSusp0->addForceAtPos(vForce, pSusp0->hub->getPosition(0.0f), false, false);
	pSusp1->addForceAtPos(vForce, pSusp1->hub->getPosition(0.0f), false, false);

	carBody->addLocalForceAtLocalPos(vec3f(0, v12, 0), vRefPoint0);
	carBody->addLocalForceAtLocalPos(vec3f(0, v12, 0), vRefPoint1);

	//

	float fBumpStopUp = bumpStopUp;
	float fBumpStopDn = bumpStopDn;
	float fDeltaY0 = fAvgY - pSusp0->dataRelToWheel.refPoint.y;

	if (fBumpStopUp != 0.0f && fDeltaY0 > fBumpStopUp)
	{
		float fForce = (fDeltaY0 - fBumpStopUp) * 500000.0f;

		vForce = vM2 * -fForce;
		pSusp0->addForceAtPos(vForce, pSusp0->hub->getPosition(0.0f), false, false);
		pSusp1->addForceAtPos(vForce, pSusp1->hub->getPosition(0.0f), false, false);

		vForce = vec3f(0, fForce, 0);
		carBody->addLocalForceAtLocalPos(vForce, vRefPoint0);
		carBody->addLocalForceAtLocalPos(vForce, vRefPoint1);
	}

	//

	if (fBumpStopDn != 0.0f && fDeltaY0 < fBumpStopDn)
	{
		float fForce = (fDeltaY0 - fBumpStopDn) * 500000.0f;

		vForce = vM2 * -fForce;
		pSusp0->addForceAtPos(vForce, pSusp0->hub->getPosition(0.0f), false, false);
		pSusp1->addForceAtPos(vForce, pSusp1->hub->getPosition(0.0f), false, false);

		vForce = vec3f(0, fForce, 0);
		carBody->addLocalForceAtLocalPos(vForce, vRefPoint0);
		carBody->addLocalForceAtLocalPos(vForce, vRefPoint1);
	}

	//

	vec3f vHubVel0 = pSusp0->hub->getVelocity();
	vec3f vHubVel1 = pSusp1->hub->getVelocity();
	vec3f vHubVel = (vHubVel0 + vHubVel1) * 0.5f;

	vec3f vLpv0 = carBody->getLocalPointVelocity(vRefPoint0);
	vec3f vLpv1 = carBody->getLocalPointVelocity(vRefPoint1);
	vec3f vLpv = (vLpv0 + vLpv1) * 0.5f;

	vec3f vDeltaVel = vHubVel - vLpv;
	float fDamperSpeed = vDeltaVel * vM2;
	float fDamperForce = damper.getForce(fDamperSpeed);

	vForce = vM2 * fDamperForce;
	pSusp0->addForceAtPos(vForce, pSusp0->hub->getPosition(0.0f), false, false);
	pSusp1->addForceAtPos(vForce, pSusp1->hub->getPosition(0.0f), false, false);

	vForce *= -1.0f;
	carBody->addLocalForceAtLocalPos(vForce, vRefPoint0);
	carBody->addLocalForceAtLocalPos(vForce, vRefPoint1);
}

}
