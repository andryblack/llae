#include "lua/state.h"
#include "uv/buffer.h"
#include "lua/bind.h"
#include <cstdint>
/*
 * This work is based on the pugixml parser, which is:
 * Copyright (C) 2006-2020, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
*/

namespace utf {

	struct utf8_counter {
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t ch) {
			// U+0000..U+007F
			if (ch < 0x80) return result + 1;
			// U+0080..U+07FF
			else if (ch < 0x800) return result + 2;
			// U+0800..U+FFFF
			else return result + 3;
		}

		static value_type high(value_type result, uint32_t) {
			// U+10000..U+10FFFF
			return result + 4;
		}
	};

	struct utf8_writer {
		typedef uint8_t* value_type;

		static value_type low(value_type result, uint32_t ch) {
			// U+0000..U+007F
			if (ch < 0x80) {
				*result = static_cast<uint8_t>(ch);
				return result + 1;
			}
			// U+0080..U+07FF
			else if (ch < 0x800) {
				result[0] = static_cast<uint8_t>(0xC0 | (ch >> 6));
				result[1] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
				return result + 2;
			}
			// U+0800..U+FFFF
			else {
				result[0] = static_cast<uint8_t>(0xE0 | (ch >> 12));
				result[1] = static_cast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
				result[2] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
				return result + 3;
			}
		}

		static value_type high(value_type result, uint32_t ch) {
			// U+10000..U+10FFFF
			result[0] = static_cast<uint8_t>(0xF0 | (ch >> 18));
			result[1] = static_cast<uint8_t>(0x80 | ((ch >> 12) & 0x3F));
			result[2] = static_cast<uint8_t>(0x80 | ((ch >> 6) & 0x3F));
			result[3] = static_cast<uint8_t>(0x80 | (ch & 0x3F));
			return result + 4;
		}

		static value_type any(value_type result, uint32_t ch) {
			return (ch < 0x10000) ? low(result, ch) : high(result, ch);
		}
	};

	struct utf16_counter {
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t) {
			return result + 1;
		}

		static value_type high(value_type result, uint32_t) {
			return result + 2;
		}
	};

	struct utf16_writer {
		typedef uint16_t* value_type;

		static value_type low(value_type result, uint32_t ch) {
			*result = static_cast<uint16_t>(ch);
			return result + 1;
		}

		static value_type high(value_type result, uint32_t ch) {
			uint32_t msh = static_cast<uint32_t>(ch - 0x10000) >> 10;
			uint32_t lsh = static_cast<uint32_t>(ch - 0x10000) & 0x3ff;

			result[0] = static_cast<uint16_t>(0xD800 + msh);
			result[1] = static_cast<uint16_t>(0xDC00 + lsh);

			return result + 2;
		}

		static value_type any(value_type result, uint32_t ch) {
			return (ch < 0x10000) ? low(result, ch) : high(result, ch);
		}
	};

	struct utf32_counter {
		typedef size_t value_type;

		static value_type low(value_type result, uint32_t) {
			return result + 1;
		}

		static value_type high(value_type result, uint32_t) {
			return result + 1;
		}
	};

	struct utf32_writer {
		typedef uint32_t* value_type;

		static value_type low(value_type result, uint32_t ch) {
			*result = ch;
			return result + 1;
		}

		static value_type high(value_type result, uint32_t ch) {
			*result = ch;
			return result + 1;
		}

		static value_type any(value_type result, uint32_t ch){
			*result = ch;
			return result + 1;
		}
	};

	struct utf16_decoder {
		typedef uint16_t type;

		template <typename Traits> 
		static inline typename Traits::value_type process(const uint16_t* data, size_t size, typename Traits::value_type result, Traits) {
			while (size) {
				uint16_t lead = *data;

				// U+0000..U+D7FF
				if (lead < 0xD800) {
					result = Traits::low(result, lead);
					data += 1;
					size -= 1;
				}
				// U+E000..U+FFFF
				else if (static_cast<unsigned int>(lead - 0xE000) < 0x2000) {
					result = Traits::low(result, lead);
					data += 1;
					size -= 1;
				}
				// surrogate pair lead
				else if (static_cast<unsigned int>(lead - 0xD800) < 0x400 && size >= 2) {
					uint16_t next = data[1];

					if (static_cast<unsigned int>(next - 0xDC00) < 0x400) {
						result = Traits::high(result, 0x10000 + ((lead & 0x3ff) << 10) + (next & 0x3ff));
						data += 2;
						size -= 2;
					}
					else {
						data += 1;
						size -= 1;
					}
				}
				else {
					data += 1;
					size -= 1;
				}
			}

			return result;
		}
	};

	struct utf8_decoder {
		typedef uint8_t type;

		template <typename Traits> static inline typename Traits::value_type process(const uint8_t* data, size_t size, typename Traits::value_type result, Traits) {
			const uint8_t utf8_byte_mask = 0x3f;

			while (size) {
				uint8_t lead = *data;

				// 0xxxxxxx -> U+0000..U+007F
				if (lead < 0x80) {
					result = Traits::low(result, lead);
					data += 1;
					size -= 1;

					// process aligned single-byte (ascii) blocks
					if ((reinterpret_cast<uintptr_t>(data) & 3) == 0)
					{
						// round-trip through void* to silence 'cast increases required alignment of target type' warnings
						while (size >= 4 && (*static_cast<const uint32_t*>(static_cast<const void*>(data)) & 0x80808080) == 0)
						{
							result = Traits::low(result, data[0]);
							result = Traits::low(result, data[1]);
							result = Traits::low(result, data[2]);
							result = Traits::low(result, data[3]);
							data += 4;
							size -= 4;
						}
					}
				}
				// 110xxxxx -> U+0080..U+07FF
				else if (static_cast<unsigned int>(lead - 0xC0) < 0x20 && size >= 2 && (data[1] & 0xc0) == 0x80) {
					result = Traits::low(result, ((lead & ~0xC0) << 6) | (data[1] & utf8_byte_mask));
					data += 2;
					size -= 2;
				}
				// 1110xxxx -> U+0800-U+FFFF
				else if (static_cast<unsigned int>(lead - 0xE0) < 0x10 && size >= 3 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80) {
					result = Traits::low(result, ((lead & ~0xE0) << 12) | ((data[1] & utf8_byte_mask) << 6) | (data[2] & utf8_byte_mask));
					data += 3;
					size -= 3;
				}
				// 11110xxx -> U+10000..U+10FFFF
				else if (static_cast<unsigned int>(lead - 0xF0) < 0x08 && size >= 4 && (data[1] & 0xc0) == 0x80 && (data[2] & 0xc0) == 0x80 && (data[3] & 0xc0) == 0x80) {
					result = Traits::high(result, ((lead & ~0xF0) << 18) | ((data[1] & utf8_byte_mask) << 12) | ((data[2] & utf8_byte_mask) << 6) | (data[3] & utf8_byte_mask));
					data += 4;
					size -= 4;
				}
				// 10xxxxxx or 11111xxx -> invalid
				else {
					data += 1;
					size -= 1;
				}
			}

			return result;
		}
	};

	static lua::multiret utf16_decode(lua::state& l) {
		auto data = uv::buffer_view::get(l,1,true);
		auto src = reinterpret_cast<const uint16_t*>(data.get_base());
		auto len = data.get_len()/2;
		auto res_size = utf16_decoder::process(src,len,0,utf8_counter{});
		std::unique_ptr<char[]> res(new char[res_size]);
		auto write = reinterpret_cast<uint8_t*>(res.get());
		utf16_decoder::process(src,len,write,utf8_writer{});
		l.pushlstring(res.get(),res_size);
		return {1};
	}

	static lua::multiret utf16_encode(lua::state& l) {
		auto data = uv::buffer_view::get(l,1,true);
		auto src = reinterpret_cast<const uint8_t*>(data.get_base());
		auto len = data.get_len();
		auto res_size = utf8_decoder::process(src,len,0,utf16_counter{});
		auto res = uv::buffer::alloc(res_size*sizeof(uint16_t));
		auto write = reinterpret_cast<uint16_t*>(res->get_base());
		utf8_decoder::process(src,len,write,utf16_writer{});
		lua::push(l,std::move(res));
		return {1};
	}

	struct utf32_pusher {
		struct value_type {
			lua::state& l;
			lua_Integer index;
			value_type push(uint32_t ch) {
				l.pushinteger(ch);
				l.seti(-2,index);
				return {l,index+1};
			}
			value_type& operator = (value_type&& d) {
				index = d.index;
				return *this;
			}
			value_type(lua::state& l,lua_Integer idx):l(l),index(idx) {}
			value_type(value_type&& v) : l(v.l),index(v.index) {}
			value_type(const value_type& v) : l(v.l),index(v.index) {}
		};
		static value_type low(value_type result, uint32_t ch) {
			return result.push(ch);
		}
		static value_type high(value_type result, uint32_t ch) {
			return result.push(ch);
		}

		static value_type any(value_type result, uint32_t ch){
			return result.push(ch);
		}
	};

	static lua::multiret utf16_char(lua::state& l) {
		auto len = l.gettop();
		std::unique_ptr<uint16_t[]> src(new uint16_t[len]);
		for (lua_Integer i=1; i<=len; ++i) {
			src.get()[i-1] =  l.tointeger(i);
		}
		auto res_size = utf16_decoder::process(src.get(),len,0,utf8_counter{});
		std::unique_ptr<char[]> res(new char[res_size]);
		auto write = reinterpret_cast<uint8_t*>(res.get());
		utf16_decoder::process(src.get(),len,write,utf8_writer{});
		l.pushlstring(res.get(),res_size);
		return {1};
	}

	static lua::multiret utf16_codes(lua::state& l) {
		auto data = uv::buffer_view::get(l,1,true);
		auto src = reinterpret_cast<const uint16_t*>(data.get_base());
		auto len = data.get_len()/2;
		auto res_size = utf16_decoder::process(src,len,0,utf32_counter{});
		l.createtable(res_size,0);
		utf16_decoder::process(src,len,{l,1},utf32_pusher{});
		return {1};
	}
}



int luaopen_utf16(lua_State* L) {
	lua::state l(L);
	
	l.createtable();

    lua::bind::function(l,"decode",       utf::utf16_decode);
    lua::bind::function(l,"encode",       utf::utf16_encode);
    lua::bind::function(l,"char",         utf::utf16_char);
    lua::bind::function(l,"codes",        utf::utf16_codes);

	return 1;
} 