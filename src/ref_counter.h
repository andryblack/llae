#ifndef __LLAE_REF_COUNTER_H_INCLUDED__
#define __LLAE_REF_COUNTER_H_INCLUDED__

#include <cstring>


extern "C" {
#include <lua.h>
}


class RefCounter {
private:
	static size_t m_num_objects;
	size_t m_refs;
	RefCounter(const RefCounter&);
	RefCounter& operator = (const RefCounter&);
public:
	RefCounter();
	virtual ~RefCounter();
	void add_ref();
	void remove_ref();
	static size_t get_num_objects() { return m_num_objects; }
protected:
	virtual void on_release();
};

template <class T>
class Ref {
private:
	T* m_ptr;
	void release() {
		if (m_ptr) {
			m_ptr->remove_ref();
			m_ptr = 0;
		}
	}
public:
	Ref() : m_ptr(0) {}
	Ref(T* ptr) : m_ptr(ptr) {
		if (m_ptr) m_ptr->add_ref();
	}
	~Ref() {
		release();
	}

	T* get() const { return m_ptr; }

	Ref(const Ref& other) : m_ptr(other.get()) {
		if (m_ptr) m_ptr->add_ref();
	}
	Ref& operator = (const Ref& other) {
		if (other.get()!=m_ptr) {
            release();
            m_ptr = other.get();
            if (m_ptr) m_ptr->add_ref();
        }
        return *this;
	}

	typedef T* Ref::*unspecified_bool_type;
        
    operator unspecified_bool_type() const {
        return m_ptr ? &Ref::m_ptr : 0;
    }

    void reset() {
        release();
    }
    void reset(T* ptr) {
        release();
        m_ptr = ptr;
        if (m_ptr) m_ptr->add_ref();
    }
    T* operator -> () const { return get(); }
	T & operator*() const { return *get(); }

	static int gc(lua_State* L)  {
		Ref* ref = static_cast<Ref*>(lua_touserdata(L,1));
		ref->release();
		return 0;
	}
	static T* get_ptr(lua_State* L,int idx) {
		// @todo check metatable
		Ref* ref = static_cast<Ref*>(lua_touserdata(L,idx));
		return ref->get();
	}
	static Ref get_ref(lua_State* L,int idx) {
		return *static_cast<Ref*>(lua_touserdata(L,idx));
	}
	
};

#endif /*__LLAE_REF_COUNTER_H_INCLUDED__*/