#pragma once

#include "Core/Core.h"
#include <vector>

namespace D {

struct ScoringSystem
{
	ScoringSystem();
	~ScoringSystem();

	void init(struct Car* car);
	void step(float dt);

	void computeAgentReward(float dt);
	void computeDriftScore(float dt);
	void resetDrift();
	void validateDrift();
	bool checkExtremeDrift(float triggerSlipLevel = 0.8f) const;
	void setWeight(int id, float w);
	float getWeight(int id) const;
	const char* getWeightName(int id) const;

	struct Car* car = nullptr;

	bool drifting = false;
	bool driftExtreme = false;
	bool driftInvalid = false;
	float currentDriftAngle = 0;
	float currentSpeedMultiplier = 0;
	float lastDriftDirection = 0;
	float driftStraightTimer = 0;
	float instantDriftDelta = 0;
	float instantDrift = 0;
	float driftPoints = 0;
	int driftComboCounter = 0;

	float agentDriftReward = 0;

	std::vector<float> rewardWeights;
};

}
