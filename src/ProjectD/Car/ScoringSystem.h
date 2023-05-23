#pragma once

#include "Core/Core.h"
#include <vector>
#include <unordered_map>

namespace D {

struct ScoringVar
{
	std::string name;
	float value;
};

struct ScoringConfig
{
	ScoringConfig();
	~ScoringConfig();

	static ScoringConfig* get();

	void initDefaults();
	void removeVars();

	void addVar(const std::string& name, float value);
	void setVar(const std::string& name, float value);
	float getVar(const std::string& name) const;

	std::vector<ScoringVar*> vvars;
	std::unordered_map<std::string, ScoringVar*> mvars;
};

struct ScoringSystem
{
	ScoringSystem();
	~ScoringSystem();

	void init(struct Car* car);
	void step(float dt);
	void reset();

	void computeAgentReward(float dt);
	void computeDriftScore(float dt);
	void resetDrift();
	void validateDrift();
	bool checkExtremeDrift(float triggerSlipLevel = 0.8f) const;

	inline float getVar(const std::string& name) const { return config->getVar(name); }

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

	ScoringConfig* config = nullptr;
	float stepReward = 0;
	float totalReward = 0;
	float prevEpisodeReward = 0;
	int oldPointId = 0;
	int oldSplinePointId = 0;
};

}
