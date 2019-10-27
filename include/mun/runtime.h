#ifndef MUN_RUNTIME_CPP_BINDINGS_H_
#define MUN_RUNTIME_CPP_BINDINGS_H_

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string_view>

#include "mun/invoke_result.h"
#include "mun/runtime_capi.h"

namespace mun {
    namespace details {
        template <typename Arg>
        constexpr bool valid_arg(const MunTypeInfo& arg_type) {
            auto output_guid = type_guid<Arg>();
            return std::equal(std::begin(arg_type.guid.b), std::end(arg_type.guid.b), std::begin(output_guid.b), std::end(output_guid.b));
        }

        template <typename Arg1, typename Arg2>
        constexpr bool equal_args() {
            auto arg1_guid = type_guid<Arg1>();
            auto arg2_guid = type_guid<Arg2>();
            return std::equal(std::begin(arg1_guid.b), std::end(arg1_guid.b), std::begin(arg2_guid.b), std::end(arg2_guid.b));
        }

        template <typename Output, typename... Args>
        constexpr bool valid_fn(const MunFunctionSignature& signature) noexcept {
            constexpr auto NUM_ARGS = sizeof...(Args);

            if (signature.num_arg_types != NUM_ARGS) {
                std::cerr << "Invalid number of arguments. Expected: " << std::to_string(NUM_ARGS) << ". Found: " << std::to_string(signature.num_arg_types) << "." << std::endl;
                return false;
            }

            if constexpr (NUM_ARGS > 0) {
                const MunTypeInfo* arg_ptr = signature.arg_types;
                const bool valid_args[] = { valid_arg<Args>(*(arg_ptr++))... };
                for (size_t idx = 0; idx < NUM_ARGS; ++idx) {
                    if (!valid_args[idx]) {
                        const char* arg_names[] = { type_name<Args>()... };
                        std::cerr << "Invalid argument type at index " << idx << ". Expected: " << signature.arg_types[idx].name << ". Found: " << arg_names[idx] << "." << std::endl;
                        return false;
                    }
                }
            }

            if (signature.return_type) {
                if (!valid_arg<Output>(*signature.return_type)) {
                    std::cerr << "Invalid return type. Expected: " << signature.return_type->name << ". Found: " << type_name<Output>() << "." << std::endl;
                    return false;
                }
            } else if (!equal_args<void, Output>()) {
                std::cerr << "Invalid return type. Expected: " << type_name<void>() << ". Found: " << type_name<Output>() << "." << std::endl;
                return false;
            }

            return true;
        }
    }

    // TODO: Automate the generation of GUIDs
    template<typename T>
    constexpr MunGuid type_guid() {
        static_assert(false, "Unknown type")
    }

    template<typename T>
    constexpr const char* type_name() {
        static_assert(false, "Unknown type")
    }

    template<>
    constexpr MunGuid type_guid<bool>() {
        return MunGuid{
            { 3, 250, 172, 250, 126, 222, 75, 18, 246, 18, 212, 156, 254, 63, 16, 230 }
        };
    }

    template<>
    constexpr const char* type_name<bool>() {
        return "@core::bool";
    }

    template<>
    constexpr MunGuid type_guid<double>() {
        return MunGuid{
            { 143, 142, 214, 135, 13, 131, 110, 33, 172, 233, 214, 19, 161, 21, 42, 130 }
        };
    }

    template<>
    constexpr const char* type_name<double>() {
        return "@core::float";
    }

    template<>
    constexpr MunGuid type_guid<int64_t>() {
        return MunGuid{
            { 143, 152, 209, 84, 128, 137, 239, 52, 175, 82, 21, 160, 15, 102, 47, 114 }
        };
    }

    template<>
    constexpr const char* type_name<int64_t>() {
        return "@core::int";
    }

    template<>
    constexpr MunGuid type_guid<void>() {
        return MunGuid{
            { 202, 82, 158, 158, 79, 247, 42, 14, 1, 174, 64, 173, 40, 117, 124, 68 }
        };
    }

    template<>
    constexpr const char* type_name<void>() {
        return "@core::empty";
    }

    /** A wrapper around a `MunErrorHandle`.
     *
     * Frees the corresponding error object on destruction, if it exists.
     */
    class Error {
    public:
        /** Constructs an error from a `MunErrorHandle`.
         *
         * \param handle an error handle
         */
        Error(MunErrorHandle handle) noexcept
            : m_handle(handle) {}

