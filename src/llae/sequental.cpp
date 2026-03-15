
#include "sequental.h"
#include "common/intrusive_ptr.h"
#include "llae/error.h"
#include "llae/logger.h"

namespace llae {

    class sequental_error : public error {
    protected:
        const char* m_active_op = nullptr;
        const char* m_new_op = nullptr;
    public:
        sequental_error(const char* active, const char* next) : m_active_op(active),m_new_op(next) {}
        virtual std::string to_string() const override {
            char buffer[128] = {0};
            ::snprintf(buffer,sizeof(buffer),"[%s]:failed start %s, %s is active",get_category().c_str(),m_new_op,m_active_op);
            return std::string(buffer);
        }
    };

    class sequental_scope_error : public sequental_error {
    public:
        sequental_scope_error(const char* active, const char* next) : sequental_error(active,next) {}
        virtual std::string to_string() const override {
            char buffer[128] = {0};
            ::snprintf(buffer,sizeof(buffer),"[%s]:failed scope start %s, %s is active",get_category().c_str(),m_new_op,m_active_op);
            return std::string(buffer);
        }
    };

    error_ptr sequental::start_op(const char* new_op) {
        if (m_current_op) {
            return common::make_intrusive<sequental_error>(m_current_op,new_op);
        }
        m_current_op = new_op;
        return error_ptr {};
    }
    void sequental::end_op(const char* name) {
        if (m_current_op) {
            if (::strcmp(m_current_op,name) != 0) {
                LOG_ERROR("sequental error: expected " << m_current_op << " but got " << name);
                assert(false && "sequental error");
            }
        }
        m_current_op = nullptr;
    }

    error_ptr sequental_scope::start(const char* name) {
        if (m_name) {
            return common::make_intrusive<sequental_scope_error>(m_name,name);
        }
        auto err = m_seq.start_op(name);
        if (err) {
            return err;
        }
        m_name = name;
        return error_ptr{};
    }
}