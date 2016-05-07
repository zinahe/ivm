#include "pub/mem.h"
#include "func.h"
#include "context.h"
#include "runtime.h"
#include "vm.h"
#include "call.h"
#include "coro.h"
#include "gc/gc.h"
#include "err.h"

static
void
ivm_function_init(ivm_function_t *func,
				  ivm_ctchain_t *context,
				  ivm_param_list_t *param_list,
				  ivm_exec_t *body,
				  ivm_signal_mask_t intsig)
{
	func->is_native = IVM_FALSE;
	func->u.f.param_list = param_list;
	func->u.f.closure = context
						? ivm_ctchain_clone(context)
						: IVM_NULL;
	func->u.f.body = body;
	func->intsig = intsig;

	return;
}

static
void
ivm_function_initNative(ivm_function_t *func,
						ivm_native_function_t native,
						ivm_signal_mask_t intsig)
{
	func->is_native = IVM_TRUE;
	func->u.native = native;
	func->intsig = intsig;

	return;
}

ivm_function_t *
ivm_function_new(ivm_ctchain_t *context,
				 ivm_param_list_t *param_list,
				 ivm_exec_t *body,
				 ivm_signal_mask_t intsig)
{
	ivm_function_t *ret = MEM_ALLOC(sizeof(*ret));

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("function"));

	ivm_function_init(ret, context, param_list, body, intsig);

	return ret;
}

ivm_function_t *
ivm_function_newNative(ivm_native_function_t func,
					   ivm_signal_mask_t intsig)
{
	ivm_function_t *ret = MEM_ALLOC(sizeof(*ret));

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("native function"));

	ivm_function_initNative(ret, func, intsig);

	return ret;
}

void
ivm_function_free(ivm_function_t *func)
{
	if (func) {
		if (!func->is_native) {
			ivm_param_list_free(func->u.f.param_list);
			ivm_ctchain_free(func->u.f.closure);
		}

		MEM_FREE(func);
	}

	return;
}

ivm_function_t *
ivm_function_clone(ivm_function_t *func)
{
	ivm_function_t *ret = MEM_ALLOC(sizeof(*ret));

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("cloned function"));

	MEM_COPY(ret, func, sizeof(*ret));
	if (!ret->is_native) {
		ret->u.f.closure = ivm_ctchain_clone(ret->u.f.closure);
	}

	return ret;
}

ivm_runtime_t *
ivm_function_createRuntime(ivm_vmstate_t *state,
						   const ivm_function_t *func)
{
	ivm_runtime_t *ret = IVM_NULL;

	if (func) {
		if (func->is_native) {
			ret = ivm_runtime_new(IVM_NULL, IVM_NULL);
		} else {
			ret = ivm_runtime_new(func->u.f.body, func->u.f.closure);
			ivm_ctchain_addContext(IVM_RUNTIME_GET(ret, CONTEXT),
								   ivm_context_new(state));
		}
	}

	return ret;
}

ivm_caller_info_t *
ivm_function_invoke(const ivm_function_t *func, ivm_coro_t *coro)
{
	if (func->is_native)
		return IVM_NULL;

	return ivm_runtime_invoke(coro->runtime, coro,
							  func->u.f.body, func->u.f.closure);
}

ivm_object_t *
ivm_function_callNative(const ivm_function_t *func,
						ivm_vmstate_t *state,
						ivm_ctchain_t *context,
						IVM_FUNCTION_COMMON_ARG)
{
	return func->u.native(state, context, IVM_FUNCTION_COMMON_ARG_PASS);
}

void
ivm_function_setParam(const ivm_function_t *func,
					  ivm_vmstate_t *state,
					  ivm_ctchain_t *context, IVM_FUNCTION_COMMON_ARG)
{
	ivm_argc_t i = 0;
	ivm_param_list_t *param_list;
	ivm_param_list_iterator_t iter;
	ivm_char_t *name;

	if (!func->is_native
		&& (param_list = func->u.f.param_list)) {
		IVM_PARAM_LIST_EACHPTR(param_list, iter) {
			name = IVM_PARAM_LIST_ITER_GET(iter);
			printf("%s\n", name);
			if (i < argc) {
				ivm_ctchain_setLocalSlot(context, state, name, argv[i]);
			} else {
				ivm_ctchain_setLocalSlot(context, state, name, IVM_UNDEFINED(state));
			}

			i++;
		}
	}

	return;
}

void
ivm_function_object_destructor(ivm_object_t *obj,
							   ivm_vmstate_t *state)
{
	ivm_function_t *func = IVM_AS(obj, ivm_function_object_t)->val;
	
	ivm_function_free(func);

	return;
}

ivm_object_t *
ivm_function_object_new(ivm_vmstate_t *state, ivm_function_t *func)
{
	ivm_function_object_t *ret = ivm_vmstate_alloc(state, sizeof(*ret));

	ivm_object_init(IVM_AS_OBJ(ret), state, IVM_FUNCTION_OBJECT_T);

	ret->val = ivm_function_clone(func);
	ivm_vmstate_addDesLog(state, IVM_AS_OBJ(ret)); /* function objects need destruction */

	return IVM_AS_OBJ(ret);
}

ivm_object_t *
ivm_function_object_new_nc(struct ivm_vmstate_t_tag *state,
						   ivm_function_t *func)
{
	ivm_function_object_t *ret = ivm_vmstate_alloc(state, sizeof(*ret));

	ivm_object_init(IVM_AS_OBJ(ret), state, IVM_FUNCTION_OBJECT_T);

	ret->val = func;
	ivm_vmstate_addDesLog(state, IVM_AS_OBJ(ret));

	return IVM_AS_OBJ(ret);
}

void
ivm_function_object_traverser(ivm_object_t *obj,
							  ivm_traverser_arg_t *arg)
{
	ivm_function_t *func = IVM_AS(obj, ivm_function_object_t)->val;

	if (!func->is_native) {
		arg->trav_ctchain(func->u.f.closure, arg);
	}

	return;
}
