#include "loop.h"
#include "uv/loop.h"
#include "app.h"

namespace llae {

	loop& loop::get(lua::state& l) {
		return app::get(l).loop();
	}

}