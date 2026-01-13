#include "logger.h"
#include <cstring>
#include <vector>
#include <algorithm>
#include <iostream>
#include <ctime>
#include "lua/types.h"
#include "uv/fs.h"
#include "llae/buffer.h"
#include "lua/state.h"
#include "lua/bind.h"


META_OBJECT_INFO(llae::log_handler,meta::object);

namespace llae {

    static struct lua_log_level_data {
        const log::level level;
        const char prefix[4];
        char console_prefix[64];
    } lua_log_levels_map[] = {
        {log::level::raw,"", ""},
        {log::level::debug,"[D]", "[D]"},
        {log::level::info,"[I]","[I]"},
        {log::level::warning,"[W]","[W]"},
        {log::level::error,"[E]","[E]"},
        {log::level::fatal,"[F]","[F]"},
    };
    
    static lua_log_level_data* is_lua_log_level(const void* ptr) {
        auto begin = reinterpret_cast<const uint8_t*>(lua_log_levels_map);
        auto p = static_cast<const uint8_t*>(ptr);
        if (p < begin || p >= (begin+sizeof(lua_log_levels_map))) {
            return nullptr;
        }
        return &lua_log_levels_map[(p-begin)/sizeof(lua_log_level_data)];
    }

    static void lua_print(lua::state& l) {
        auto n = l.gettop();
        if (n < 1) {
            return;
        }
        static std::vector<char> message_buf;
        message_buf.clear();
        auto level = log::level::raw;
        int i = 1;
        if (l.get_type(1) == lua::value_type::lightuserdata) {
            auto d = is_lua_log_level(l.touserdata(1));
            if (d) {
                level = d->level;
                i = 2;
            }
        } 
        for (;i<=n;++i) {
            size_t len = 0;
            auto str = l.tolstring(i, len);
            if (str) {
                if (i!=1) {
                    message_buf.emplace_back('\t');
                }
                size_t offset = message_buf.size();
                message_buf.resize(message_buf.size() + len);
                ::memcpy(message_buf.data()+offset,str,len);
            } else {
                message_buf.emplace_back('\t');
            }
        }
        log::write(level, {message_buf.data(),message_buf.size()});
    }

    static void lua_set_console_prefix(lua::state& l) {
        if (l.get_type(1) == lua::value_type::lightuserdata) {
            auto ptr = is_lua_log_level(l.touserdata(1));
            if (ptr) {
                auto str = l.checkstring(2);
                ::strncpy(ptr->console_prefix, str, 63);
            }
        } else if (l.isinteger(1)) {
            log::set_console_prefix(static_cast<log::level>(l.tointeger(1)), l.checkstring(2));
        }
    }

    static std::vector<log_handler_ptr> m_handlers;

    const char* log::level_to_string(log::level level, bool console) {
        switch (level) {
            case log::level::raw:
                return "";
            case log::level::debug: 
            case log::level::info: 
            case log::level::warning: 
            case log::level::error: 
            case log::level::fatal: 
                return console ? lua_log_levels_map[static_cast<size_t>(level)].console_prefix : lua_log_levels_map[static_cast<size_t>(level)].prefix;
        }
        return "[X]";
    }

    void log::set_console_prefix(log::level level,const char* prefix) {
        if (level != log::level::raw) {
            ::strncpy(lua_log_levels_map[static_cast<size_t>(level)].console_prefix, prefix, 63);
        }
    }

    class stdout_log_handler : public log_handler {
    private:
        
    public:
        virtual void write(log::level level,std::string_view message) override {
            if (level == log::level::raw) {
                std::cout << message << std::endl;
            } else {
                std::cout << log::level_to_string(level,true) << " " << message << std::endl;
            }
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
            if (level == log::level::raw) {
                ss << message << std::endl;
            } else {
                ss << log::level_to_string(level,false) << " " << message << std::endl;
            }
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
        virtual void flush() override {
            if (m_file) {
                m_file->fsync(m_loop);
            }
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
            if (level == log::level::raw) {
                ss << m_time_buf << " " << message << std::endl;
            } else {
                ss << m_time_buf << " " << log::level_to_string(level,false) << " " << message << std::endl;
            }
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

    void log::flush() {
        for (auto& handler : m_handlers) {
            handler->flush();
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

    static void level_bind(lua::state& l, const char* name, log::level level) {
        auto ptr = &lua_log_levels_map[static_cast<size_t>(level)];
        l.pushlightuserdata(ptr);
        l.setfield(-2, name);
    }
    void log::lbind(lua::state& l) {
        l.createtable();
        lua::bind::value(l,"debug",level::debug);
        lua::bind::value(l,"info",level::info);
        lua::bind::value(l,"warning",level::warning);
        lua::bind::value(l,"error",level::error);
        lua::bind::value(l,"fatal",level::fatal);
        l.setfield(-2,"level");
        l.createtable();
        level_bind(l,"debug",level::debug);
        level_bind(l,"info",level::info);
        level_bind(l,"warning",level::warning);
        level_bind(l,"error",level::error);
        level_bind(l,"fatal",level::fatal);
        l.setfield(-2,"print_level");
        lua::bind::function(l,"write",&log::write);
        lua::bind::function(l,"add_stdout_handler",&log::add_stdout_handler);
        lua::bind::function(l,"remove_stdout_handler",&log::remove_stdout_handler);
        lua::bind::function(l,"add_file_handler",&log::add_file_handler);
        lua::bind::function(l,"print",&llae::lua_print);
        lua::bind::function(l,"set_console_prefix",lua_set_console_prefix);
    }
}
