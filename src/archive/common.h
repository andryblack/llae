#ifndef __LLAE_ARCHIVE_COMMON_H_INCLUDED__
#define __LLAE_ARCHIVE_COMMON_H_INCLUDED__

#include "common/intrusive_ptr.h"
#include "uv/buffer.h"
#include "uv/mutex.h"
#include "uv/work.h"
#include "uv/fs.h"
#include "uv/async.h"
#include "uv/luv.h"
#include "uv/stream.h"
#include "meta/object.h"
#include "lua/state.h"
#include "lua/stack.h"
#include "llae/app.h"
#include "llae/memory.h"

#include <vector>


namespace archive { 

	struct alloc_tag {};

	namespace impl {
	
	static const size_t COMPRESSED_BLOCK_SIZE = 1024 * 16;



	template <class T,class Lib>
	class compressionstream : public meta::object {
		LLAE_NAMED_ALLOC(alloc_tag)
		using compressionstream_ptr = common::intrusive_ptr<compressionstream> ;
		
		class compress_work : public uv::work {
			LLAE_NAMED_ALLOC(alloc_tag)
		protected:
            compressionstream_ptr m_stream;
			int m_z_status = Lib::OK;
			uv::buffer_ptr m_out;
			void flush_out(typename Lib::stream& z) {
				if (m_out) {
					m_stream->add_data(std::move(m_out));
				}
				m_out = std::move(uv::buffer::alloc(COMPRESSED_BLOCK_SIZE));
                Lib::fill_out(z,m_out->get_base(),m_out->get_len());
			}
			virtual void on_after_work(int status) override {
				auto& s(llae::app::get(get_loop()).lua());
				if (m_stream) {
	                m_stream->on_work_complete(s,status,m_z_status);
				}
			}
			explicit compress_work(compressionstream_ptr&& compress) : m_stream(std::move(compress)) {}
		};


		class compress_buffers : public compress_work {
			LLAE_NAMED_ALLOC(alloc_tag)
		private:
			uv::write_buffers m_buffers;
		protected:
			
			virtual void on_work() override {
				auto& z(this->m_stream->m_z);
				for (auto& b:m_buffers.get_buffers()) {
                    Lib::fill_in(z,b.base,b.len);
					
					while(Lib::has_in(z)) {
						if (!Lib::has_out(z)) {
							this->flush_out(z);
						}
						int r = T::process(&z,Lib::NO_FLUSH,*this->m_stream);
                        if (r == Lib::STREAM_END) {
                            this->m_z_status = r;
                            break;
                        }
						if (r != Lib::OK && r != Lib::BUF_ERROR) {
							this->m_z_status = r;
							return;
						}
					}
                    if (this->m_z_status == Lib::STREAM_END) {
                        break;
                    }
				}
	            if (this->m_out) {
                    this->m_out->set_len(COMPRESSED_BLOCK_SIZE-z.avail_out);
                    this->m_stream->add_data(std::move(this->m_out));
	            }
	            z.avail_out = 0;
			}
			virtual void on_after_work(int status) override {
				auto& s(llae::app::get(this->get_loop()).lua());
				m_buffers.reset(s);
				compress_work::on_after_work(status);
			}
		public:
			compress_buffers(compressionstream_ptr&& stream) : compress_work(std::move(stream)) {
			}
			void put(lua::state& l) {
				m_buffers.put(l);
			}
		};

		class compress_finish : public compress_buffers {
			LLAE_NAMED_ALLOC(alloc_tag)
		protected:
			virtual void on_work() override {
				compress_buffers::on_work();
				if (this->m_z_status != Lib::OK)
					return;
				auto& z(this->m_stream->m_z);
				while (true) {
					if (!Lib::has_out(z)) {
						this->flush_out(z);
					}
					int r = T::process(&z,Lib::FINISH,*this->m_stream);
					if (r == Lib::STREAM_END) {
	                    if (this->m_out) {
                            this->m_out->set_len(COMPRESSED_BLOCK_SIZE-z.avail_out);
                            this->m_stream->add_data(std::move(this->m_out));
	                    }
						return;
					}
					if (r != Lib::OK && r != Lib::BUF_ERROR) {
                        this->m_z_status = r;
						return;
					}
				}
			}
			virtual void on_after_work(int status) override{
				compress_buffers::on_after_work(status);
				if (this->m_stream) {
                    this->m_stream->on_finish(llae::app::get(this->get_loop()).lua());
				}
			}
		public:
			compress_finish(compressionstream_ptr&& stream) : compress_buffers(std::move(stream)) {}
		};


