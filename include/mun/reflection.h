#ifndef MUN_REFLECTION_H_
#define MUN_REFLECTION_H_

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>

#include "mun/md5.h"
#include "mun/runtime_capi.h"
#include "mun/struct_ref.h"

namespace mun {
namespace details {
constexpr MunGuid type_guid(const char* type_name) noexcept {
    const auto hash = md5::compute(type_name);
    return MunGuid{
        hash[0], hash[1], hash[2],  hash[3],  hash[4],  hash[5],  hash[6],  hash[7],
        hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15],
    };
}
}  // namespace details

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
    return ReturnTypeReflection<T>::type_guid() == ReturnTypeReflection<U>::type_guid();
}

template <typename Arg>
inline std::optional<std::pair<const char*, const char*>> equals_argument_type(
    const MunTypeInfo& type_info, const Arg& arg) noexcept {
    if (type_info.guid == ArgumentReflection<Arg>::type_guid(arg)) {
        return std::nullopt;
    } else {
        const auto expected_name = ArgumentReflection<Arg>::type_name(arg);
        return std::make_pair(type_info.name, expected_name);
    }
}

template <typename T>
inline std::optional<std::pair<const char*, const char*>> equals_return_type(
    const MunTypeInfo& type_info) noexcept {
    if (type_info.group == MunTypeGroup::FundamentalTypes) {
        if (type_info.guid != ReturnTypeReflection<T>::type_guid()) {
            return std::make_pair(type_info.name, ReturnTypeReflection<T>::type_name());
        }
    } else if (!reflection::equal_types<StructRef, T>()) {
        return std::make_pair(type_info.name, ReturnTypeReflection<T>::type_name());
    }

    return std::nullopt;
}
}  // namespace reflection

template <typename T>
struct ArgumentReflection;

template <typename T>
struct ReturnTypeReflection;

#define IMPL_PRIMITIVE_TYPE_REFLECTION(ty, name_literal)                                          \
    template <>                                                                                   \
    struct ReturnTypeReflection<ty> {                                                             \
        static constexpr const char* type_name() noexcept { return name_literal; }                \
        static constexpr MunGuid type_guid() noexcept { return details::type_guid(type_name()); } \
    };                                                                                            \
                                                                                                  \
    template <>                                                                                   \
    struct ArgumentReflection<ty> {                                                               \
        static constexpr const char* type_name(const ty&) noexcept {                              \
            return ReturnTypeReflection<ty>::type_name();                                         \
        }                                                                                         \
        static constexpr MunGuid type_guid(const ty&) noexcept {                                  \
            return ReturnTypeReflection<ty>::type_guid();                                         \
        }                                                                                         \
    };

IMPL_PRIMITIVE_TYPE_REFLECTION(bool, "core::bool");
IMPL_PRIMITIVE_TYPE_REFLECTION(float, "core::f32");
IMPL_PRIMITIVE_TYPE_REFLECTION(double, "core::f64");
IMPL_PRIMITIVE_TYPE_REFLECTION(int8_t, "core::i8");
IMPL_PRIMITIVE_TYPE_REFLECTION(int16_t, "core::i16");
IMPL_PRIMITIVE_TYPE_REFLECTION(int32_t, "core::i32");
IMPL_PRIMITIVE_TYPE_REFLECTION(int64_t, "core::i64");
// IMPL_PRIMITIVE_TYPE_REFLECTION(int128_t, "core::i128");
IMPL_PRIMITIVE_TYPE_REFLECTION(uint8_t, "core::u8");
IMPL_PRIMITIVE_TYPE_REFLECTION(uint16_t, "core::u16");
IMPL_PRIMITIVE_TYPE_REFLECTION(uint32_t, "core::u32");
IMPL_PRIMITIVE_TYPE_REFLECTION(uint64_t, "core::u64");
// IMPL_PRIMITIVE_TYPE_REFLECTION(uint128_t, "core::u128");

template <>
struct ReturnTypeReflection<void> {
    static constexpr const char* type_name() noexcept { return "core::empty"; }
    static constexpr MunGuid type_guid() noexcept { return details::type_guid(type_name()); }
};
}  // namespace mun

#endif
