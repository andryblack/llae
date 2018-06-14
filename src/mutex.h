#ifndef __LLAE_MUTEX_H_INCLUDED__
#define __LLAE_MUTEX_H_INCLUDED__

#include <uv.h>

class MutexBase {
protected:
	uv_mutex_t	m_mutex;
	MutexBase() {}
	friend class Condition;
private:
	MutexBase(const MutexBase&);
	MutexBase& operator = (const MutexBase&);
public:
	~MutexBase() {
		uv_mutex_destroy(&m_mutex);
	}	
	void lock() {
		uv_mutex_lock(&m_mutex);
	}
	void unlock() {
		uv_mutex_unlock(&m_mutex);
	}
};
class Mutex : public MutexBase {
public:
	Mutex() { uv_mutex_init(&m_mutex); } // todo exception
};

#if defined(UV_VERSION_HEX) && (UV_VERSION_HEX >= 0x011500) 
class RecursiveMutex : public MutexBase {
public:
	RecursiveMutex() { uv_mutex_init_recursive(&m_mutex); } // todo exception
};
#endif

class MutexLock {
private:
	MutexBase& m_mutex;
	MutexLock(const MutexLock&);
	MutexLock& operator = (const MutexLock&);
public:
	explicit MutexLock(MutexBase& m) : m_mutex(m) { m_mutex.lock(); }
	~MutexLock() { m_mutex.unlock(); }
};

#endif /*__LLAE_MUTEX_H_INCLUDED__*/