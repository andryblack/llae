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
$ make -C build/bootstrap
$ LUA_PATH="tools/?.lua;scripts/?.lua" ./bin/llae-bootstrap bootstrap
```



## Examples

```bash
$ llae run examples/info.lua 
```

## Development

### Build inplace
```bash
LUA_PATH="tools/?.lua;scripts/?.lua" ./bin/llae-bootstrap --root=. --inplace=true install --development
LUA_PATH="tools/?.lua;scripts/?.lua" ./bin/llae-bootstrap --root=. --inplace=true init --development
premake5 --file=build/premake5.lua gmake2
premake5 --file=build/premake5.lua ecc
# build
make -C build -j
# run tests
./bin/llae run tests --verbose
```

### Build docker image with llae
```bash
docker build -f docker/llae.dockerfile -t llae:latest .
```

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE).
