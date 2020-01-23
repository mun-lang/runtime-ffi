#ifndef MUN_RUNTIME_CPP_BINDINGS_H_
#define MUN_RUNTIME_CPP_BINDINGS_H_

#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string_view>
#include <variant>

#include "mun/invoke_result.h"
#include "mun/md5.h"
#include "mun/reflection.h"
#include "mun/runtime_capi.h"

namespace mun {
    /** A wrapper around a `MunErrorHandle`.
     *
     * Frees the corresponding error object on destruction, if it exists.
     */
    class Error {
    public:
        /** Default constructs an error. */
        Error() noexcept
            : m_handle{ 0 } {}

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

        /** Move assigns an error.
         *
         * \param other an rvalue reference to an error
         */
        Error& operator=(Error&& other) noexcept{
            m_handle = other.m_handle;
            other.m_handle._0 = 0;
            return *this;
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
        friend std::optional<Runtime> make_runtime(std::string_view library_path, Error* out_error) noexcept;
        
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

        /** Retrieves `MunFunctionInfo` from the runtime for the corresponding `fn_name`.
         *
         * \param fn_name the name of the desired function
         * \param out_error a pointer that will optionally return an error
         * \return a possible `MunFunctionInfo` struct
         */
        std::optional<MunFunctionInfo> get_function_info(std::string_view fn_name, Error* out_error = nullptr) noexcept {
            bool has_fn;
            MunFunctionInfo temp;
            if (auto error = Error(mun_runtime_get_function_info(m_handle, fn_name.data(), &has_fn, &temp))) {
                if (out_error) {
                    *out_error = std::move(error);
                }
                return std::nullopt;
            }

            return has_fn ? std::make_optional(std::move(temp)) : std::nullopt;
        }

        /** Checks for updates to hot reloadable assemblies.
         *
         * \param out_error a pointer that will optionally return an error
         * \return whether the runtime was updated
         */
        bool update(Error* out_error = nullptr) {
            bool updated;
            if (auto error = Error(mun_runtime_update(m_handle, &updated))) {
                if (out_error) {
                    *out_error = std::move(error);
                }
                return false;
            }
            return updated;
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
    std::optional<Runtime> make_runtime(std::string_view library_path, Error* out_error = nullptr) noexcept {
        MunRuntimeHandle handle;
        if (auto error = Error(mun_runtime_create(library_path.data(), &handle))) {
            if (out_error) {
                *out_error = std::move(error);
            }
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
        auto make_error = [](Runtime& runtime, std::string_view fn_name, Args... args) {
            return InvokeResult<Output, Args...>(
                [&runtime, fn_name](Args... fn_args) { return invoke_fn<Output, Args...>(runtime, fn_name, fn_args...); },
                [&runtime]() { return runtime.update(); },
                std::move(args)...
            );
        };

        Error error;
        constexpr auto NUM_ARGS = sizeof...(Args);
        if (auto fn_info = runtime.get_function_info(fn_name, &error); !fn_info) {
            std::cerr << "Failed to retrieve function info due to error: " << error.message() << std::endl;
        } else if (!fn_info) {
            std::cerr << "Failed to obtain function '" << fn_name << "'" << std::endl;
        } else {
            const auto& signature = fn_info->signature;
            if (signature.num_arg_types != NUM_ARGS) {
                std::cerr << "Invalid number of arguments. Expected: " << std::to_string(signature.num_arg_types) <<
                    ". Found: " << std::to_string(NUM_ARGS) << "." << std::endl;

                return make_error(runtime, fn_name, args...);
            }

            if constexpr (NUM_ARGS > 0) {
                const MunTypeInfo* const* arg_ptr = signature.arg_types;
                const std::optional<std::pair<const char*, const char*>> return_type_diffs[] = {
                    reflection::equals_argument_type(**(arg_ptr++), args)...
                };

                for (size_t idx = 0; idx < NUM_ARGS; ++idx) {
                    if (auto diff = return_type_diffs[idx]) {
                        const auto& [expected, found] = *diff;
                        std::cerr << "Invalid argument type at index " << idx <<
                            ". Expected: " << expected <<
                            ". Found: " << found << "." << std::endl;

                        return make_error(runtime, fn_name, args...);
                    }
                }
            }

            if (signature.return_type) {
                const auto& return_type = signature.return_type;
                if (auto diff = reflection::equals_return_type<Output>(*return_type)) {
                    const auto& [expected, found] = *diff;
                    std::cerr << "Invalid return type. Expected: " << expected <<
                        ". Found: " << found << "." << std::endl;

                    return make_error(runtime, fn_name, args...);
                }
            } else if (!reflection::equal_types<void, Output>()) {
                std::cerr << "Invalid return type. Expected: " << ReturnTypeReflection<void>::type_name() <<
                    ". Found: " << ReturnTypeReflection<Output>::type_name() << "." << std::endl;

                return make_error(runtime, fn_name, args...);;
            }

            auto fn = reinterpret_cast<typename Marshal<Output>::type(*)(typename Marshal<Args>::type...)>(fn_info->fn_ptr);
            if constexpr (std::is_same_v<Output, void>) {
                fn(args...);
                return InvokeResult<Output, Args...>(std::monostate {});
            } else {
                return InvokeResult<Output, Args...>(Marshal<Output>::from(fn(Marshal<Args>::to(args)...), *signature.return_type));
            }
        }

        return make_error(runtime, fn_name, args...);
    }
}

#endif /* MUN_RUNTIME_CPP_BINDINGS_H_ */
