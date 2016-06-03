#include "pub/mem.h"
#include "instr.h"
#include "str.h"
#include "exec.h"
#include "err.h"

#define INSTR_TYPE_N_ARG_INIT(instr, exec) \
	(instr).arg = 0

#define INSTR_TYPE_I_ARG_INIT(instr, exec) \
	(instr).arg = arg

#define INSTR_TYPE_S_ARG_INIT(instr, exec) \
	(instr).arg = ivm_exec_registerString((exec), str)


#define OP_GEN(o, name, arg, ...) \
	ivm_instr_t ivm_instr_gen_##o(IVM_INSTR_TYPE_##arg##_ARG \
								   ivm_exec_t *exec) \
	{ \
		ivm_instr_t ret; \
		ret.proc = ivm_op_table_getProc(IVM_OP(o)); \
		INSTR_TYPE_##arg##_ARG_INIT((ret), (exec)); \
		ret.op = IVM_OP(o); \
		return ret; \
	}

	#include "op.def"

#undef OP_GEN