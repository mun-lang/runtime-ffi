#ifndef MUN_RUNTIME_CPP_BINDINGS_H_
#define MUN_RUNTIME_CPP_BINDINGS_H_

#include <cassert>
#include <optional>
#include <string_view>

#include "mun/error.h"
#include "mun/runtime_capi.h"

namespace mun {
/** A wrapper around a `MunRuntimeHandle`.
 *
 * Frees the corresponding runtime object on destruction, if it exists.
 */
class Runtime {
    friend std::optional<Runtime> make_runtime(std::string_view library_path,
                                               Error* out_error) noexcept;

    /** Constructs a runtime from an instantiated `MunRuntimeHandle`.
     *
     * \param handle a runtime handle
     */
    Runtime(MunRuntimeHandle handle) noexcept : m_handle(handle) {}

   public:
    /** Move constructs a runtime
     *
     * \param other an rvalue reference to a runtime
     */
    Runtime(Runtime&& other) noexcept : m_handle(other.m_handle) { other.m_handle._0 = nullptr; }

    /** Destructs a runtime */
    ~Runtime() noexcept { mun_runtime_destroy(m_handle); }

    /** Retrieves `MunFunctionInfo` from the runtime for the corresponding
     * `fn_name`.
     *
     * \param fn_name the name of the desired function
     * \param out_error a pointer that will optionally return an error
     * \return possibly, the desired `MunFunctionInfo` struct
     */
    std::optional<MunFunctionInfo> find_function_info(std::string_view fn_name,
                                                      Error* out_error = nullptr) noexcept {
        bool has_fn;
        MunFunctionInfo temp;
        if (auto error =
                Error(mun_runtime_get_function_info(m_handle, fn_name.data(), &has_fn, &temp))) {
            if (out_error) {
                *out_error = std::move(error);
            }
            return std::nullopt;
        }

        return has_fn ? std::make_optional(std::move(temp)) : std::nullopt;
    }

    /**
     * Allocates an object in the runtime of the given `type_info`. If
     * successful, `obj` is returned, otherwise the nothing is returned and the
     * `out_error` is set - if it is not null.
     *
     * \param type_info the type to allocate
     * \param out_error a pointer to fill with a potential error
     * \return potentially, the handle of an allocated object
     */
    std::optional<MunGcPtr> gc_alloc(MunUnsafeTypeInfo type_info, Error* out_error = nullptr) const
        noexcept {
        MunGcPtr obj;
        if (auto error = Error(mun_gc_alloc(m_handle, type_info, &obj))) {
            if (out_error) {
                *out_error = std::move(error);
            }
            return std::nullopt;
        }

        return std::make_optional(obj);
    }

    /** Collects all memory that is no longer referenced by rooted objects.
     *
     * Returns `true` if memory was reclaimed, `false` otherwise. This behavior
     * will likely change in the future.
     */
    bool gc_collect() const noexcept {
        bool reclaimed;
        assert(mun_gc_collect(m_handle, &reclaimed)._0 == 0);
        return reclaimed;
    }

    /**
     * Roots the specified `obj`, which keeps it and objects it references
     * alive.
     *
     * Objects marked as root, must call `mun_gc_unroot` before they can
     * be collected. An object can be rooted multiple times, but you must make
     * sure to call `mun_gc_unroot` an equal number of times before the object
     * can be collected. If successful, `obj` has been rooted, otherwise a
     * non-zero error handle is returned.
     *
     * \param obj a garbage collection handle
     */
    void gc_root_ptr(MunGcPtr obj) const noexcept { assert(mun_gc_root(m_handle, obj)._0 == 0); }

    /**
     * Unroots the specified `obj`, potentially allowing it and objects it
     * references to be collected.
     *
     * An object can be rooted multiple times, so you must make sure to call
     * `gc_unroot_ptr` the same number of times as `gc_root_ptr` was called
     * before the object can be collected.
     *
     * \param obj a garbage collection handle
     */
    void gc_unroot_ptr(MunGcPtr obj) const noexcept {
        assert(mun_gc_unroot(m_handle, obj)._0 == 0);
    }

    /**
     * Retrieves the type information for the specified `obj`.
     *
     * \param obj a garbage collection handle
     * \return the handle's type information
     */
    MunUnsafeTypeInfo ptr_type(MunGcPtr obj) const noexcept {
        MunUnsafeTypeInfo type_info;
        assert(mun_gc_ptr_type(m_handle, obj, &type_info)._0 == 0);
        return type_info;
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

/** Construct a new runtime that loads the library at `library_path` and its
dependencies.
 *
 * On failure, the error is returned through the `out_error` pointer, if set.
 *
 * \param library_path the path to a Mun library
 * \param out_error optionally, a pointer to an `Error` instance
 * \return potentially, a runtime
.*/
inline std::optional<Runtime> make_runtime(std::string_view library_path,
                                           Error* out_error = nullptr) noexcept {
    MunRuntimeHandle handle;
    if (auto error = Error(mun_runtime_create(library_path.data(), &handle))) {
        if (out_error) {
            *out_error = std::move(error);
        }
        return std::nullopt;
    }

    return Runtime(handle);
}
}  // namespace mun

#endif /* MUN_RUNTIME_CPP_BINDINGS_H_ */
