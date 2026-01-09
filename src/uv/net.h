#pragma once
#include "luv.h"
#include "lua/raw_bind.h"

namespace uv {
    struct sockaddr_in_mt  {
        static constexpr const char* name = "struct sockaddr_in";
        using object = struct sockaddr_in;
        static constexpr const auto fields = std::make_tuple(
            lua::field("sin_family",&sockaddr_in::sin_family),
            lua::field("sin_port",&sockaddr_in::sin_port),
            lua::field("sin_addr",&sockaddr_in::sin_addr)
        );
    };

    struct sockaddr_in6_mt  {
        static constexpr const char* name = "struct sockaddr_in6";
        using object = struct sockaddr_in6;
        static constexpr const auto fields = std::make_tuple(
            lua::field("sin_family",&sockaddr_in6::sin6_family),
            lua::field("sin_port",&sockaddr_in6::sin6_port),
            lua::field("sin_addr",&sockaddr_in6::sin6_addr)
        );
    };
}