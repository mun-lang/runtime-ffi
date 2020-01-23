#ifndef MUN_STRUCT_H_
#define MUN_STRUCT_H_

#include <cassert>
#include <optional>

#include "mun/marshal.h"
#include "mun/reflection.h"
#include "mun/runtime_capi.h"

namespace mun {
    namespace details {
        std::optional<size_t> find_index(const MunStructInfo& struct_info, std::string_view field_name) noexcept {
            const auto begin = struct_info.field_names;
            const auto end = struct_info.field_names + struct_info.num_fields;

            const auto it = std::find(begin, end, field_name);
            if (it == end) {
                std::cerr << "Struct `" << struct_info.name << "` does not contain field `" << field_name << "`." << std::endl;
                return std::nullopt;
            }

            return std::make_optional(std::distance(begin, it));
        }

        std::string format_struct_field(std::string_view struct_name, std::string_view field_name) noexcept {
            std::string formatted;
            formatted.reserve(struct_name.size() + 2 + field_name.size());

            return formatted.append(struct_name).append("::").append(field_name);
        }
    }

    class Struct {
    public:
        Struct(MunStructInfo struct_info, std::byte* raw) noexcept
            : m_info(struct_info)
            , m_raw(raw)
        {}

        Struct(const Struct&) noexcept = default;
        Struct(Struct&&) noexcept = default;

        Struct& operator=(Struct&&) noexcept = default;

        template <typename T>
        std::optional<T> get(std::string_view field_name) const noexcept {
            if (const auto idx = details::find_index(m_info, field_name)) {
                const auto* field_type = m_info.field_types[*idx];
                if (auto diff = reflection::equals_return_type<T>(*field_type)) {
                    const auto& [expected, found] = *diff;

                    std::cerr << "Mismatched types for `" << details::format_struct_field(m_info.name, field_name) <<
                        "`. Expected: `" << expected << "`. Found: `" << found << "`." << std::endl;

                    return std::nullopt;
                }

                const auto offset = m_info.field_offsets[*idx];
                return std::make_optional(Marshal<T>::from(*reinterpret_cast<Marshal<T>::type*>(m_raw + offset), *field_type));
            } else {
                return std::nullopt;
            }
        }

        template <typename T>
        std::optional<T> replace(std::string_view field_name, T value) const noexcept {
            if (const auto idx = details::find_index(m_info, field_name)) {
                const auto* field_type = m_info.field_types[*idx];
                if (auto diff = reflection::equals_return_type<T>(*field_type)) {
                    const auto& [expected, found] = *diff;

                    std::cerr << "Mismatched types for `" << details::format_struct_field(m_info.name, field_name) <<
                        "`. Expected: `" << expected << "`. Found: `" << found << "`." << std::endl;

                    return std::nullopt;
                }

                const auto offset = m_info.field_offsets[*idx];
                Marshal<T>::type marshalled = Marshal<T>::to(value);
                std::swap(
                    *reinterpret_cast<Marshal<T>::type*>(m_raw + offset),
                    marshalled
                );

                return std::make_optional(Marshal<T>::from(marshalled, *field_type));
            } else {
                return std::nullopt;
            }
        }

        template <typename T>
        bool set(std::string_view field_name, T value) const noexcept {
            if (const auto idx = details::find_index(m_info, field_name)) {
                const auto* field_type = m_info.field_types[*idx];
                if (auto diff = reflection::equals_return_type<T>(*field_type)) {
                    const auto& [expected, found] = *diff;

                    std::cerr << "Mismatched types for `" << details::format_struct_field(m_info.name, field_name) <<
                        "`. Expected: `" << expected << "`. Found: `" << found << "`." << std::endl;

                    return false;
                }

                const auto offset = m_info.field_offsets[*idx];
                *reinterpret_cast<Marshal<T>::type*>(m_raw + offset) = Marshal<T>::to(value);
                return true;
            } else {
                return false;
            }
        }

        const MunStructInfo& info() const noexcept {
            return m_info;
        }

        std::byte* raw() const noexcept {
            return m_raw;
        }

    private:
        MunStructInfo m_info;
        std::byte* m_raw;
    };

    template <>
    struct Marshal<Struct> {
        using type = std::byte*;

        static Struct from(type ptr, const MunTypeInfo& type_info) noexcept {
            assert(type_info.group == MunTypeGroup::StructTypes);

            // As type_info is guaranteed to be a struct, this always succeeds.
            MunStructInfo struct_info;
            mun_type_info_as_struct(&type_info, &struct_info);

            return Struct(struct_info, ptr);
        }

        static type to(Struct value) noexcept {
            return value.raw();
        }
    };

    template <>
    struct ArgumentReflection<Struct> {
        static const char* type_name(const Struct& s) noexcept {
            return s.info().name;
        }
    };

    template <>
    struct ReturnTypeReflection<Struct> {
        static constexpr const char* type_name() noexcept {
            return "struct";
        }
    };
}

#endif
