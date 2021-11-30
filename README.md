[![Build Status](https://ci.sandboxgames.ru/api/badges/andry/llae/status.svg?ref=refs/heads/develop)](https://ci.sandboxgames.ru/andry/llae)

## LLAE

Lua applicication engine.

libuv based async application engine.

build-in modules:

* json parse / encode (yajl)

* http client/server, https client (openssl)

## Build and install

```bash
$ premake5 download
$ premake5 unpack
$ premake5 gmake2
$ make -C build
$ LUA_PATH="tools/?.lua;scripts/?.lua" ./bin/llae-bootstrap bootstrap
```

## Build inplace
```bash
LUA_PATH="tools/?.lua;scripts/?.lua" ./bin/llae-bootstrap --root=. --inplace=true install
LUA_PATH="tools/?.lua;scripts/?.lua" ./bin/llae-bootstrap --root=. --inplace=true init 
premake5 --file=build/premake5.lua gmake2
```

## Examples

```bash
$ llae run examples/info.lua 
```

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE).
