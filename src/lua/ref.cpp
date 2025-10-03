#include "ref.h"
#include <cassert>

namespace lua {

    ref::~ref() {
        assert(m_ref == LUA_NOREF);
    }
}
