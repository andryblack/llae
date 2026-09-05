# LLAE Lua binding autogeneration

This document is the source of truth for annotating C++ headers so LLAE can generate Lua bindings and EmmyLua (`---@`) meta files.

Audience: humans and coding models that add or maintain C++/Lua bindings in LLAE or in a project that depends on LLAE.

Do not invent tags, attributes, or macros that are not listed here. If a declaration is not tagged (and is not covered by `@luabind(all=true)`), it is not bound.

## What the generator does

On `llae init` the project tool:

1. Reads headers listed in `bind_headers` / `bind_header` (project and installed modules).
2. Preprocesses them with extra macros (see [Preprocessor defines](#preprocessor-defines)).
3. Parses a C++ subset and collects declarations whose doc comments contain `@luabind` (or that sit under `@luabind(all=true)`).
4. Writes, per Lua module:
   - `build/src/gen_module_<module>.cpp` — `luaopen_<module>()` registration
   - `build/lua-meta/<module>.lua` — EmmyLua annotations

`<module>` is the Lua module name (dots preserved in the filename). The C symbol is `luaopen_<module with dots replaced by _>`.

One physical header should contribute to **one** Lua module. Nested namespaces that would produce a second name must set `module=` (see [Module name](#module-name)).

## How to enable generation in a project

In `llae-project.lua` (paths relative to the project root):

```lua
-- All .h / .hpp under the directory, recursively
bind_headers('src')

-- One file
bind_header('src/mylib/api.h')
```

In a module’s `llae-module.lua`:

```lua
bind_headers = {
  { dir = '${dir}/src/mylib' },
  { filename = '${dir}/src/mylib/extra.h' },
}
```

`${dir}` is the module location. Tokens are expanded from the module/project env.

Generation runs as part of `llae init`. After changing tags, run `llae init` again so `build/src/gen_module_*.cpp` and `build/lua-meta/*.lua` update.

## Where tags go

Tags live in **doc comments immediately before** the declaration they apply to.

Recognized doc comments (only these; ordinary `//` and `/* */` are discarded):

```cpp
/// @luabind
void foo();

/**
 * Brief text used as Lua `---` comment.
 * @luabind(name=bar)
 */
void foo_impl();
```

Rules:

- The comment must sit on the declaration that should be bound, not on a friend, a later overload, or a `.cpp` definition.
- Several consecutive `///` lines before one declaration are concatenated.
- Non-tag text in that comment becomes the Lua description (`---` lines in meta).
- Tag a **public** API only. Private helpers must not have `@luabind`.
- The parser is a C++ **subset**. Keep tagged headers free of heavy templates, SFINAE, and exotic macros. Prefer a dedicated bind header (or `#ifdef LUABIND_PARSE` stubs) over tagging unreadable library headers.

## Tag syntax

Pattern: `@name` or `@name(args)`.

Arguments are a comma-separated list. Each item is either:

- positional: `foo`
- named: `key=value`

Whitespace around names and values is trimmed. Commas inside `()`, `<>`, `[]`, `{}`, and quoted strings do not split arguments.

Several tags may appear in one comment, separated by newlines, spaces, commas, or `*`.

Inline text after a tag (until newline or the next `@tag`) is that tag’s **comment**. `@lparam` / `@lreturn` use that comment as the parameter/return description.

```cpp
/// Creates a digest.
/// @lparam(data,string|llae.buffer_base) Input bytes
/// @lreturn(result,string?)
/// @lreturn(error,string?)
/// @luabind(name=finish,async=true)
llae::result_promise_ptr<llae::buffer_base_ptr> async_finish(llae::loop& a);
```

Multiple `@luabind(...)` on the same declaration are merged; later named keys overwrite earlier ones.

Unknown tag names are stored but ignored by the binder. Do not use them.

## Tags the binder understands

| Tag | On | Repeatable | Role |
|---|---|---|---|
| `@luabind` | class/struct, function/method, field, enum, namespace, using, constexpr/const value | merge | Opt-in + bind options |
| `@lparam` | function/method | yes | Lua parameter list for meta (replaces inference) |
| `@lreturn` | function/method | yes | Lua return list for meta (replaces inference) |
| `@loverload` | function/method | yes | Extra EmmyLua `@overload` line |

`@luabind` is required to generate C++ registration, unless an ancestor namespace/class has `@luabind(all=true)`.

`@lparam` / `@lreturn` / `@loverload` affect **meta only**. They do not change the C++ wrapper.

### `@luabind` attributes

All attributes are named (`key=value`). Boolean flags are the string `true`.

| Attribute | Applies to | Effect |
|---|---|---|
| `name` | anything bound | Lua name. Default: C++ identifier. |
| `module` | anything at module scope | Lua module name. Default: enclosing namespace, `::` → `.`, lowercased. Nested classes use the parent class’s module. |
| `all` | namespace, class/struct | `true`: bind every nested class, method, field, enum, function, value, and `using` even without a local `@luabind`. Nested items may still have their own `@luabind(...)` for options. |
| `hidden` | class/struct | `true`: register the metatable (inheritance) but do **not** export the class as a module field. Use for abstract bases (`handle`, `stream`, …). |
| `async` | function/method | `true`: register with `llae::async_function`. C++ return type must be `llae::result_promise_ptr<T>`. Lua gets a yielding `name` and a promise-returning `async_<name>`. |
| `raw` | constructor (method whose C++ name equals the class name) | `true`: `lua::bind::raw_constructor` (value type, userdata in place). Without `raw`: `lua::bind::constructor` (`new T` + `intrusive_ptr`, for `meta::object`). Both register Lua `new`. |
| `wrapper` | function/method, module value | C++ function/object to take the address of, instead of `prefix::name`. Example: `wrapper=uv_hrtime`. |
| `alias` | function/method | Second Lua name for the same pointer (operators: `__add`, `__tostring`, …). |
| `policy` | function/method, field | Token pasted as `lua::bind::<policy>` in generated code. Must be a complete C++ type in `lua::bind`. |
| `readonly` | field | `true`: generate `lua::bind::field_ro` (no setter). |
| `prefix` | enum | Strip this prefix from each enumerator’s Lua name. |
| `value` | module/class constant | C++ expression bound instead of `prefix::name`. Example: `value=AF_INET`. |
| `type` | **class field only** | Override the C++ type string used for Lua meta field type mapping. |
| `method` | method | If set, bind a C++ `static` method as an instance method (`:` in meta) instead of `.`. |

Not consumed (present in some headers, ignored by the generator): `ltype`. Do not rely on it. Override Lua signatures with `@lparam` / `@lreturn`, or field `type=`.

### `@lparam(name,lua_type)` comment

Positional: `name`, then EmmyLua type.

If **any** `@lparam` is present, the inferred parameter list is discarded and only the tags are used, in order.

Use `@lparam` whenever inference would be wrong or leak C++ types: `lua::multiret`, `lua::state&`, `llae::loop&`, buffers, class pointers, overloads implemented in Lua.

### `@lreturn(name,lua_type)`

Positional: Lua return **name**, then EmmyLua type. Repeat for multiple returns (`result` + `error` is the usual pair).

If **any** `@lreturn` is present, inferred returns are discarded.

### `@loverload(signature)`

One positional argument: the rest of an EmmyLua overload, e.g. `fun(a:string,b:integer):boolean`. Emitted as `---@overload fun(...)`.

## What can be bound

| C++ construct | How it appears in Lua |
|---|---|
| `class` / `struct` with `@luabind` | Module field (unless `hidden=true`). Metatable registered; bases determine registration order. |
| Method | `obj:name(...)` unless C++ `static` (then `Class.name(...)`), unless `method=` is set. |
| Constructor (`T::T(...)`) | Lua `Class.new(...)`. At most one constructor per class. |
| Static factory (`lnew`, …) | Use `name=new`. Does not count as the C++ constructor. |
| Data member | Getter/setter (or getter only if `readonly=true`). |
| `static constexpr` / `static const` member | Class constant (`lua::bind::value`). |
| Namespace `constexpr` / const object | Module constant. |
| `enum` / `enum class` | Lua table of integer (or expression) values. Forward-declared enums are skipped. |
| Free function | Module function. |
| `using Alias = Type` with `@luabind` | Type alias for C++ type rewriting in constructors/signatures. Not a Lua export. |

Untagged friend functions, destructors, and nested types are not bound unless `all=true` covers them.

A method whose C++ name equals the class name is treated as the constructor, not as a regular method.

## Module name

Default: enclosing C++ namespace, `::` replaced by `.`, **lowercased**.

```cpp
namespace crypto {           // module "crypto"
  class bignum { ... };      // crypto.bignum
}

namespace uv { namespace fs {  // module "uv.fs" unless overridden
  void mkdir(...);
}}
```

A class or function with no namespace and no `module=` is an error.

To keep several namespaces in one Lua module (and one generated `.cpp`):

```cpp
/// @luabind(module=uv)
void helper();
```

or put `@luabind(module=uv,all=true)` on the namespace.

## Class kinds

### `meta::object` (refcounted)

Typical LLAE objects (`uv::tcp_connection`, `crypto::hmac`, …):

```cpp
/// @luabind
class hmac : public meta::object {
    META_OBJECT
public:
    /// @lparam(algorithm,string)
    /// @lreturn(instance,crypto.hmac?)
    /// @lreturn(error,string?)
    /// @luabind(name=new)
    static lua::multiret lnew(lua::state& l);
};
```

In the `.cpp` file: `META_OBJECT_INFO(crypto::hmac, meta::object);`.

During parse, `META_OBJECT` is defined empty so the macro does not confuse the parser.

Prefer a static `lnew` + `name=new` when construction needs Lua (optional args, error tuple). Use an untagged C++ constructor plus `@luabind` on `T::T` only when `lua::bind::constructor` can call it (`intrusive_ptr`).

### Value structs

Plain structs stored in userdata:

```cpp
/// @luabind
struct vec2 {
    /// @luabind(raw=true)
    vec2(float x, float y);
    /// @luabind
    float x;
    /// @luabind
    float y;
};
```

`raw=true` is required for value-type constructors.

### Hidden bases

```cpp
/// @luabind(hidden=true)
class handle : public meta::object {
    META_OBJECT
};
/// @luabind
class tcp_connection : public stream { ... };
```

Register the base so derived metatables chain; do not export `uv.handle` if users should not construct it.

## Functions and methods

### Renaming

C++ names often have a Lua prefix (`l`, `async_`). Always set `name=` to the Lua API name:

```cpp
/// @luabind(name=close,async=true)
llae::result_promise_ptr<void> async_close(llae::loop& l);

/// @luabind(name=new)
static lua::multiret lnew(lua::state& l);

/// @luabind(name=__lt)
lua::multiret less(lua::state& l);
```

### `async=true`

C++ signature must return `llae::result_promise_ptr<T>` (or `void` inside the promise).

Registration uses `llae::async_function`, which installs:

- `name` — yield until the promise resolves (`error("is async")` if the caller is not yieldable)
- `async_name` — return the promise object

`llae::loop&` is filled from `llae::loop::get(l)` in the async helper; it is **not** a Lua argument. `lua::state&` is likewise not a Lua argument.

Meta inference does **not** drop `llae::loop&`. For accurate meta, add `@lparam` / `@lreturn` on async APIs that take `loop&`.

```cpp
/// @luabind(name=start,async=true)
llae::result_promise_ptr<void> async_start(llae::loop& a, llae::buffer_base_ptr key);
```

Inferred Lua params would wrongly include `a`. Prefer:

```cpp
/// @lparam(key,string|llae.buffer_base?)
/// @luabind(name=start,async=true)
llae::result_promise_ptr<void> async_start(llae::loop& a, llae::buffer_base_ptr key);
```

### `wrapper=`

Bind a different C++ function than the one whose name was parsed (C API, helper, or a Lua-facing overload):

```cpp
/// @luabind(name=available_parallelism,wrapper=uv_available_parallelism)
int lavailable_parallelism(lua_State* L);

/// @luabind(wrapper=autobind_tests::test_bind_struct_wrapper)
void test_wrapped();
```

The generated code takes `&<wrapper>`. The declaration still exists so the parser has a name and (for meta) a signature. Under `#ifdef LUABIND_PARSE` the declaration may be a stub (`LUABIND_FUNC`).

### `alias=` vs `name=`

- `name=` — the primary Lua name (replaces the C++ identifier).
- `alias=` — **additional** name for the same function (does not replace).

```cpp
/// @luabind(alias=__mul)
bignum_ptr mul(lua::state& l);   // Lua: mul and __mul

/// @luabind(name=__lt)
lua::multiret less(lua::state& l); // Lua: only __lt
```

### `policy=`

Appended as `lua::bind::<policy>` after the function pointer.

Defined in `src/lua/policy.h`:

| Policy | Use |
|---|---|
| `return_ref_policy<N>{}` | Returned object/pointer is a Lua reference to stack index `N` (usually `1` = self). Lifetime follows that userdata. |
| `return_self_ref_policy` | Same as `return_ref_policy<1>`. |
| `return_arg_policy<N>{}` | Ignore C++ return; push stack index `N` again. |
| `return_self_policy` | Same as `return_arg_policy<1>`. |

```cpp
/// @luabind(policy=return_ref_policy<1>{})
test_bind_fields* get_self() { return this; }
```

Do not write `lua::bind::` in the tag; the template adds that prefix.

## Fields and constants

```cpp
/// @luabind
int field1;

/// @luabind(readonly=true)
int const_field = 5;

/// @luabind(policy=field_ref_policy{})
test_bind_struct nested;

/// @luabind(policy=string_policy{})
char string_field[10];

/// @luabind(policy=string_policy<false>{})
uint8_t data_field[10];

/// @luabind
static constexpr int constexpr_value = 123;
```

Field policies (`src/lua/policy.h`):

| Policy | Use |
|---|---|
| (omit) | Copy via `lua::stack<T>`. |
| `field_ref_policy{}` | Nested object/array as a referenced userdata of the parent. |
| `string_policy{}` | Fixed `char[N]` as a Lua string (NUL-trimmed). |
| `string_policy<false>{}` | Fixed byte array as a Lua string of length `N` (no NUL trim). |

A member is exported as a **constant** (`lua::bind::value`) when it is `constexpr` or (`static` and `const`). Otherwise it is a field.

Namespace-level constants:

```cpp
/// @luabind(name=O_RDONLY,value=UV_FS_O_RDONLY)
static constexpr auto LO_RDONLY = UV_FS_O_RDONLY;

#ifdef LUABIND_PARSE
/// @luabind(value=AF_INET)
LUABIND_FIELD(AF_INET)
#endif
```

`value=` is the C++ expression written into generated code. `name=` is the Lua field name.

## Enums

```cpp
/// @luabind
enum class state { off, on };

/// @luabind(prefix=test_module_enum2_)
enum test_module_enum2 {
    test_module_enum2_a,
    test_module_enum2_b,
};
```

- `enum class` values are qualified as `Class::Enum::value` / `ns::Enum::value`.
- Unscoped enumerators use `Class::value` / `ns::value`.
- `prefix=` strips a common enumerator prefix in Lua only (`a`, `b`, not `test_module_enum2_a`).
- Implicit values follow C++ (start at 0, increment; after an explicit value, continue from there).
- Nested enums become `module.Class.Enum` in meta; at namespace scope, `module.Enum`.

## Type mapping (meta inference)

Used when `@lparam` / `@lreturn` are absent.

Skipped C++ parameters (not emitted in Lua meta):

- `lua::state&`
- `llae::app&`

Not skipped (must override with `@lparam` if they are not Lua args): `llae::loop&`.

Built-in C++ → Lua:

| C++ | Lua arg | Lua return |
|---|---|---|
| `int`, `size_t` | `integer` | `integer` |
| `float`, `double` | `number` | `number` |
| `bool` | `boolean` | `boolean` |
| `std::string`, `std::string_view` | `string` | `string` |
| `void` | (none) | (none) |
| `llae::buffer_base_ptr` (and `&` / `&&`) | `string\|llae.buffer_base?` | `llae.buffer_base?` |
| `llae::buffer_view` / `const llae::buffer_view&` | `string\|llae.buffer_base` | `string` |
| `std::optional<T>` | `T?` | `T?` |
| `common::intrusive_ptr<T>` | `T?` | `T?` |
| enum bound in the same module | `module.Enum` | same |
| nested class enum | `module.Class.Enum` | same |
| `lua::multiret` | — | `any` named `result` |

`llae::result<void>` → `boolean?`, `string?` (`result`, `error`).

`llae::result<T>` → mapped `T?` (trailing `?` on `T` stripped first), `string?`.

`llae::result_promise_ptr<T>`:

- without `async=true`: same pair as `llae::result<T>` (`void`/`empty` → `boolean?`)
- with `async=true`: `llae.promise<Inner>` named `promise` (`void`/`empty` → `boolean`)

`using` aliases tagged `@luabind` are substituted before this mapping (and before constructor C++ types are written).

Anything else is copied as the C++ spelling (`crypto::bignum`, `mbedtls_operation_t`, …). That is **wrong** for LuaLS. For those APIs, write `@lparam` / `@lreturn` with Lua types (`crypto.bignum`, `integer`, `string|llae.buffer_base`, …).

Constructor C++ types in generated `lua::bind::constructor<T, ...>` are rewritten with namespace qualification and `using` aliases so `dep` in `arg_types` becomes `arg_types::dep`.

## Preprocessor defines

Always defined while generating (not while compiling the project normally):

| Macro | Expansion |
|---|---|
| `LUABIND_PARSE` | `1` |
| `LUABIND_FIELD(Name)` | `::luabind_autobind_type Name;` |
| `LUABIND_FUNC(Name)` | `::luabind_autobind_type Name();` |
| `LLAE_BINDING_GENERATION` | empty (defined) |
| `META_OBJECT` | empty (defined) |

Use `#ifdef LUABIND_PARSE` for declarations that exist only for the binder (stubs, extra constants, C API wrappers). They must not be compiled into the real binary.

`LUABIND_FIELD` / `LUABIND_FUNC` are **only** defined during generation. They must appear inside `#ifdef LUABIND_PARSE`.

Semantics:

- `LUABIND_FUNC(name)` — stub function. Combine with `@luabind` and usually `wrapper=` / `policy=` / `name=`.
- `LUABIND_FIELD(name)` — stub object. At namespace scope → module value. Inside a class → field, unless you write a real `constexpr`/`const static` declaration instead of the macro.

## Binding an external C/C++ API

When the real header is not suitable to parse, add a bind-only view:

```cpp
#pragma once
#include "real_api.h"

/// @luabind(all=true)
namespace mylib {

#ifdef LUABIND_PARSE

enum class mode { a, b };

struct handle {
    /// @luabind(raw=true)
    handle();
    LUABIND_FUNC(size)
    /// @luabind(readonly=true)
    LUABIND_FIELD(id)
};

/// @luabind(wrapper=mylib_c_version)
LUABIND_FUNC(version)

#endif
}
```

`all=true` binds every stub. Add a local `@luabind(...)` on a stub only to set options (`readonly`, `policy`, `wrapper`, `raw`, `prefix`, …).

Put the bind header on `bind_headers`. Keep the real types in a header the parser does not need to understand.

## Generated C++ (contract)

Per class, a static `luabind_<prefix>_<class>(lua::state&)` that registers constructor, methods, fields, nested enums, constants.

`luaopen_<module>`:

1. `register_metatable` for each class, bases first.
2. Create the module table.
3. Export non-hidden class metatables by Lua class name.
4. Register free functions, enums, module values.

Methods with `async=true` call `llae::async_function`; others `lua::bind::function`. Fields use `field` / `field_ro`.

Do not hand-edit `build/src/gen_module_*.cpp`. Change tags and regenerate.

Hand-written `lua-meta/*.lua` in a module (installed via `install_metas`) can be richer than generated meta. Generated files go to the **project** `build/lua-meta/`. Prefer `@lparam`/`@lreturn` so generated meta stays usable; keep hand-written meta only when the Lua API is not a straight C++ wrap.

## Model checklist

When adding or changing a Lua-visible C++ API:

1. Put the declaration in a header covered by `bind_headers` / `bind_header`.
2. Place `///` or `/** */` **immediately above** that declaration.
3. Add `@luabind` (or rely on ancestor `all=true`).
4. Set `name=` if the C++ identifier is not the Lua name (`lnew` → `new`, `async_close` → `close`, `less` → `__lt`).
5. Set `async=true` iff the return type is `llae::result_promise_ptr<T>`.
6. Set `hidden=true` on bases that must not appear as `require` fields.
7. Set `raw=true` on value-type constructors; never on `meta::object` factories.
8. Add `@lparam` / `@lreturn` whenever the Lua types are not in the inference table, or a C++ parameter (`loop&`, `lua::state&` already skipped, `lua::multiret`) would leak into meta.
9. Do not tag private methods, destructors, or overloads that are not the Lua entry point.
10. One constructor per class; extra Lua constructors are static methods with `name=new`.
11. One Lua module per header; use `module=` if namespaces would split.
12. After edits, tell the user to run `llae init` (do not run it unless they asked).

## Complete examples

### Module class (`meta::object`)

```cpp
namespace crypto {

/// @luabind
using bignum_ptr = common::intrusive_ptr<bignum>;

/// Arbitrary-precision integer.
/// @luabind
class bignum : public meta::object {
    META_OBJECT
public:
    /// @lparam(other,crypto.bignum)
    /// @luabind(alias=__add)
    bignum_ptr add(lua::state& l);

    /// @lreturn(result,crypto.bignum)
    /// @luabind(name=new)
    static lua::multiret lnew(lua::state& l);
};

}
```

Lua: `require 'crypto'`, `crypto.bignum.new()`, `a:add(b)` and `a + b`.

### Async method

```cpp
/// @luabind
class file : public meta::object {
    META_OBJECT
public:
    /// @luabind(async=true,name=close)
    llae::result_promise_ptr<void> async_close(llae::loop& l);
};
```

Lua: `file:close()` (yields), `file:async_close()` (promise). Add `@lparam`/`@lreturn` if meta must not show `l`.

### Enum + fields + policies

```cpp
/// @luabind
struct item {
    /// @luabind
    enum class state { off, on };
    /// @luabind
    int x = 0;
    /// @luabind(readonly=true)
    int id = 0;
    /// @luabind(policy=field_ref_policy{})
    item child;
    /// @luabind(raw=true)
    item();
};
```

### Namespace `all=true`

```cpp
/// @luabind(all=true)
namespace tools {
    class Foo {
        void bar();
        int value = 0;
        enum class Kind { one = 1, two };
    };
    static void ping(int);
    constexpr int answer = 42;
}
```

Every nested declaration is bound. Lua module name: `tools`.

## Errors and limits

- Multiple `@luabind` constructors on one class → generation error.
- Declaration in the global namespace without `module=` → error (`not found module`).
- Two different module names from one header → logged as error; avoid this.
- Full C++ is not parsed: skip function bodies’ inner types the parser cannot see; tag the declarations, not the definitions.
- `all=true` binds **everything** nested, including members you may not want in Lua. Prefer explicit `@luabind` on public API unless the unit is a dedicated bind stub.

## Implementation map

| Piece | Path |
|---|---|
| Tag parser | `tools/cparse/tags.lua` |
| Binder / type maps | `tools/cparse/process_bind.lua` |
| Project hook | `tools/project.lua` (`generate_bindings`) |
| C++ template | `data/binding-module-template.cpp` |
| Meta template | `data/binding-meta-template.lua` |
| Policies | `src/lua/policy.h` |
| Function/field registration | `src/lua/bind.h` |
| Async registration | `src/llae/async_bind.h` |
| Header fixtures | `src/tests/autobind_tests.h`, `src/tests/extern_autobind.h` |
