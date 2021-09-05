#include "ABIValidator.h"
#include "Diag.h"

namespace D {

static const ABIValidator _moduleAbi;

void ABIValidator::validate(const ABIValidator& otherAbi)
{
	#define ABI_TEST(field) (_moduleAbi.field == otherAbi.field)
	GUARD_FATAL
	(
		// alignment/endian test
		ABI_TEST(u8) &&
		ABI_TEST(u16) &&
		ABI_TEST(u32) &&
		ABI_TEST(u64) &&
		ABI_TEST(ptr) &&

		// sizeof test
		ABI_TEST(sz_ABIValidator) &&
		ABI_TEST(sz_ptr) &&
		ABI_TEST(sz_index) &&
		ABI_TEST(sz_int) &&
		ABI_TEST(sz_short) &&
		ABI_TEST(sz_char) &&
		ABI_TEST(sz_bool) && 
		ABI_TEST(sz_float) &&
		ABI_TEST(sz_double)
	);
	#undef ABI_TEST
}

}
