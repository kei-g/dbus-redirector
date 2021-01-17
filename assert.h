#ifndef __include_assert_h__
#define __include_assert_h__

#include <assert.h>

#ifdef _DEBUG
	#define DR_ASSERT(expr) assert(expr)
#else
	#define DR_ASSERT(expr)
#endif

#endif /* __include_assert_h__ */