		class write_file_pipe : public compress_work {
			LLAE_NAMED_ALLOC(alloc_tag)
		private:
			static const size_t BUFFER_SIZE = 1024 * 8;
			uv::file_ptr m_file;
			
			int64_t m_offset = 0;

			uv_fs_t	m_fs_req;
			uv::buffer_ptr m_read_buffer;
			uv::buffer_ptr m_compress_buffer;

			bool m_compress_active = false;
			bool m_read_active = false;
			bool m_eof = false;
			
			static void fs_cb(uv_fs_t* req) {
				write_file_pipe* self = static_cast<write_file_pipe*>(req->data);
				self->on_read(llae::app::get(req->loop).loop(),uv_fs_get_result(req));
			}
			void on_end(uv::loop& l,int status) {
				if (m_compress_active || m_read_active)
					return;
                this->m_stream->on_finish(llae::app::get(l).lua());
				this->remove_ref();
			}
			
			int start_compress(uv::loop& l) {
				m_compress_active = true;
	            if (!m_eof) {
	                m_compress_buffer = std::move(m_read_buffer);
	            } else {
	                m_compress_buffer.reset();
	            }
				int r = this->queue_work(l);
				if (r < 0) {
					m_compress_active = false;
				}
				return r;
			}

			virtual void on_work() override {
				auto& z(this->m_stream->m_z);
				if (m_compress_buffer) {
                    Lib::fill_in(z,m_compress_buffer->get_base(),m_compress_buffer->get_len());
					
					while(Lib::has_in(z)) {
						if (!Lib::has_out(z)) {
							this->flush_out(z);
						}
						int r = T::process(&z,Lib::NO_FLUSH,*this->m_stream);
						if (r != Lib::OK && r != Lib::BUF_ERROR) {
                            this->m_z_status = r;
							return;
						}
					}
				} else {
					while (true) {
						if (!Lib::has_out(z)) {
							this->flush_out(z);
						}
						int r = T::process(&z,Lib::FINISH,*this->m_stream);
						if (r == Lib::STREAM_END) {
		                    if (this->m_out) {
                                this->m_out->set_len(COMPRESSED_BLOCK_SIZE-z.avail_out);
                                this->m_stream->add_data(std::move(this->m_out));
		                    }
							return;
						}
						if (r != Lib::OK && r != Lib::BUF_ERROR) {
                            this->m_z_status = r;
							return;
						}
					}
				}
			}
			virtual void on_after_work(int status) override {
				m_compress_active = false;
				m_read_buffer = std::move(m_compress_buffer);
	            if (m_read_buffer) {
	                m_read_buffer->set_len(BUFFER_SIZE);
	            }
				auto& l(llae::app::get(this->get_loop()).loop());
	            auto& s(llae::app::get(this->get_loop()).lua());
	            if (status < 0 || ((this->m_z_status!=Lib::OK&&this->m_z_status!=Lib::BUF_ERROR))) {
                    this->m_stream->on_work_complete(s,status,this->m_z_status);
	            } else if (m_eof){
                    this->m_stream->on_work_complete(s,status,this->m_z_status);
	                on_end(l,status);
	            } else if (status>=0 && (this->m_z_status==Lib::OK||this->m_z_status==Lib::BUF_ERROR)) {
	                int r = start_read(l);
	                if (r < 0) {
	                    on_end(l,r);
	                }
	            }
			}
			int start_read(uv::loop& l) {
				m_read_active = true;
				if (!m_read_buffer) {
					m_read_buffer = uv::buffer::alloc(BUFFER_SIZE);
				}
				int r = uv_fs_read(l.native(),&m_fs_req,m_file->get(),
					m_read_buffer->get(),1,m_offset,&fs_cb);
				if (r < 0) {
					m_read_active = false;
				}
				return r;
			}

			void on_read(uv::loop& l,int status) {
				m_read_active = false;
				if (status < 0) {
					return on_end(l,status);
				}
				if (status > 0) {
					m_offset += status;
					m_read_buffer->set_len(status);
					int r = start_compress(l);
					if (r < 0) 
						return on_end(l,r);
				} else {
					m_eof = true;
					int r = start_compress(l);
					if (r < 0) 
						return on_end(l,r);
				}
			}
			
		protected:
			~write_file_pipe() {
			}
		public:
			write_file_pipe(compressionstream_ptr&& stream,uv::file_ptr&& f) :
				compress_work(std::move(stream)),m_file(std::move(f)) {
				m_fs_req.data = this;
			}
			int start(uv::loop& l) {
				this->add_ref();
				int r = start_read(l);
				if (r < 0) {
					this->remove_ref();
				}
				return r;
			}
		};

