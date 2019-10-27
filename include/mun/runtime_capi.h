#ifndef MUN_RUNTIME_BINDINGS_H_
#define MUN_RUNTIME_BINDINGS_H_

/* Generated with cbindgen:0.9.1 */

#include <stdbool.h>
#include <stdint.h>

/**
 * A type that represents the privacy level of modules, functions, or variables.
 */
enum MunPrivacy
#ifdef __cplusplus
  : uint8_t
#endif // __cplusplus
 {
    Public = 0,
    Private = 1,
};
#ifndef __cplusplus
typedef uint8_t MunPrivacy;
#endif // __cplusplus

typedef uintptr_t MunToken;

typedef struct {
    MunToken _0;
} MunErrorHandle;

typedef struct {
    void *_0;
} MunRuntimeHandle;

/**
 * <div rustbindgen derive="PartialEq">
 */
typedef struct {
    uint8_t b[16];
} MunGuid;

typedef struct {
    MunGuid guid;
    const char *name;
} MunTypeInfo;

/**
 * <div rustbindgen derive="Clone">
 */
typedef struct {
    const char *name;
    const MunTypeInfo *arg_types;
    const MunTypeInfo *return_type;
    uint16_t num_arg_types;
    MunPrivacy privacy;
} MunFunctionSignature;

/**
 * <div rustbindgen derive="Clone">
 */
typedef struct {
    MunFunctionSignature signature;
    const void *fn_ptr;
} MunFunctionInfo;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void mun_error_destroy(MunErrorHandle error_handle);

const char *mun_error_message(MunErrorHandle error_handle);

MunErrorHandle mun_runtime_create(const char *library_path, MunRuntimeHandle *handle);

void mun_runtime_destroy(MunRuntimeHandle handle);

MunErrorHandle mun_runtime_get_function_info(MunRuntimeHandle handle,
                                             const char *fn_name,
                                             bool *has_fn_info,
                                             MunFunctionInfo *fn_info);

MunErrorHandle mun_runtime_update(MunRuntimeHandle handle, bool *updated);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* MUN_RUNTIME_BINDINGS_H_ */
