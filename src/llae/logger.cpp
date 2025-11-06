#include "logger.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <ctime>
#include "uv/fs.h"
#include "llae/buffer.h"


META_OBJECT_INFO(llae::log_handler,meta::object);

namespace llae {

    

    static std::vector<log_handler_ptr> m_handlers;

    static const char* level_to_string(log::level level) {
        switch (level) {
            case log::level::debug: return "[D]";
            case log::level::info: return "[I]";
            case log::level::warning: return "[W]";
            case log::level::error: return "[E]";
            case log::level::fatal: return "[F]";
        }
        return "[X]";
    }

    class stdout_log_handler : public log_handler {
    private:
        
    public:
        virtual void write(log::level level,std::string_view message) override {
            std::cout << level_to_string(level) << " " << message << std::endl;
        }
        static log_handler_ptr instance() {
            static log_handler_ptr s_instance = common::make_intrusive<stdout_log_handler>();
            return s_instance;
        }
    };

    class file_log_handler : public log_handler {
    protected:
        uv::loop m_loop;
        uv::file_ptr m_file;
    public:
        explicit file_log_handler(uv::file_ptr file) : m_loop(uv::loop::default_loop()), m_file(std::move(file)) {}
        virtual void write(log::level level, std::string_view message) override {
            std::stringstream ss;
            ss << level_to_string(level) << " " << message << std::endl;
            auto str = ss.str();
            m_file->write(m_loop,llae::buffer_view(str.data(),str.length()));
        }
        virtual bool close() override {
            if (m_file) {
                m_file->close(m_loop);
                m_file.reset();
            }
            return true;
        }
    };
    class time_file_log_handler : public file_log_handler {
        char m_time_buf[32];
    public:
        explicit time_file_log_handler(uv::file_ptr file) : file_log_handler(std::move(file)) {}
        virtual void write(log::level level, std::string_view message) override {
            time_t now = time(nullptr);
            auto tm = localtime(&now);
            strftime(m_time_buf,sizeof(m_time_buf),"%Y-%m-%d %H:%M:%S",tm);
            std::stringstream ss;
            ss << m_time_buf << " " << level_to_string(level) << " " << message << std::endl;
            auto str = ss.str();
            m_file->write(m_loop,llae::buffer_view(str.data(),str.length()));
        }
    };

    void log::add_handler(log_handler_ptr handler) {
        m_handlers.push_back(handler);
    }

    void log::remove_handler(log_handler_ptr handler) {
        m_handlers.erase(std::remove(m_handlers.begin(), m_handlers.end(), handler), m_handlers.end());
    }

    void log::write(log::level level,std::string_view message) {
        for (auto& handler : m_handlers) {
            handler->write(level,message);
        }
    }

    void log::close() {
        for (auto it = m_handlers.begin(); it != m_handlers.end(); ) {
            if ((*it)->close()) {
                it = m_handlers.erase(it);
            } else {
                ++it;
            }
        }
    }

    void log::add_stdout_handler() {
        add_handler(stdout_log_handler::instance());
    }

    void log::remove_stdout_handler() {
        remove_handler(stdout_log_handler::instance());
    }

    log_handler_ptr log::add_file_handler(const uv::file_ptr& file, bool with_time) {
        log_handler_ptr handler;
        if (with_time) {
            handler = common::make_intrusive<time_file_log_handler>(file);
        } else {
            handler = common::make_intrusive<file_log_handler>(file);
        }
        add_handler(handler);
        return handler;
    }
}