		class async_resume_read : public uv::async {
            compressionstream* m_self;
            LLAE_NAMED_ALLOC(alloc_tag)
		public:
			async_resume_read(uv::loop& l,compressionstream* s) : uv::async(l),m_self(s) {}
			virtual void on_async() override {
				if (m_self) {
					m_self->continue_read(llae::app::get(get_loop()).lua());
				}
			}
			void reset() {
				m_self = nullptr;
			}
		};


		using compress_work_ptr = common::intrusive_ptr<compress_work>;
		void on_work_complete(lua::state& l,int status,int z_err) {
			if (status < 0 || (z_err != Lib::OK && z_err != Lib::BUF_ERROR)) {
                if (z_err == Lib::STREAM_END) {
                    m_finished = true;
                    if (m_read_resume) {
                        m_read_resume->reset();
                    }
                    m_read_resume.reset();
                } else {
                    m_is_error = true;
                }
			}
			if (m_write_cont.valid()) {
				m_write_cont.push(l);
				auto toth = l.tothread(-1);
				l.pop(1);// thread
				int args;
				if (status < 0) {
					toth.pushnil();
					uv::push_error(toth,status);
					args = 2;
				} else if (z_err != Lib::OK && z_err != Lib::BUF_ERROR && z_err != Lib::STREAM_END) {
					toth.pushnil();
                    Lib::pusherror(toth,m_z,z_err);
					args = 2;
				} else {
					toth.pushboolean(true);
					args = 1;
				}
				m_write_cont.reset(l);
				auto s = toth.resume(l,args);
				if (s != lua::status::ok && s != lua::status::yield) {
					llae::app::show_error(toth,s);
				}
			}
			continue_read(l);
		}
		void on_finish(lua::state& l) {
			m_finished = true;
            if (m_read_resume) {
                m_read_resume->send();
            }
            m_read_resume.reset();
			continue_read(l);
		}
		void add_data(uv::buffer_ptr&& b) {
			uv::scoped_lock l(m_mutex);
			if (!m_processed.empty()) {
				size_t tail = m_processed.back()->get_capacity()-m_processed.back()->get_len();
				if (tail >= b->get_len()) {
					::memcpy(m_processed.back()->get_end(),b->get_base(),b->get_len());
					m_processed.back()->set_len(m_processed.back()->get_len()+b->get_len());
					b.reset();
				}
			}
			if (b) {
				m_processed.emplace_back(std::move(b));
			}
			if (m_read_resume) {
				m_read_resume->send();
			}
		}
		
	protected:
		lua::ref m_write_cont;
		common::intrusive_ptr<async_resume_read> m_read_resume;
		typename Lib::stream m_z;
		
