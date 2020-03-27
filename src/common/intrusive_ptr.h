#ifndef __LAE_COMMON_INTRUSIVE_PTR_H_INCLUDED__
#define __LAE_COMMON_INTRUSIVE_PTR_H_INCLUDED__

namespace common {
	template <class T>
    class intrusive_ptr {
    private:
        T* m_ptr;
        void release() {
            if (m_ptr) {
                m_ptr->remove_ref();
                m_ptr = nullptr;
            }
        }
        template<class U> friend class intrusive_ptr;
    public:
        intrusive_ptr() : m_ptr(nullptr) {}
        template <class U>
        explicit intrusive_ptr(U* ptr) : m_ptr(ptr) {
            if (m_ptr) m_ptr->add_ref();
        }
        ~intrusive_ptr() {
            release();
        }

        T* get() const { return m_ptr; }
        template <class U>
        intrusive_ptr(const intrusive_ptr<U>& other) : m_ptr(other.get()) {
            if (m_ptr) m_ptr->add_ref();
        }
        intrusive_ptr(const intrusive_ptr& other) : m_ptr(other.get()) {
            if (m_ptr) m_ptr->add_ref();
        }
        template <class U>
        intrusive_ptr( intrusive_ptr<U>&& other) : m_ptr(other.get()) {
            other.m_ptr = nullptr;
        }
        intrusive_ptr( intrusive_ptr&& other) : m_ptr(other.get()) {
            other.m_ptr = nullptr;
        }
        template <class U>
        intrusive_ptr& operator = (const intrusive_ptr<U>& other) {
            if (other.get()!=m_ptr) {
                release();
                m_ptr = other.get();
                if (m_ptr) m_ptr->add_ref();
            }
            return *this;
        }
        intrusive_ptr& operator = (const intrusive_ptr& other) {
            if (other.get()!=m_ptr) {
                release();
                m_ptr = other.get();
                if (m_ptr) m_ptr->add_ref();
            }
            return *this;
        }
        template <class U>
        intrusive_ptr& operator = ( intrusive_ptr<U>&& other) {
            if (other.get()!=m_ptr) {
                release();
                m_ptr = other.get();
            }
            other->m_ptr = nullptr;
            return *this;
        }
        intrusive_ptr& operator = ( intrusive_ptr&& other) {
            if (other.get()!=m_ptr) {
                release();
                m_ptr = other.get();
            }
            other->m_ptr = nullptr;
            return *this;
        }

        typedef T* intrusive_ptr::*unspecified_bool_type;
            
        operator unspecified_bool_type() const {
            return m_ptr ? &intrusive_ptr::m_ptr : 0;
        }

        void reset() {
            release();
        }
        template <class U>
        void reset(U* ptr) {
            if (ptr) ptr->add_ref();
            release();
            m_ptr = ptr;
        }
        T* operator -> () const { assert(m_ptr); return get(); }
        T & operator*() const { assert(m_ptr); return *get(); }
        
    };
}

#endif /*__COMMON_INTRUSIVE_PTR_H_INCLUDED__*/
