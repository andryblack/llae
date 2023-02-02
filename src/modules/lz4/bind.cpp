#include "uncompress.h"
#include "lua/bind.h"


class decompress_raw_work : public uv::lua_cont_work {
private:
    uv::buffer_base_ptr m_src_data;
    uv::buffer_ptr m_dst_data;
    int m_result = 0;
protected:
    virtual void on_work() override {
        m_result = LZ4_decompress_safe(static_cast<const char*>(m_src_data->get_base()),
            static_cast<char*>(m_dst_data->get_base()),
            m_src_data->get_len(),m_dst_data->get_capacity());
        if (m_result >= 0) {
            m_dst_data->set_len(m_result);
        }
    }
    virtual int resume_args(lua::state& l,int status) override {
        int args;
        if (status < 0) {
            l.pushnil();
            uv::push_error(l,status);
            args = 2;
        } else if (m_result < 0) {
            l.pushnil();
            l.pushstring("decompress failed");
            args = 2;
        } else {
            lua::push(l,std::move(m_dst_data));
            args = 1;
        }
        return args;
    }
public:
    explicit decompress_raw_work(lua::ref&& cont,uv::buffer_base_ptr&& src,size_t dst_buffer_size) : uv::lua_cont_work(std::move(cont)),m_src_data(std::move(src)) {
        m_dst_data = uv::buffer::alloc(dst_buffer_size);
    }
};

class compress_raw_work : public uv::lua_cont_work {
private:
    uv::buffer_base_ptr m_src_data;
    uv::buffer_ptr m_dst_data;
    int m_result = 0;
protected:
    virtual void on_work() override {
        m_result = LZ4_compress_default(static_cast<const char*>(m_src_data->get_base()),
            static_cast<char*>(m_dst_data->get_base()),
            m_src_data->get_len(),m_dst_data->get_capacity());
        if (m_result >= 0) {
            m_dst_data->set_len(m_result);
        }
    }
    virtual int resume_args(lua::state& l,int status) override {
        int args;
        if (status < 0) {
            l.pushnil();
            uv::push_error(l,status);
            args = 2;
        } else if (m_result < 0) {
            l.pushnil();
            l.pushstring("compress failed");
            args = 2;
        } else {
            lua::push(l,std::move(m_dst_data));
            args = 1;
        }
        return args;
    }
public:
    explicit compress_raw_work(lua::ref&& cont,uv::buffer_base_ptr&& src) :uv::lua_cont_work(std::move(cont)),m_src_data(std::move(src))  {
        m_dst_data = uv::buffer::alloc(LZ4_compressBound(m_src_data->get_len()));
    }
};

static lua::multiret lua_lz4decompress_raw(lua::state& l) {
    auto buf = uv::buffer_base::get(l,1,true);
    auto dst_size = l.checkinteger(2);
    if (!buf) {
        l.argerror(1,"need data");
    }
    if (!l.isyieldable()) {
        l.pushnil();
        l.pushstring("decompress_raw is async");
        return {2};
    }
    
    {
        l.pushthread();
        lua::ref cont;
        cont.set(l);
        common::intrusive_ptr<decompress_raw_work> work(new decompress_raw_work(std::move(cont),std::move(buf),dst_size));
        int r = work->queue_work(l);
        if (r < 0) {
            work->reset(l);
            l.pushnil();
            uv::push_error(l,r);
            return {2};
        }
    }
    
    l.yield(0);
    return {0};
}

static lua::multiret lua_lz4compress_raw(lua::state& l) {
    auto buf = uv::buffer_base::get(l,1,true);
    if (!buf) {
        l.argerror(1,"need data");
    }
    if (!l.isyieldable()) {
        l.pushnil();
        l.pushstring("compress_raw is async");
        return {2};
    }
    
    {
        l.pushthread();
        lua::ref cont;
        cont.set(l);
        common::intrusive_ptr<compress_raw_work> work(new compress_raw_work(std::move(cont),std::move(buf)));
        int r = work->queue_work(l);
        if (r < 0) {
            work->reset(l);
            l.pushnil();
            uv::push_error(l,r);
            return {2};
        }
    }
    
    l.yield(0);
    return {0};
}

int luaopen_archive_lz4(lua_State* L) {
    lua::state l(L);
    
    lua::bind::object<archive::lz4uncompress>::register_metatable(l,&archive::lz4uncompress::lbind);
    lua::bind::object<archive::lz4uncompress_read>::register_metatable(l,&archive::lz4uncompress_read::lbind);
    lua::bind::object<archive::lz4uncompress_to_stream>::register_metatable(l,&archive::lz4uncompress_to_stream::lbind);
    
    l.createtable();
    lua::bind::object<archive::lz4uncompress>::get_metatable(l);
    l.setfield(-2,"lz4uncompress");
    lua::bind::object<archive::lz4uncompress_read>::get_metatable(l);
    l.setfield(-2,"lz4uncompress_read");
    lua::bind::object<archive::lz4uncompress_to_stream>::get_metatable(l);
    l.setfield(-2,"lz4uncompress_to_stream");

    lua::bind::function(l,"new_lz4_read",&archive::lz4uncompress_read::new_decompress);
    lua::bind::function(l,"new_lz4_to_stream",&archive::lz4uncompress_to_stream::new_decompress);

    lua::bind::function(l,"decompress_raw",&lua_lz4decompress_raw);
    lua::bind::function(l,"compress_raw",&lua_lz4compress_raw);

    return 1;
}