		std::vector<uv::buffer_ptr> m_processed;
		uv::mutex m_mutex;
		bool m_is_error = false;
		bool m_finished = false;
		virtual void continue_read(lua::state& l) = 0;
	public:
		compressionstream() {
			memset(&m_z, 0, sizeof(m_z));
		}
		~compressionstream() {
			if (m_read_resume) {
				m_read_resume->reset();
			}
		}
        void init_common(uv::loop& l) {
            m_read_resume.reset( new async_resume_read(l,this) );
        }
		bool is_error() const { return m_is_error; }
		lua::multiret write(lua::state& l) {
			if (is_error()) {
	            l.pushnil();
	            l.pushstring("zlibsteam::write is error");
	            return {2};
	        }
	        if (m_finished) {
	            l.pushnil();
	            l.pushstring("zlibsteam::write is finished");
	            return {2};
	        }
	        if (!l.isyieldable()) {
	            l.pushnil();
	            l.pushstring("zlibsteam::write is async");
	            return {2};
	        }
	        if (m_write_cont.valid()) {
	            l.pushnil();
	            l.pushstring("zlibsteam::write async not completed");
	            return {2};
	        }
	        {
	            l.pushthread();
	            m_write_cont.set(l);
	            
	            common::intrusive_ptr<compress_buffers> work(new compress_buffers(compressionstream_ptr(this)));
	            l.pushvalue(2);
	            work->put(l);
	            int r = work->queue_work(llae::app::get(l).loop());
	            if (r < 0) {
					l.pushnil();
					uv::push_error(l,r);
					return {2};
				} 
	        }
	        l.yield(0);
	        return {0};
		}
		lua::multiret finish(lua::state& l) {
			if (is_error()) {
	            l.pushnil();
	            l.pushstring("zlibcompress::finish is error");
	            return {2};
	        }
	        
	        if (!l.isyieldable()) {
	            l.pushnil();
	            l.pushstring("zlibcompress::finish is async");
	            return {2};
	        }
	        if (m_write_cont.valid()) {
	            l.pushnil();
	            l.pushstring("zlibcompress::finish async not completed");
	            return {2};
	        }
	        if (m_finished) {
	        	if (l.gettop()>1) {
		            l.pushnil();
		            l.pushstring("zlibcompress::finish is finished");
		            return {2};
		        } else {
		        	l.pushboolean(true);
		        	return {1};
		        }
	        }
	        {
	            l.pushthread();
	            m_write_cont.set(l);
	            
	            common::intrusive_ptr<compress_buffers> work(new compress_finish(compressionstream_ptr(this)));
	            if (l.gettop()>1) {
	            	l.pushvalue(2);
	            	work->put(l);
	            }
	            int r = work->queue_work(llae::app::get(l).loop());
	            if (r < 0) {
					l.pushnil();
					uv::push_error(l,r);
					return {2};
				} 
	        }
	        l.yield(0);
	        return {0};
		}
		lua::multiret send(lua::state& l) {
			if (is_error()) {
	            l.pushnil();
	            l.pushstring("zlibcompress::send is error");
	            return {2};
	        }
	        if (m_finished) {
	            l.pushnil();
	            l.pushstring("zlibcompress::send is finished");
	            return {2};
	        }
	        if (!l.isyieldable()) {
	            l.pushnil();
	            l.pushstring("zlibcompress::send is async");
	            return {2};
	        }
	        if (m_write_cont.valid()) {
	            l.pushnil();
	            l.pushstring("zlibcompress::send async not completed");
	            return {2};
	        }
			{
				auto f = lua::stack<common::intrusive_ptr<uv::file> >::get(l,2);
				if (!f) {
					l.argerror(2,"uv::file expected");
				}
				l.pushthread();
				m_write_cont.set(l);

				common::intrusive_ptr<write_file_pipe> req{new write_file_pipe(compressionstream_ptr(this),
					std::move(f))};
				
				int r = req->start(llae::app::get(l).loop());
				if (r < 0) {
					l.pushnil();
					uv::push_error(l,r);
					return {2};
				} 
			}
			l.yield(0);
			return {0};
		}
		
    };

    template <class Base>
    class compressionstream_read : public Base {
    private:
        lua::ref m_read_cont;
        bool m_read_buffer = false;
    protected:
        virtual ~compressionstream_read() {
            m_read_cont.release();
        }
        virtual void continue_read(lua::state& l) override {
            if (!m_read_cont.valid()) {
                return;
            }
            uv::buffer_ptr buf;
            {
                uv::scoped_lock l(this->m_mutex);
                if (!this->m_processed.empty()) {
                    buf = std::move(this->m_processed.front());
                    this->m_processed.erase(this->m_processed.begin());
                }
            }
            if (!buf && !this->m_finished && !this->is_error()) {
                return;
            }
            m_read_cont.push(l);
            auto toth = l.tothread(-1);
            l.pop(1);// thread
            int args;
            if (this->is_error()) {
                this->m_is_error = true;
                toth.pushnil();
                toth.pushstring("compression error");
                args = 2;
            } else {
                if (buf) {
                    if (m_read_buffer) {
                        lua::push(toth,std::move(buf));
                    } else {
                        toth.pushlstring(static_cast<const char*>(buf->get_base()),buf->get_len());
                    }
                } else {
                    toth.pushnil();
                }
                args = 1;
            }
            m_read_cont.reset(l);
            auto s = toth.resume(l,args);
            if (s != lua::status::ok && s != lua::status::yield) {
                llae::app::show_error(toth,s);
            }
        }
    public:
        compressionstream_read() {}
        lua::multiret read(lua::state& l) {
            if (this->is_error()) {
                l.pushnil();
                l.pushstring("zlibsteam::read is error");
                return {2};
            }
            if (m_read_cont.valid()) {
                l.pushnil();
                l.pushstring("zlibsteam::read async not completed");
                return {2};
            }
            uv::buffer_ptr buf;
            {
                uv::scoped_lock l(this->m_mutex);
                if (!this->m_processed.empty()) {
                    buf = this->m_processed.front();
                    this->m_processed.erase(this->m_processed.begin());
                }
            }
            if (buf) {
                l.pushlstring(static_cast<const char*>(buf->get_base()),buf->get_len());
                return {1};
            }
            if (this->m_finished) {
                l.pushnil();
                return {1};
            }
            if (l.gettop()>1 && l.isboolean(2) && !l.toboolean(2)) {
                l.pushstring("");
                return {1};
            }
            if (!l.isyieldable()) {
                l.pushnil();
                l.pushstring("zlibsteam::read is async");
                return {2};
            }
            
            {
                l.pushthread();
                m_read_buffer = false;
                m_read_cont.set(l);
            }
            
            l.yield(0);
            return {0};
        }
        lua::multiret read_buffer(lua::state& l) {
            if (this->is_error()) {
                l.pushnil();
                l.pushstring("compress_steam::read is error");
                return {2};
            }
            if (m_read_cont.valid()) {
                l.pushnil();
                l.pushstring("compress_steam::read async not completed");
                return {2};
            }
            uv::buffer_ptr buf;
            {
                uv::scoped_lock l(this->m_mutex);
                if (!this->m_processed.empty()) {
                    buf = this->m_processed.front();
                    this->m_processed.erase(this->m_processed.begin());
                }
            }
            if (buf) {
                lua::push(l,std::move(buf));
                return {1};
            }
            if (this->m_finished) {
                l.pushnil();
                return {1};
            }
            if (!l.isyieldable()) {
                l.pushnil();
                l.pushstring("compress_steam::read is async");
                return {2};
            }
            
            {
                l.pushthread();
                m_read_buffer = true;
                m_read_cont.set(l);
            }
            
            l.yield(0);
            return {0};
        }
    };

