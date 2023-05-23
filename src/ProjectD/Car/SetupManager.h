#pragma once

#include "Core/Core.h"
#include <vector>
#include <unordered_map>

namespace D {

enum class SpinnerType
{
	Default = 0,
	Clicks = 1,
	ClicksAbs = 2,
	RawFloat = 3,
	RawPredefined = 4,
};

struct SetupSpinner
{
	float value = 0;
	float minV = 0;
	float maxV = 0;
	float step = 0;
	SpinnerType type = SpinnerType::RawFloat;
};

struct SetupVar
{
	std::string name;
	std::string units;
	std::string tab;
	float* fvalue = nullptr;
	double* dvalue = nullptr;
	float mult = 1;
	float dispMult = 1;
	float defV = 0;
	float minV = -FLT_MAX;
	float maxV = FLT_MAX;
	float step = 0.01f;
	int order = 0;
	bool tunable = false;

	SpinnerType spinnerType = SpinnerType::RawFloat;
	std::vector<float> spinnerValues;

	inline SetupVar(const std::string& _name, const std::string& _units, float* _value, float _mult, float _dispMult) : 
		name(_name), units(_units), fvalue(_value), mult(_mult), dispMult(_dispMult) {}

	inline SetupVar(const std::string& _name, const std::string& _units, double* _value, float _mult, float _dispMult) : 
		name(_name), units(_units), dvalue(_value), mult(_mult), dispMult(_dispMult) {}

	// RAW (SIM) VALUE
	inline void setRaw(float v) { if (fvalue) { *fvalue = v; } else if (dvalue) { *dvalue = v; } }
	inline float getRaw() const { return fvalue ? *fvalue : (dvalue ? (float)*dvalue : 0.0f); }

	SetupSpinner getSpinner() const;
	SetupSpinner getSpinner(SpinnerType type) const;
	void setValue(const SetupSpinner& spinner);
	int getSpinnerValueIdx(float value) const;

	// TUNE VALUE (depends on spinnerType)
	void setTune(float v);
	float getTune() const;
};

struct SetupGearRatio
{
	std::string name;
	float value = 0;

	static std::vector<SetupGearRatio> load(const std::wstring& filename);
};

struct SetupManager
{
	SetupManager();
	~SetupManager();

	void init(struct Car* car);
	void removeVars();
	SetupVar* addVar(const std::string& name, const std::string& units, double mult, double dispMult, float* fvalue);
	SetupVar* addVar(const std::string& name, const std::string& units, double mult, double dispMult, double* dvalue);
	void addVar(SetupVar* var);
	SetupVar* getVar(const std::string& name);
	void setRawTune(const std::string& name, float value); // UNSAFE
	void setTune(const std::string& name, float value);

	std::vector<SetupVar*> vvars;
	std::unordered_map<std::string, SetupVar*> mvars;
};

}
