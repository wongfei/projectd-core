#include "Car/ScoringSystem.h"
#include "Car/Car.h"
#include "Car/Tyre.h"
#include "Car/Drivetrain.h"
#include "Car/Engine.h"
#include "Sim/Track.h"

namespace D {

enum class RewardWeight
{
	Drift = 0,
	Gas,
	Rpm,
	Speed,
	EngineStall,
	GearGrind,
	DriveDirection,
	ProbeDistance,
	Collision,
	OutOfTrack,
	NUM_WEIGHTS // LAST
};

static const char* _weightNames[] = 
{
	"Drift",
	"Gas",
	"Rpm",
	"Speed",
	"EngineStall",
	"GearGrind",
	"DriveDirection",
	"ProbeDistance",
	"Collision",
	"OutOfTrack",
};

ScoringSystem::ScoringSystem() {}
ScoringSystem::~ScoringSystem() {}

void ScoringSystem::init(struct Car* _car)
{
	car = _car;

	rewardWeights.resize((size_t)RewardWeight::NUM_WEIGHTS, 1.0f);
}

void ScoringSystem::step(float dt)
{
	computeDriftScore(dt);
	computeAgentReward(dt);
}

void ScoringSystem::computeAgentReward(float dt)
{
	float reward = 0.0f;

	reward += getWeight((int)RewardWeight::Drift) * instantDriftDelta;

	reward += getWeight((int)RewardWeight::Gas) * linscalef(car->controls.gas, 0.0f, 1.0f, 0.0f, 0.25f);

	reward += getWeight((int)RewardWeight::Rpm) * linscalef(car->getEngineRpm(), 0.0f, (float)car->drivetrain->engineModel->getLimiterRPM(), 0.0f, 0.25f);

	reward += getWeight((int)RewardWeight::Speed) * linscalef(car->speed.kmh(), 0.0f, 100.0f, 0.0f, 0.25f);

	#if 1
	if (car->getEngineRpm() < 500)
	{
		reward += getWeight((int)RewardWeight::EngineStall) * -0.5f;
	}
	#endif

	#if 1
	if (car->drivetrain->isGearGrinding)
	{
		reward += getWeight((int)RewardWeight::GearGrind) * -0.5f;
	}
	#endif

	#if 1
	if (car->probeHits.size())
	{
		float closestProbe = FLT_MAX;
		for (size_t i = 0; i < car->probeHits.size(); ++i)
		{
			float dist = car->probeHits[i];
			if (closestProbe > dist && dist > 0.0f)
				closestProbe = dist;
		}

		const float nearDistance = 1.8f;
		const float farDistance = 6.0f;

		if (closestProbe < nearDistance)
		{
			reward += getWeight((int)RewardWeight::ProbeDistance) * -2.0f;
		}
		else if (closestProbe < farDistance)
		{
			reward += getWeight((int)RewardWeight::ProbeDistance) * linscalef(closestProbe, nearDistance, farDistance, -0.25f, 0.0f);
		}
	}
	#endif

	#if 1
	if (car->collisionFlag)
	{
		//log_printf(L"collision");

		//reward += getWeight((int)RewardWeight::Collision) * -100.0f;

		if (car->teleportOnCollision)
		{
			car->teleportByMode((TeleportMode)car->teleportMode);
		}
	}
	#endif

	const auto trackPointId = car->nearestTrackPointId;
	if (trackPointId >= 0 && trackPointId < (int)car->track->fatPoints.size())
	{
		const auto& pt = car->track->fatPoints[trackPointId];

		#if 1
		if ((car->body->getPosition(0) - pt.center).len() > car->track->computedTrackWidth * 0.55f) // EPIC FAIL!!!
		{
			car->outOfTrackFlag = true;

			//log_printf(L"out of track");

			//reward += getWeight((int)RewardWeight::OutOfTrack) * -100.0f;

			if (car->teleportOnBadLocation)
			{
				car->teleportByMode((TeleportMode)car->teleportMode);
			}
		}
		#endif

		#if 1
		if (car->speed.kmh() > 3.0f)
		{
			if (car->velocityVsTrack < 0.85f)
			{
				//reward += scoreWrongDirW * linscalef(dot, -1.0f, cutoff, -1.0f, 0.0f);
			}

			reward += getWeight((int)RewardWeight::DriveDirection) * linscalef(car->velocityVsTrack, -1.0f, 1.0f, -0.5f, 0.5f);
		}
		#endif
	}

	agentDriftReward = reward;
}

void ScoringSystem::computeDriftScore(float dt)
{
	validateDrift();
	float fBeta = fabsf(car->getBetaRad());

	const float fSpeedKmh = car->speed.kmh();

	if (fSpeedKmh > 20.0f && fBeta > 0.13089749f)
	{
		auto v = car->body->getLocalVelocity();

		if (!drifting)
		{
			lastDriftDirection = signf(v.x);
			driftComboCounter = 1;
			driftInvalid = false;
			instantDrift = 0.0f;
		}

		currentDriftAngle = fBeta - 0.13089749f;

		float fSpeedMult = (fSpeedKmh - 20.0f) * 0.015384615f;
		fSpeedMult = tclamp(fSpeedMult, 0.0f, 2.0f);

		currentSpeedMultiplier = fSpeedMult;

		driftExtreme = checkExtremeDrift();

		float fDelta = fSpeedMult * currentDriftAngle;
		if (driftExtreme)
			fDelta *= 2.0f;

		instantDriftDelta = fDelta;
		instantDrift += fDelta;

		if (fabsf(v.x) > 4.0f)
		{
			float fDir = signf(v.x);
			if (lastDriftDirection != fDir && fBeta > 0.26179498f)
			{
				instantDrift += 50.0f;
				driftComboCounter++;
				lastDriftDirection = fDir;
			}
		}

		drifting = true;
		driftStraightTimer = 0.0f;
	}

	if (drifting)
	{
		if (fSpeedKmh > 20.0f && fBeta < 0.065448746f)
		{
			driftStraightTimer += dt;
		}
		else
		{
			driftStraightTimer = 0.0f;
		}

		if (driftInvalid)
		{
			resetDrift();
			return;
		}

		if (driftStraightTimer > 1.0f)
		{
			driftComboCounter = 0;
			driftPoints += instantDrift;
			drifting = false;
			instantDrift = 0.0f;
		}
	}

	if (driftInvalid)
	{
		resetDrift();
	}
}

void ScoringSystem::resetDrift()
{
	currentDriftAngle = 0.0;
	currentSpeedMultiplier = 0.0;
	driftExtreme = false;
	drifting = false;
	instantDrift = 0.0;
	driftComboCounter = 0;
}

inline bool isDirty(Tyre* tyre)
{
	auto* surf = tyre->surfaceDef;
	return (surf && surf->dirtAdditiveK > 0.001f);
}

void ScoringSystem::validateDrift()
{
	bool bInvalid = true;

	int nDirtyTyres = 0;
	for (int i = 0; i < 4; ++i)
		nDirtyTyres += (int)isDirty(car->tyres[i].get());

	if (nDirtyTyres <= 2)
	{
		if (car->speed.kmh() >= 20.0f)
		{
			bool bDamage = false;
			for (int i = 0; i < 5; ++i)
			{
				if (fabsf(car->damageZoneLevel[i] - car->oldDamageZoneLevel[i]) > 0.001f)
				{
					bDamage = true;
					break;
				}
			}

			if (!bDamage && car->drivetrain->currentGear)
			{
				bInvalid = false;
			}
		}
	}

	if (bInvalid)
		driftInvalid = true;
}

inline bool isExtremeDrift(Tyre* tyre, float triggerSlipLevel)
{
	auto* surf = tyre->surfaceDef;
	return (surf 
		&& fabsf(tyre->status.angularVelocity) > 4.0
		&& fabsf(tyre->status.slipRatio) > triggerSlipLevel
		&& tyre->status.load > 10.0
		&& surf->gripMod >= 0.9f);
}

bool ScoringSystem::checkExtremeDrift(float triggerSlipLevel) const
{
	int nDriftyTyres = 0;
	for (int i = 0; i < 4; ++i)
		nDriftyTyres += (int)isExtremeDrift(car->tyres[i].get(), triggerSlipLevel);

	return nDriftyTyres > 1;
}

void ScoringSystem::setWeight(int id, float w)
{
	if (id >= 0 && id < (int)rewardWeights.size())
		rewardWeights[id] = w;
}

float ScoringSystem::getWeight(int id) const
{
	if (id >= 0 && id < (int)rewardWeights.size())
		return rewardWeights[id];
	return 0.0f;
}

const char* ScoringSystem::getWeightName(int id) const
{
	if (id >= 0 && id < (int)RewardWeight::NUM_WEIGHTS)
		return _weightNames[id];
	return "";
}

}
