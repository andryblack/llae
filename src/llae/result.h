#pragma once

#include "error.h"
#include <memory>
#include "lua/stack.h"
#include "common/optional_storage.h"

namespace llae {

	template <typename Result = void>
	class result {
	private:
		typename common::optional_storage<Result>::type m_result;
		error_ptr m_error;
	public:
		explicit result(Result& res) : m_result(res) {}
		result(Result&& res) : m_result(std::move(res)) {}
		result(error_ptr&& err) : m_error(std::move(err)) {}
		result(const error_ptr& err) : m_error(err) {}

		bool has_result() const { return common::optional_storage<Result>::has_value(m_result); }
		Result& get_result() { return common::optional_storage<Result>::get(m_result); }
		const error_ptr& get_error() const { return m_error; }

		int push(lua::state& s) const {
			if (common::optional_storage<Result>::has_value(m_result)) {
				return lua::push(s,common::optional_storage<Result>::get(m_result));
			} else if (m_error) {
				s.pushnil();
				return lua::stack<error_ptr>::push(s,m_error) + 1;
			} else {
				s.pushnil();
				return 1;
			}
		}
	};

	template <>
	class result<void> {
	private:
		error_ptr m_error;
	public:
		explicit result() {}
		result(error_ptr&& err) : m_error(std::move(err)) {}
		const error_ptr& get_error() const { return m_error; }
		int push(lua::state& s) const {
			if (m_error) {
				s.pushnil();
				return lua::stack<error_ptr>::push(s,m_error) + 1;
			} else {
				s.pushboolean(true);
				return 1;
			}
		}
	};

}

namespace lua {

	template <typename Result>
	struct stack<llae::result<Result>> {
		static int push(state& s,const llae::result<Result>& r) {
			return r.push(s);
		}
	};

}