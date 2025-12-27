#pragma once

#include "archive/common.h"
#include "llae-private/zstd.h"

namespace archive{ 

	lua::multiret zstd_compress(lua::state& l);
}