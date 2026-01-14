#pragma once

#include "common/ref_counter.h"
#include "common/intrusive_ptr.h"
#include "meta/object.h"
#include <string_view>
#include <sstream>

namespace uv {
    class file;
    using file_ptr = common::intrusive_ptr<file>;
}
namespace lua {
    class state;
}

namespace llae {

    class log_handler;
    using log_handler_ptr = common::intrusive_ptr<log_handler>;

    struct log {
        enum class level {
            raw,
            debug,
            info,
            warning,
            error,
            fatal,
        };
        static void add_handler(log_handler_ptr handler);
        static void remove_handler(log_handler_ptr handler);
		static void write(level level,std::string_view message);
        static void add_stdout_handler();
        static void remove_stdout_handler();
        static void close();
        static void flush();
        static const char* level_to_string(level leve,bool console);
        static void set_console_prefix(level leve,const char* prefix);
        static log_handler_ptr add_file_handler(const uv::file_ptr& file, bool with_time = false);

        static void lbind(lua::state& l);
	};

	class log_handler : public meta::object {
		META_OBJECT
	public:
        void print(lua::state& l);
		virtual void write(log::level level, std::string_view message) = 0;
        virtual bool close() { return false; }
        virtual void flush() { }
        static void lbind(lua::state& l);
	};

    struct log_builder {
        log::level level;
        log_builder(log::level level) : level(level) {}
        std::stringstream stream;
        template <typename T>
        log_builder& operator<<(const T& value) {
            stream << value;
            return *this;
        }
        ~log_builder() {
            log::write(level,stream.str());
        }
    };

#ifndef NDEBUG
#define LOG_DEBUG(X) do {::llae::log_builder(::llae::log::level::debug) << X;} while(false);
#else
#define LOG_DEBUG(X) do {;} while(false);
#endif
#define LOG_INFO(X) do {::llae::log_builder(::llae::log::level::info) << X;} while(false);
#define LOG_WARNING(X) do {::llae::log_builder(::llae::log::level::warning) << X;} while(false);
#define LOG_ERROR(X) do {::llae::log_builder(::llae::log::level::error) << X;} while(false);
#define LOG_FATAL(X) do {::llae::log_builder(::llae::log::level::fatal) << X;} while(false);

}