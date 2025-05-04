#pragma once

#include "meta/object.h"
#include <string>
#include "common/intrusive_ptr.h"

namespace llae {

	class error : public meta::object {
		META_OBJECT
	protected:
		static const std::string default_category;
	public:
		virtual const std::string& get_category() const { return default_category; }
		virtual std::string to_string() const;
	};
	using error_ptr = common::intrusive_ptr<error>;

	class string_error : public error {
		META_OBJECT
	private:
		std::string m_message;
	public:
		explicit string_error( std::string_view message ) : m_message(message) {}
		virtual std::string to_string() const override;
		static error_ptr create( std::string_view message ) {
			return common::make_intrusive<string_error>(message);
		}
	};

	class code_error : public error {
		META_OBJECT
	private:
		int m_code = 0;
	public:
		explicit code_error(int code) : m_code(code) {}
		int get_code() const { return m_code; }
		virtual std::string to_string() const override;
	};

}

#include "lua/stack.h"

namespace lua {
	
	template <>
	struct stack<llae::error_ptr> {
		static int push(state& l,const llae::error_ptr& e) {
			if (e) {
				l.pushstring(e->to_string().c_str());
			} else {
				l.pushnil();
			}
			return 1;
		}
	};

}