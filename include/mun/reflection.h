#ifndef MUN_REFLECTION_H_
#define MUN_REFLECTION_H_

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>

#include "mun/runtime_capi.h"

namespace mun {
    constexpr MunGuid type_guid(const char* type_name) noexcept {
        const auto hash = md5::compute(type_name);
        return MunGuid{
            hash[0],  hash[1],  hash[2],  hash[3],
            hash[4],  hash[5],  hash[6],  hash[7],
            hash[8],  hash[9],  hash[10], hash[11],
            hash[12], hash[13], hash[14], hash[15],
        };
    }

    constexpr inline bool operator==(const MunGuid& lhs, const MunGuid& rhs) noexcept {
        for (auto idx = 0; idx < 16; ++idx) {
            if (lhs.b[idx] != rhs.b[idx]) {
                return false;
            }
        }
        return true;
    }

    constexpr inline bool operator!=(const MunGuid& lhs, const MunGuid& rhs) noexcept {
        return !(lhs == rhs);
    }

    namespace reflection {
        template <typename T, typename U>
        constexpr bool equal_types() noexcept {
            return type_guid(ReturnTypeReflection<T>::type_name()) ==
                type_guid(ReturnTypeReflection<U>::type_name());
        }

        template <typename Arg>
        inline std::optional<std::pair<const char*, const char*>> equals_argument_type(const MunTypeInfo& type_info, const Arg& arg) noexcept {
            const auto expected_name = ArgumentReflection<Arg>::type_name(arg);
            if (type_info.guid == type_guid(expected_name)) {
                return std::nullopt;
            } else {
                return std::make_pair(type_info.name, expected_name);
            }
        }

        template <typename T>
        inline std::optional<std::pair<const char*, const char*>> equals_return_type(const MunTypeInfo& type_info) noexcept {
            if (type_info.group == MunTypeGroup::FundamentalTypes) {
                if (type_info.guid != type_guid(ReturnTypeReflection<T>::type_name())) {
                    return std::make_pair(type_info.name, ReturnTypeReflection<T>::type_name());
                }
            } else if (!reflection::equal_types<Struct, T>()) {
                return std::make_pair(
                    type_info.name,
                    ReturnTypeReflection<T>::type_name()
                );
            }

            return std::nullopt;
        }
    }

    template <typename T>
    struct ArgumentReflection;

    template <typename T>
    struct ReturnTypeReflection;

    template <>
    struct ReturnTypeReflection<bool> {
        static constexpr const char* type_name() noexcept {
            return "@core::bool";
        }
    };

    template <>
    struct ReturnTypeReflection<double> {
        static constexpr const char* type_name() noexcept {
            return "@core::float";
        }
    };

    template <>
    struct ReturnTypeReflection<int64_t> {
        static constexpr const char* type_name() noexcept {
            return "@core::int";
        }
    };

    template <>
    struct ReturnTypeReflection<void> {
        static constexpr const char* type_name() noexcept {
            return "@core::empty";
        }
    };

    template <>
    struct ArgumentReflection<bool> {
        static constexpr const char* type_name(const bool&) noexcept {
            return ReturnTypeReflection<bool>::type_name();
        }
    };

    template <>
    struct ArgumentReflection<double> {
        static constexpr const char* type_name(const double&) noexcept {
            return ReturnTypeReflection<double>::type_name();
        }
    };

    template <>
    struct ArgumentReflection<int64_t> {
        static constexpr const char* type_name(const int64_t&) noexcept {
            return ReturnTypeReflection<int64_t>::type_name();
        }
    };
}

#endif
