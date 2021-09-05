#pragma once

#include "Core/Core.h"

#define OT_VALIDATE_ABI\
	D::ABIValidator::validate({})

namespace D {

struct ABIValidator
{
	// alignment/endian test
	uint8_t u8 = 0xFA;
	uint16_t u16 = 0xFAFB;
	uint32_t u32 = 0xFAFBFCFD;
	uint64_t u64 = 0xFAFBFCFDFAFBFCFD;
	void* ptr = (void*)0x00AABBCCDDEEFF00;

	// sizeof test
	#define SIZE_FIELD(type, name) size_t sz_##name = sizeof(type)
	SIZE_FIELD(ABIValidator, ABIValidator);
	SIZE_FIELD(void*, ptr);
	SIZE_FIELD(size_t, index);
	SIZE_FIELD(int, int);
	SIZE_FIELD(short, short);
	SIZE_FIELD(char, char);
	SIZE_FIELD(bool, bool);
	SIZE_FIELD(float, float);
	SIZE_FIELD(double, double);
	#undef SIZE_FIELD

	static void validate(const ABIValidator& otherAbi);
};

}
