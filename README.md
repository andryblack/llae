[![Build Status](https://ci.sandboxgames.ru/api/badges/andry/llae1/status.svg?ref=refs/heads/develop)](https://ci.sandboxgames.ru/andry/llae1)

## LLAE

Lua applicication engine.

libuv based async application engine.

build-in modules:

* json parse / encode (yajl)

* http client/server, https client (openssl)

## Build and install

```bash
$ premake5 download
$ premake5 gmake2
$ make -C build
$ LUA_PATH="tools/?.lua;scripts/?.lua" ./bin/llae-bootstrap bootstrap
```

## Examples

```bash
$ llae run examples/info.lua 
```

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE).
