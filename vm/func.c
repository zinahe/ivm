#include "pub/mem.h"
#include "pub/com.h"
#include "pub/err.h"
#include "pub/vm.h"
#include "pub/inlines.h"

#include "gc/gc.h"
#include "func.h"
#include "context.h"
#include "runtime.h"
#include "call.h"
#include "coro.h"

IVM_INLINE
void
_ivm_function_init(ivm_vmstate_t *state,
				   ivm_function_t *func,
				   ivm_exec_t *body)
{
	func->is_native = IVM_FALSE;
	func->u.body = body;

	return;
}

IVM_INLINE
void
_ivm_function_initNative(ivm_vmstate_t *state,
						 ivm_function_t *func,
						 ivm_native_function_t native)
{
	func->is_native = IVM_TRUE;
	func->u.native = native;

	return;
}

ivm_function_t *
ivm_function_new(ivm_vmstate_t *state,
				 ivm_exec_t *body)
{
	ivm_function_t *ret = ivm_vmstate_allocFunc(state);

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("function"));

	_ivm_function_init(state, ret, body);

	return ret;
}

ivm_function_t *
ivm_function_newNative(ivm_vmstate_t *state,
					   ivm_native_function_t func)
{
	ivm_function_t *ret = ivm_vmstate_allocFunc(state);

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("native function"));

	_ivm_function_initNative(state, ret, func);

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

void
ivm_function_object_destructor(ivm_object_t *obj,
							   ivm_vmstate_t *state)
{	
	ivm_ctchain_free(IVM_AS(obj, ivm_function_object_t)->closure, state);
	// ivm_function_free(func->val, state);

	return;
}

ivm_object_t *
ivm_function_object_new(ivm_vmstate_t *state,
						ivm_ctchain_t *context,
						ivm_function_t *func)
{
	ivm_function_object_t *ret = ivm_vmstate_alloc(state, sizeof(*ret));

	ivm_object_init(IVM_AS_OBJ(ret), state, IVM_FUNCTION_OBJECT_T);

	ret->closure = ivm_ctchain_addRef(context);
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
