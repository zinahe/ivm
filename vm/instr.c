#include "pub/const.h"
#include "pub/err.h"

#include "std/mem.h"
#include "std/string.h"

#include "opcode.h"
#include "instr.h"
#include "exec.h"

#define INSTR_TYPE_N_ARG_INIT(instr, exec) \
	ivm_opcode_arg_fromInt(0)

#define INSTR_TYPE_I_ARG_INIT(instr, exec) \
	ivm_opcode_arg_fromInt(arg)

#define INSTR_TYPE_A_ARG_INIT(instr, exec) \
	ivm_opcode_arg_fromInt(arg)

#define INSTR_TYPE_X_ARG_INIT(instr, exec) \
	ivm_opcode_arg_fromInt(arg)

#define INSTR_TYPE_F_ARG_INIT(instr, exec) \
	ivm_opcode_arg_fromFloat(arg)

#define INSTR_TYPE_S_ARG_INIT(instr, exec) \
	ivm_opcode_arg_fromPointer(len == -1 ? ivm_exec_registerString((exec), str) : ivm_exec_registerString_n((exec), str, len))

#define OPCODE_GEN(o, name, param, st_inc, ...) \
	ivm_instr_t ivm_instr_gen_##o(IVM_INSTR_TYPE_##param##_ARG    \
								  ivm_exec_t *exec,               \
								  ivm_size_t len,                 \
								  ivm_uint_t line)                \
	{                                                             \
		return (ivm_instr_t) {                                    \
			.entry = ivm_opcode_table_getEntry(IVM_OPCODE(o)),    \
			.arg = INSTR_TYPE_##param##_ARG_INIT((ret), (exec)),  \
			.lineno = line,                                       \
			.opc = IVM_OPCODE(o)                                  \
		};                                                        \
	}

	#include "opcode.def.h"

#undef OPCODE_GEN
