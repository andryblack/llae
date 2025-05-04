#pragma once

#include <new>
#include <cstdint>
#include <utility>
#include <cassert>

namespace common {

    template <typename Func,size_t Size=4>
    class inplace_function;

    template <size_t Size,typename Res,typename ...Args>
    class inplace_function<Res (Args...),Size> {
    private:
        void*& vtbl() { return m_storage.ptrs[0]; }
        const void*const& vtbl() const { return m_storage.ptrs[0]; }
        void* data() { return m_storage.data; }
        class ImplBase {
        public:
            virtual ~ImplBase() {}
            virtual Res call(Args...args) = 0;
            virtual void move(void* to) = 0;
        };
        alignas(ImplBase) union {
            void* ptrs[Size];
            uint8_t data[sizeof(void*)*Size]; 
        } m_storage;
        ImplBase* impl() { assert(vtbl()); return static_cast<ImplBase*>(data()); }
        template <typename Func>
        class Impl : public ImplBase {
            Func m_func;
        public:
            explicit Impl(Func&& func) : m_func(std::move(func)) {
                static_assert(sizeof(Impl) <= sizeof(m_storage), "too big");
            }
            virtual Res call(Args...args) override {
                return m_func(std::forward<Args>(args)...);
            }
            virtual void move(void* to) override {
                new (to) Impl(std::move(m_func));
            }
        };
        void destroy() {
            if (vtbl()) {
                impl()->~ImplBase();
                vtbl() = nullptr;
            }
        }
    public:
        inplace_function() {
            vtbl() = nullptr;
        }
        ~inplace_function() {
            destroy();
        }
        inplace_function(const inplace_function&) = delete;
        inplace_function& operator = (const inplace_function&) = delete;
        inplace_function(std::nullptr_t) {
            vtbl() = nullptr;
        }
        inplace_function(inplace_function&& func) {
            if (func.vtbl()) {
                func.impl()->move(data());
                func.destroy();
            } else {
                vtbl() = nullptr;
            }
        }
        inplace_function& operator = (inplace_function&& func) {
            if (&func != this) {
                destroy();
                if (func.vtbl()) {
                    func.impl()->move(data());
                    func.destroy();
                }
            }
            return *this;
        }
        template <typename Func>
        inplace_function(Func&& func) {
            new (data()) Impl<Func>(std::forward<Func>(func));
        }
        Res operator () (Args...args) {
            return impl()->call(std::forward<Args>(args)...);
        }
        operator bool () const {
            return vtbl();
        }
        void reset() {
            destroy();
        }
    };

}