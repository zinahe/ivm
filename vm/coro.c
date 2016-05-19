#include "stdlib.h"
#include "pub/mem.h"
#include "coro.h"
#include "vmstack.h"
#include "call.h"
#include "vm.h"
#include "context.h"
#include "op.h"
#include "err.h"

ivm_coro_t *
ivm_coro_new()
{
	ivm_coro_t *ret = MEM_ALLOC(sizeof(*ret),
								ivm_coro_t *);

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("coroutine"));

	ret->stack = ivm_vmstack_new();
	ret->frame_st = ivm_frame_stack_new();
	ret->runtime = IVM_NULL;

	return ret;
}

void
ivm_coro_free(ivm_coro_t *coro,
			  ivm_vmstate_t *state)
{
	if (coro) {
		ivm_frame_stack_foreach(coro->frame_st, (ivm_stack_foreach_proc_t)ivm_frame_free);
		ivm_vmstack_free(coro->stack);
		ivm_frame_stack_free(coro->frame_st);
		ivm_runtime_free(coro->runtime, state);
		MEM_FREE(coro);
	}

	return;
}

#define ivm_coro_kill(coro, state) \
	ivm_runtime_free((coro)->runtime, (state)); \
	(coro)->runtime = IVM_NULL;

ivm_object_t *
ivm_coro_start(ivm_coro_t *coro, ivm_vmstate_t *state, ivm_function_t *root)
{
	ivm_object_t *ret = IVM_NULL;
	ivm_op_proc_t tmp_proc;
	ivm_frame_t *tmp_frame;
	ivm_runtime_t *tmp_runtime;
	ivm_vmstack_t *tmp_stack;

	if (root) {
		/* root of sleeping coro cannot be reset */
		IVM_ASSERT(!coro->runtime, IVM_ERROT_MSG_RESET_CORO_ROOT);
		coro->runtime = ivm_function_createRuntime(root, state);
	}

	if (ivm_function_isNative(root)) {
		ret = ivm_function_callNative(root, state,
									  IVM_RUNTIME_GET(coro->runtime, CONTEXT),
									  IVM_FUNCTION_SET_ARG_2(0, IVM_NULL));
		ivm_coro_kill(coro, state);
	} else if (coro->runtime) {
		tmp_runtime = coro->runtime;
		tmp_stack = coro->stack;

		while (1) {
			ret = IVM_NULL;

			while (IVM_RUNTIME_GET(tmp_runtime, EXEC) &&
				   IVM_RUNTIME_GET(tmp_runtime, PC)
				   < ivm_exec_length(IVM_RUNTIME_GET(tmp_runtime, EXEC))) {
				tmp_proc = ivm_op_table_getProc(ivm_exec_opAt(IVM_RUNTIME_GET(tmp_runtime, EXEC),
															  IVM_RUNTIME_GET(tmp_runtime, PC)));
				switch (tmp_proc(state, coro)) {
					case IVM_ACTION_BREAK:
						goto ACTION_BREAK;
					case IVM_ACTION_YIELD:
						goto ACTION_YIELD;
					default:;
				}
				ivm_vmstate_checkGC(state);
			}
ACTION_BREAK:
			
			tmp_frame = ivm_frame_stack_pop(coro->frame_st);

			if (tmp_frame) {
				if ((ivm_vmstack_size(tmp_stack)
					 - IVM_FRAME_GET(tmp_frame, STACK_TOP)) > 0) {
					ret = ivm_vmstack_pop(tmp_stack);
				}
		
				ivm_runtime_restore(tmp_runtime, state, coro, tmp_frame);
				ivm_frame_free(tmp_frame, state);

				if (IVM_RUNTIME_GET(tmp_runtime, IS_NATIVE)) {
					goto END;
				}

				ivm_vmstack_push(tmp_stack,
								 ret ? ret
								 	 : IVM_NULL_OBJ(state));
			} else {
				/* no more callee to restore, end coro */
				ivm_coro_kill(coro, state);
				break;
			}
		}

ACTION_YIELD:
		ret = ivm_vmstack_pop(tmp_stack);
	}

END:

	return ret ? ret : IVM_NULL_OBJ(state);
}
