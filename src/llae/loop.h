#pragma once

namespace lua {
    class state;
}

namespace llae {

	class loop {
    public:
        static loop& get(lua::state& l);
	};

}
