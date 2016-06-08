#include "pub/mem.h"
#include "pub/com.h"
#include "pub/err.h"
#include "pub/vm.h"

#include "inline/obj.h"
#include "gc/gc.h"
#include "func.h"
#include "context.h"
#include "runtime.h"
#include "call.h"
#include "coro.h"

#define CLOSURE_CONTEXT(origin, state)  \
	((origin) \
	 ? ivm_ctchain_clone((context), (state)) \
	 : IVM_NULL)

IVM_PRIVATE
void
ivm_function_init(ivm_vmstate_t *state,
				  ivm_function_t *func,
				  ivm_exec_t *body,
				  ivm_signal_mask_t intsig)
{
	func->is_native = IVM_FALSE;
	func->u.f.body = body;
	func->intsig = intsig;

	return;
}

IVM_PRIVATE
void
ivm_function_initNative(ivm_vmstate_t *state,
					    ivm_function_t *func,
						ivm_native_function_t native,
						ivm_signal_mask_t intsig)
{
	func->is_native = IVM_TRUE;
	func->u.native = native;
	func->intsig = intsig;

	return;
}

ivm_function_t *
ivm_function_new(ivm_vmstate_t *state,
				 ivm_exec_t *body,
				 ivm_signal_mask_t intsig)
{
	ivm_function_t *ret = ivm_vmstate_allocFunc(state);

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("function"));

	ivm_function_init(state, ret, body, intsig);

	return ret;
}

ivm_function_t *
ivm_function_newNative(ivm_vmstate_t *state,
					   ivm_native_function_t func,
					   ivm_signal_mask_t intsig)
{
	ivm_function_t *ret = ivm_vmstate_allocFunc(state);

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("native function"));

	ivm_function_initNative(state, ret, func, intsig);

	return ret;
}

void
ivm_function_free(ivm_function_t *func,
				  ivm_vmstate_t *state)
{
	if (func) {
		ivm_vmstate_dumpFunc(state, func);
	}

	return;
}

ivm_function_t *
ivm_function_clone(ivm_function_t *func,
				   ivm_vmstate_t *state)
{
	ivm_function_t *ret = IVM_NULL;

	if (func) {
		ret = ivm_vmstate_allocFunc(state);
		MEM_COPY(ret, func, sizeof(*ret));
	}

	return ret;
}

IVM_PRIVATE
void
ivm_function_invoke_c(const ivm_function_t *func,
					  ivm_vmstate_t *state,
					  ivm_ctchain_t *context,
					  ivm_runtime_t *runtime)
{
	if (func->is_native) {
		context = ivm_ctchain_clone(context, state);
		ivm_runtime_invoke(runtime, state, IVM_NULL, context);
	} else {
		context = ivm_ctchain_appendContext(context, state,
											ivm_context_new(state));
		ivm_runtime_invoke(runtime, state, func->u.f.body, context);
	}

	return;
}

ivm_runtime_t *
ivm_function_createRuntime(const ivm_function_t *func,
						   ivm_vmstate_t *state,
						   ivm_ctchain_t *context)
{
	ivm_runtime_t *ret = ivm_runtime_new(state);

	ivm_function_invoke_c(func, state, context, ret);

	return ret;
}

void
ivm_function_invoke(const ivm_function_t *func,
					ivm_vmstate_t *state,
					ivm_ctchain_t *context,
					ivm_coro_t *coro)
{
	ivm_runtime_t *runtime = IVM_CORO_GET(coro, RUNTIME);
	ivm_frame_t *cur = ivm_runtime_getFrame(runtime, state, coro);

	ivm_function_invoke_c(func, state, context, runtime);
	ivm_frame_stack_push(IVM_CORO_GET(coro, FRAME_STACK), cur);

	return;
}

void
ivm_function_object_destructor(ivm_object_t *obj,
							   ivm_vmstate_t *state)
{
	ivm_function_object_t *func = IVM_AS(obj, ivm_function_object_t);
	
	ivm_ctchain_free(func->closure, state);
	ivm_function_free(func->val, state);

	return;
}

ivm_object_t *
ivm_function_object_new(ivm_vmstate_t *state,
						ivm_ctchain_t *context,
						ivm_function_t *func)
{
	ivm_function_object_t *ret = ivm_vmstate_alloc(state, sizeof(*ret));

	ivm_object_init(IVM_AS_OBJ(ret), state, IVM_FUNCTION_OBJECT_T);

	ret->closure = ivm_ctchain_clone(context, state);
	ret->val = func;
	ivm_vmstate_addDesLog(state, IVM_AS_OBJ(ret)); /* function objects need destruction */

	return IVM_AS_OBJ(ret);
}

void
ivm_function_object_traverser(ivm_object_t *obj,
							  ivm_traverser_arg_t *arg)
{
	arg->trav_ctchain(IVM_AS(obj, ivm_function_object_t)
					  ->closure,
					  arg);

	return;
}