        /** Move constructs an error.
         *
         * \param other an rvalue reference to an error
         */
        Error(Error&& other) noexcept
            : m_handle(other.m_handle) {
            other.m_handle._0 = 0;
        }

        /** Destructs the error. */
        ~Error() noexcept {
            mun_error_destroy(m_handle);
        }

        /** Retrieves the error message, if it exists, otherwise returns a nullptr.
         *
         * The message is UTF-8 encoded.
         */
        const char* message() noexcept {
            return mun_error_message(m_handle);
        }

        /** Retrieves whether an error exists */
        operator bool() const noexcept {
            return m_handle._0 != 0;
        }

    private:
        MunErrorHandle m_handle;
    };

    /** A wrapper around a `MunRuntimeHandle`.
     *
     * Frees the corresponding runtime object on destruction, if it exists.
     */
    class Runtime {
        friend std::optional<Runtime> make_runtime(std::string_view library_path) noexcept;
        
        /** Constructs a runtime from an instantiated `MunRuntimeHandle`.
         *
         * \param handle a runtime handle
         */
        Runtime(MunRuntimeHandle handle) noexcept
            : m_handle(handle)
        {}

    public:
        /** Move constructs a runtime
         *
         * \param other an rvalue reference to a runtime
         */
        Runtime(Runtime&& other) noexcept
            : m_handle(other.m_handle) {
            other.m_handle._0 = nullptr;
        }

        /** Destructs a runtime */
        ~Runtime() noexcept {
            mun_runtime_destroy(m_handle);
        }

        /** Retrieves `FunctionInfo` from the runtime for the corresponding `fn_name`.
         *
         * \param fn_name the name of the desired function
         * \param fn_info an output reference to an optional `MunFunctionInfo` struct
         * \return a possible error
         */
        Error get_function_info(std::string_view fn_name, std::optional<MunFunctionInfo>& fn_info) noexcept {
            bool has_fn;
            MunFunctionInfo temp;
            auto error = Error(mun_runtime_get_function_info(m_handle, fn_name.data(), &has_fn, &temp));
            if (!error) {
                if (has_fn) {
                    fn_info.emplace(std::move(temp));
                } else {
                    fn_info.reset();
                }
            }

            return error;
        }

        /** Checks for updates to hot reloadable assemblies.
         *
         * \param updated an output reference signalling whether the runtime was updated
         * \return a possible error
         */
        Error update(bool& updated) {
            return Error(mun_runtime_update(m_handle, &updated));
        }

    private:
        MunRuntimeHandle m_handle;
    };

    /** Construct a new runtime that loads the library at `library_path` and its dependencies.
     *
     * On failure, the error is logged to the error log.
     *
     * \param library_path the path to a Mun library
     * \return an optional runtime
    .*/
    std::optional<Runtime> make_runtime(std::string_view library_path) noexcept {
        MunRuntimeHandle handle;
        if (auto error = Error(mun_runtime_create(library_path.data(), &handle))) {
            std::cerr << "Failed to construct Mun runtime due to error: " << error.message() << std::endl;
            return std::nullopt;
        }

        return Runtime(handle);
    }

    /** Invokes the runtime function corresponding to `fn_name` with arguments `args`.
     *
     * \param runtime the runtime
     * \param fn_name the name of the desired function
     * \param args zero or more arguments to supply to the function invocation
     * \return an invocation result
     */
    template <typename Output, typename... Args>
    InvokeResult<Output, Args...> invoke_fn(Runtime& runtime, std::string_view fn_name, Args... args) noexcept {
        std::optional<MunFunctionInfo> fn_info;
        if (auto error = runtime.get_function_info(fn_name, fn_info)) {
            std::cerr << "Failed to retrieve function info due to error: " << error.message() << std::endl;
        } else if (!fn_info) {
            std::cerr << "Failed to obtain function '" << fn_name << "'" << std::endl;
        } else if (details::valid_fn<Output, Args...>(fn_info->signature)) {
            auto fn = reinterpret_cast<Output(*)(Args...)>(fn_info->fn_ptr);
            return InvokeResult<Output, Args...>(fn(args...));
        }

        return InvokeResult<Output, Args...>(
            [&runtime, fn_name](Args... fn_args) { return invoke_fn<Output, Args...>(runtime, fn_name, fn_args...); },
            [&runtime]() { bool updated; runtime.update(updated); return updated; },
            std::move(args)...
            );
    }
}

#endif /* MUN_RUNTIME_CPP_BINDINGS_H_ */
