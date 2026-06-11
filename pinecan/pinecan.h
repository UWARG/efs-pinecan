#pragma once

#include "pinecanCommon.h"
#include "pinecanBoard.h"

#ifndef PINECAN_DEBUG
# define PINECAN_DEBUG 0
#endif

#ifndef PINECAN_ASSERT
# if PINECAN_DEBUG
#  include <assert.h>
#  define PINECAN_ASSERT(x) assert(x)
# else
#  define PINECAN_ASSERT(x) (void)(x)
# endif
#endif

#ifndef PINECAN_UNUSED
# define PINECAN_UNUSED(x) (void)(x)
#endif
