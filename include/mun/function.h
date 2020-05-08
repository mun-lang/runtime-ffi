#ifndef MUN_FUNCTION_H_
#define MUN_FUNCTION_H_

#include <string>
#include <vector>

#include "mun/runtime_capi.h"
#include "mun/type_info.h"

namespace mun {
/**
 * A wrapper around a C function with type information.
 */
struct RuntimeFunction {
    template <typename TRet, typename... TArgs>
    RuntimeFunction(std::string_view name, TRet(__cdecl* fn_ptr)(TArgs...))
        : name(name),
          arg_types({arg_type_info<TArgs>()...}),
          ret_type(return_type_info<TRet>()),
          fn_ptr(reinterpret_cast<const void*>(fn_ptr)) {}

    RuntimeFunction(const RuntimeFunction&) = default;
    RuntimeFunction(RuntimeFunction&&) = default;

    std::string name;
    std::vector<MunTypeInfo const*> arg_types;
    std::optional<MunTypeInfo const*> ret_type;
    const void* fn_ptr;
};
}  // namespace mun

#endif
