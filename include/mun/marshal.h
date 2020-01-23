#ifndef MUN_MARSHAL_H_
#define MUN_MARSHAL_H_

#include "mun/runtime_capi.h"

namespace mun {
    template <typename T>
    struct Marshal;

    template <>
    struct Marshal<int64_t> {
        using type = int64_t;

        static int64_t from(type value, const MunTypeInfo&) noexcept {
            return value;
        }

        static type to(type value) noexcept {
            return value;
        }
    };

    template <>
    struct Marshal<double> {
        using type = double;

        static double from(type value, const MunTypeInfo&) noexcept {
            return value;
        }

        static type to(double value) noexcept {
            return value;
        }
    };

    template <>
    struct Marshal<bool> {
        using type = bool;

        static bool from(type value, const MunTypeInfo&) noexcept {
            return value;
        }

        static type to(bool value) noexcept {
            return value;
        }
    };
}

#endif
