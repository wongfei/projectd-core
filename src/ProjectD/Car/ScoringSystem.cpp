#include "Car/ScoringSystem.h"
#include "Car/Car.h"
#include "Car/Tyre.h"
#include "Car/Drivetrain.h"
#include "Car/Engine.h"
#include "Sim/Track.h"

#define DECL_VAR(name)\
	static const std::string Name_##name (#name)

DECL_VAR(SmoothSteerSpeed);
DECL_VAR(MinBonusSpeed);
DECL_VAR(MaxBonusSpeed);
DECL_VAR(StallRpm);
DECL_VAR(DirectionThreshold);
DECL_VAR(OutOfTrackThreshold);
DECL_VAR(ApproachDistance);
DECL_VAR(CriticalDistance);

DECL_VAR(TravelBonus);
DECL_VAR(TravelSplineBonus);
DECL_VAR(DriftBonus);
DECL_VAR(SpeedBonus);
DECL_VAR(ThrottleBonus);
DECL_VAR(EngineRpmBonus);
DECL_VAR(DirectionBonus);

DECL_VAR(DirectionPenalty);
DECL_VAR(ObstApproachPenalty);
DECL_VAR(CollisionPenalty);
DECL_VAR(OffTrackPenalty);
DECL_VAR(GearGrindPenalty);
DECL_VAR(StallPenalty);

namespace D {

ScoringConfig::ScoringConfig() { initDefaults(); }
ScoringConfig::~ScoringConfig() { removeVars(); }

ScoringConfig* ScoringConfig::get()
{
	static ScoringConfig inst;
	return &inst;
}

void ScoringConfig::initDefaults()
{
	addVar(Name_SmoothSteerSpeed, 10.0f);
	addVar(Name_MinBonusSpeed, 5.0f);
	addVar(Name_MaxBonusSpeed, 200.0f);
	addVar(Name_StallRpm, 300.0f);
	addVar(Name_DirectionThreshold, 0.75f);
	addVar(Name_OutOfTrackThreshold, 0.51f);
	addVar(Name_ApproachDistance, 3.0f);
	addVar(Name_CriticalDistance, 2.0f);

	addVar(Name_TravelBonus, 0.0f); // once per track point
	addVar(Name_TravelSplineBonus, 0.0f); // once per spline point
	addVar(Name_DriftBonus, 0.0f);
	addVar(Name_SpeedBonus, 0.0f);
	addVar(Name_ThrottleBonus, 0.0f);
	addVar(Name_EngineRpmBonus, 0.0f);
	addVar(Name_DirectionBonus, 0.0f);

	addVar(Name_DirectionPenalty, 0.0f);
	addVar(Name_ObstApproachPenalty, 0.0f);
	addVar(Name_CollisionPenalty, 0.0f);
	addVar(Name_OffTrackPenalty, 0.0f);
	addVar(Name_GearGrindPenalty, 0.0f);
	addVar(Name_StallPenalty, 0.0f);
}

void ScoringConfig::removeVars()
{
	std::vector<ScoringVar*> tmp;
	std::swap(tmp, vvars);
	mvars.clear();

	for (auto* var : tmp)
		delete var;
}

void ScoringConfig::addVar(const std::string& name, float value)
{
	auto* var = new ScoringVar{name, value};
	vvars.push_back(var);
	mvars[name] = var;
}

void ScoringConfig::setVar(const std::string& name, float value)
{
	mvars[name]->value = value;
}

float ScoringConfig::getVar(const std::string& name) const
{
	auto iter = mvars.find(name);
	if (iter != mvars.end())
		return iter->second->value;
	return 0;
}

//=================================================================================================

ScoringSystem::ScoringSystem() {}
ScoringSystem::~ScoringSystem() {}

void ScoringSystem::init(struct Car* _car)
{
	car = _car;
	config = ScoringConfig::get();
}

void ScoringSystem::step(float dt)
{
	computeDriftScore(dt);
	computeAgentReward(dt);
}

void ScoringSystem::reset()
{
	prevEpisodeReward = totalReward;
	totalReward = 0;
	stepReward = 0;
	oldPointId = 0;
	oldSplinePointId = 0;
}

void ScoringSystem::computeAgentReward(float dt)
{
	float reward = 0.0f;

	car->smoothSteerSpeed = getVar(Name_SmoothSteerSpeed);

	const float curRpm = car->getEngineRpm();
	const float maxRpm = (float)car->drivetrain->engineModel->getLimiterRPM();

	if (oldPointId < car->nearestTrackPointId || (car->nearestTrackPointId == 0 && oldPointId != car->nearestTrackPointId))
	{
		oldPointId = car->nearestTrackPointId;
		reward += getVar(Name_TravelBonus);
	}

	if (oldSplinePointId < car->splinePointId || (car->splinePointId == 0 && oldSplinePointId != car->splinePointId))
	{
		oldSplinePointId = car->splinePointId;
		reward += getVar(Name_TravelSplineBonus);
	}

	reward += getVar(Name_DriftBonus) * instantDriftDelta;

	reward += getVar(Name_SpeedBonus) * linscalef(car->speed.kmh(), getVar(Name_MinBonusSpeed), getVar(Name_MaxBonusSpeed), 0.0f, 1.0f);

	reward += getVar(Name_ThrottleBonus) * linscalef(car->controls.gas, 0.0f, 1.0f, 0.0f, 1.0f);

	reward += getVar(Name_EngineRpmBonus) * linscalef(curRpm, 0.0f, maxRpm, 0.0f, 1.0f);

	#if 1
	if (car->getEngineRpm() < getVar(Name_StallRpm))
	{
		reward -= getVar(Name_StallPenalty);
	}
	#endif

	#if 1
	if (car->drivetrain->isGearGrinding)
	{
		reward -= getVar(Name_GearGrindPenalty);
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

		const float criticalDistance = getVar(Name_CriticalDistance);
		const float approachDistance = getVar(Name_ApproachDistance);

		if (closestProbe < approachDistance)
		{
			reward -= getVar(Name_ObstApproachPenalty) * (1.0f - linscalef(closestProbe, criticalDistance, approachDistance, 0.0f, 1.0f));
		}
	}
	#endif

	#if 1
	if (car->collisionFlag)
	{
		//log_printf(L"collision");

		reward -= getVar(Name_CollisionPenalty);

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
		if ((car->body->getPosition(0) - pt.center).len() > car->track->computedTrackWidth * getVar(Name_OutOfTrackThreshold)) // EPIC FAIL!!!
		{
			car->outOfTrackFlag = true;

			//log_printf(L"out of track");

			reward -= getVar(Name_OffTrackPenalty);

			if (car->teleportOnBadLocation)
			{
				car->teleportByMode((TeleportMode)car->teleportMode);
			}
		}
		#endif

		#if 1
		//if (car->speed.kmh() > 3.0f)
		{
			const float x = car->bodyVsTrack;
			const float thresh = tclamp(getVar(Name_DirectionThreshold), 0.1f, 1.0f);

			if (x > thresh) // good
			{
				reward += getVar(Name_DirectionBonus) * linscalef(x, thresh, 1.0f, 0.0f, 1.0f);
			}
			else
			{
				reward -= getVar(Name_DirectionPenalty) * (1.0f - linscalef(x, -1.0f, thresh, 0.0f, 1.0f));
			}
		}
		#endif
	}

	stepReward = reward;
	totalReward += reward;
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

}
