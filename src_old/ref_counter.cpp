#include "ref_counter.h"
#include <cassert>

size_t RefCounter::m_num_objects = 0;

RefCounter::RefCounter() : m_refs(0) {
	++m_num_objects;
}

RefCounter::~RefCounter() {
	--m_num_objects;
	assert(m_refs == 0);
}

void RefCounter::add_ref() {
	++m_refs;
}

void RefCounter::remove_ref() {
	assert(m_refs > 0);
	--m_refs;
	if (m_refs == 0) {
		on_release();
	}
}

void RefCounter::on_release() {
	delete this;
}