    template <class Base>
    class compressionstream_to_stream : public Base {
    private:
        using compressionstream_to_stream_ptr = common::intrusive_ptr<compressionstream_to_stream>;
        uv::stream_ptr m_stream;
        bool m_need_shutdown = false;
        class write_buffer_req : public uv::write_buffer_req {
            compressionstream_to_stream_ptr m_self;
        public:
            write_buffer_req(compressionstream_to_stream_ptr&& self,uv::buffer_base_ptr&& b) :
                uv::write_buffer_req(uv::stream_ptr(self->m_stream),std::move(b)),
                m_self(std::move(self)) {

                }
            virtual void on_write(int status) override {
                if (status < 0) {
                    uv::print_error(status);
                } else {
                    m_self->continue_read(llae::app::get(get_loop()).lua());
                }
                m_self.reset();
            }
        };
        class shutdown_req : public uv::shutdown_req {
            compressionstream_to_stream_ptr m_self;
        public:
            shutdown_req(compressionstream_to_stream_ptr&& self) :
                uv::shutdown_req(uv::stream_ptr(self->m_stream)),
                m_self(std::move(self)) {

                }
            virtual void on_shutdown(int status) override {
                if (status < 0) {
                    uv::print_error(status);
                } else {
                    m_self->continue_read(llae::app::get(get_loop()).lua());
                }
                m_self.reset();
            }
        };
    protected:
        virtual void continue_read(lua::state& l) override {
            if (!m_stream) {
                return;
            }
            uv::buffer_ptr buf;
            while (true) {
                {
                    uv::scoped_lock l(this->m_mutex);
                    if (!this->m_processed.empty()) {
                        buf = std::move(this->m_processed.front());
                        this->m_processed.erase(this->m_processed.begin());
                    }
                }
                if (!buf && !this->m_finished && !this->is_error()) {
                    return;
                }
                if (this->is_error()) {
                    return;
                }
                if (buf) {
                    common::intrusive_ptr<write_buffer_req> req(new write_buffer_req(
                                                  compressionstream_to_stream_ptr(this),
                        std::move(buf)));
                    buf.reset();
                    int st = req->write();
                    if (st < 0) {
                        uv::print_error(st);
                    }
                } else if (this->m_finished && m_need_shutdown) {
                    common::intrusive_ptr<shutdown_req> req(new shutdown_req(compressionstream_to_stream_ptr(this)));
                    int st = req->shutdown();
                    if (st < 0) {
                        uv::print_error(st);
                    } else {
                        m_stream.reset();
                    }
                } else {
                    break;
                }
            }
        }
    public:
        explicit compressionstream_to_stream( uv::stream_ptr&& stream ) : m_stream(std::move(stream)) {}
        lua::multiret shutdown(lua::state& l) {
            m_need_shutdown = true;
            return this->finish(l);
        }
    };
}}

#endif /*__LLAE_ARCHIVE_ZLIBCOMMON_H_INCLUDED__*/
