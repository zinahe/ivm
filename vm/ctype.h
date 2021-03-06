#ifndef _IVM_VM_CTYPE_H_
#define _IVM_VM_CTYPE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* #include <wchar.h> */
#include <float.h>

#include "pub/com.h"

typedef intptr_t				ivm_ptr_t;
typedef uintptr_t				ivm_uptr_t;
typedef ptrdiff_t				ivm_ptrdiff_t;

typedef int8_t					ivm_sint8_t;
typedef int16_t					ivm_sint16_t;
typedef int32_t					ivm_sint32_t;
typedef int64_t					ivm_sint64_t;

typedef uint8_t					ivm_uint8_t;
typedef uint16_t				ivm_uint16_t;
typedef uint32_t				ivm_uint32_t;
typedef uint64_t				ivm_uint64_t;

typedef bool					ivm_bool_t;
typedef int						ivm_int_t;
typedef unsigned int			ivm_uint_t;
typedef long					ivm_long_t;
typedef unsigned long			ivm_ulong_t;

typedef float					ivm_single_t;
typedef double					ivm_double_t;

/* typedef wchar_t					ivm_wchar_t; */

typedef char					ivm_char_t;
typedef unsigned char			ivm_uchar_t;
typedef signed char				ivm_schar_t;

#define IVM_IS64				(sizeof(ivm_ptr_t) == 8)

#define IVM_NULL				((void *)0)
#define IVM_FALSE				0 // false
#define IVM_TRUE				1 // true

typedef ivm_uptr_t				ivm_size_t;
typedef ivm_uint8_t				ivm_byte_t;

typedef ivm_int_t				ivm_type_tag_t;

#define IVM_NUMBER_MAX			(DBL_MAX + DBL_MAX)

typedef ivm_size_t				ivm_function_id_t;

enum {
	IVM_TYPE_FIRST = -1,
#define TYPE_GEN(tag, name, size, cons, proto_init, slots_init, ...) tag,
	#include "type.def.h"
#undef TYPE_GEN
	IVM_TYPE_COUNT
};

typedef union {
	ivm_long_t iarg;
	ivm_function_id_t xarg;
	ivm_double_t farg;
	ivm_ptr_t parg;
	ivm_uint64_t dummy;
} ivm_opcode_arg_t;

#define ivm_opcode_arg_toInt(arg) (arg.iarg)
#define ivm_opcode_arg_toFunc(arg) (arg.xarg)
#define ivm_opcode_arg_toFloat(arg) (arg.farg)
#define ivm_opcode_arg_toPointer(arg) (arg.parg)

#define ivm_opcode_arg_fromInt(i) ((ivm_opcode_arg_t) { .iarg = (ivm_long_t)(i) })
#define ivm_opcode_arg_fromFunc(x) ((ivm_opcode_arg_t) { .xarg = (ivm_function_id_t)(x) })
#define ivm_opcode_arg_fromFloat(f) ((ivm_opcode_arg_t) { .farg = (ivm_double_t)(f) })
#define ivm_opcode_arg_fromPointer(p) ((ivm_opcode_arg_t) { .parg = (ivm_ptr_t)(p) })

typedef ivm_int_t		ivm_argc_t;
typedef ivm_ptr_t		ivm_mark_t;

typedef ivm_int_t		ivm_cgid_t;

#define IVM_MARK_INIT 0

struct ivm_object_t_tag;
struct ivm_vmstate_t_tag;
struct ivm_coro_t_tag;
struct ivm_context_t_tag;

typedef struct {
	struct ivm_object_t_tag *base;
	ivm_argc_t argc;
	struct ivm_object_t_tag **argv;
} ivm_function_arg_t;

/* i starts from 1 */
#define ivm_function_arg_has(arg, i) ((i) <= (arg).argc)
#define ivm_function_arg_at(arg, i) ((arg).argv[(i) - 1])

#define ivm_function_arg_buildEmpty() \
	((ivm_function_arg_t) { IVM_NULL, 0, IVM_NULL })

typedef struct ivm_object_t_tag *
(*ivm_native_function_t)(struct ivm_vmstate_t_tag *,
						 struct ivm_coro_t_tag *,
						 struct ivm_context_t_tag *,
						 ivm_function_arg_t);

// if the double can be converted into long without the loss of precision)
IVM_INLINE
ivm_bool_t
IVM_DOUBLE_ACC(ivm_double_t num)
{
	return (num <= 0x1fffffffffffffl && num >= -0x1fffffffffffffl)
		   && (num - (ivm_long_t)num == 0);
}

// if the double can be converted to long without overflow
IVM_INLINE
ivm_bool_t
IVM_DOUBLE_NOOVERFLOW(ivm_double_t num)
{
	return num <= 0x1fffffffffffffl && num >= -0x1fffffffffffffl;
}

typedef ivm_double_t ivm_number_t;

#define ivm_number_canbeLong(num) IVM_DOUBLE_NOOVERFLOW(num)

#define IVM_GET(obj, type, member) (IVM_NULL, type##_GET_##member(obj))
#define IVM_SET(obj, type, member, val) (type##_SET_##member((obj), (val)))

#endif
