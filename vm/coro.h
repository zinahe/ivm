#ifndef _IVM_VM_CORO_H_
#define _IVM_VM_CORO_H_

#include "pub/com.h"
#include "type.h"
#include "obj.h"
#include "vmstack.h"
#include "func.h"
#include "call.h"
#include "runtime.h"

IVM_COM_HEADER

struct ivm_vmstate_t_tag;

typedef struct ivm_coro_t_tag {
	ivm_vmstack_t *stack;
	ivm_frame_stack_t *frame_st;
	ivm_runtime_t *runtime;
} ivm_coro_t;

#define IVM_CORO_GET_STACK(coro) ((coro)->stack)
#define IVM_CORO_GET_FRAME_STACK(coro) ((coro)->frame_st)
#define IVM_CORO_GET_RUNTIME(coro) ((coro)->runtime)

#define IVM_CORO_GET(obj, member) IVM_GET((obj), IVM_CORO, member)
#define IVM_CORO_SET(obj, member, val) IVM_SET((obj), IVM_CORO, member, (val))

ivm_coro_t *
ivm_coro_new();
void
ivm_coro_free(ivm_coro_t *coro,
			  struct ivm_vmstate_t_tag *state);

#define ivm_coro_stackTop(coro) (ivm_vmstack_size(coro->stack))

ivm_object_t *
ivm_coro_start(ivm_coro_t *coro,
			   struct ivm_vmstate_t_tag *state,
			   ivm_function_t *root);

#define ivm_coro_setRoot(coro, state, root) \
	((coro)->runtime = ivm_function_createRuntime((root), (state)))

#define ivm_coro_isAsleep(coro) ((coro)->runtime != IVM_NULL)

#define ivm_coro_resume(coro, state, init) \
	((init) ? ivm_vmstack_push((coro)->stack, (init)) : 0, \
	 ivm_coro_start((coro), (state), IVM_NULL))

#define ivm_coro_pushFrame(coro, frame) (ivm_frame_stack_push((coro)->frame_st, (frame)))

typedef ivm_ptlist_t ivm_coro_list_t;

#define ivm_coro_list_new ivm_ptlist_new
#define ivm_coro_list_free ivm_ptlist_free
#define ivm_coro_list_add ivm_ptlist_push
#define ivm_coro_list_size ivm_ptlist_size
#define ivm_coro_list_at(list, i) ((ivm_coro_t *)ivm_ptlist_at((list), (i)))
#define ivm_coro_list_foreach ivm_ptlist_foreach
#define ivm_coro_list_foreach_arg ivm_ptlist_foreach_arg

IVM_COM_END

#endif
