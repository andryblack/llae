#ifndef __LLAE_CONDITION_H_INCLUDED__
#define __LLAE_CONDITION_H_INCLUDED__

#include <uv.h>

class Condition {
protected:
	uv_cond_t	m_cond;
private:
	Condition(const Condition&);
	Condition& operator = (const Condition&);
public:
	Condition() {
		uv_cond_init(&m_cond);
	}
	~Condition() {
		uv_cond_destroy(&m_cond);
	}	
	void wait(MutexBase& m) {
		uv_cond_wait(&m_cond,&m.m_mutex);
	}
	void signal() {
		uv_cond_signal(&m_cond);
	}
};

#endif /*__LLAE_CONDITION_H_INCLUDED__*/