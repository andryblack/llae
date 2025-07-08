#pragma once

#include "archive/common.h"
#include <zstd.h>

namespace archive{ 

	lua::multiret zstd_compress(lua::state& l);
}