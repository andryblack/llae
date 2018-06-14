## LLAE

Lua applicication engine.

libuv based async application engine.

build-in modules:

* json parse / encode (yajl)

* http client/server, https client (openssl)

## Build

```bash
$ premake5 gmake
$ make -C build
```

## Examples

```bash
$ ./bin/llae-run examples/http_clien.lua 
$ ./bin/llae-run examples/redis.lua 
```

## License

This library is available to anybody free of charge, under the terms of MIT License (see LICENSE).
