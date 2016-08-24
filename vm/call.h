#ifndef _IVM_VM_CALL_H_
#define _IVM_VM_CALL_H_

#include "pub/com.h"
#include "pub/const.h"
#include "pub/type.h"
#include "pub/mem.h"

#include "std/pool.h"

#include "instr.h"
#include "exec.h"

IVM_COM_HEADER

/* part needed to init every call */
#define IVM_FRAME_HEADER_INIT \
	ivm_instr_t *ip;                 \
	ivm_instr_t *cat; /* catch */    \
	ivm_uint_t offset;               \
	ivm_bool_t no_reg;

#define IVM_FRAME_HEADER \
	struct ivm_context_t_tag *ctx;   \
	struct ivm_object_t_tag **bp;    \
	/* init part */                  \
	IVM_FRAME_HEADER_INIT

#define IVM_FRAME_HEADER_SIZE \
	(sizeof(struct { IVM_FRAME_HEADER }))

#define IVM_FRAME_HEADER_INIT_SIZE \
	(sizeof(struct { IVM_FRAME_HEADER_INIT }))

#define IVM_FRAME_INIT_HEADER(frame) \
	(MEM_INIT(&(frame)->ip, IVM_FRAME_HEADER_INIT_SIZE))

struct ivm_vmstate_t_tag;
struct ivm_runtime_t_tag;
struct ivm_object_t_tag;

typedef struct ivm_frame_t_tag {
	IVM_FRAME_HEADER
} ivm_frame_t;

#define IVM_FRAME_GET_BP(frame) ((frame)->bp)
#define IVM_FRAME_GET_IP(frame) ((frame)->ip)
#define IVM_FRAME_GET_CONTEXT(frame) ((frame)->ctx)
#define IVM_FRAME_GET_CATCH(frame) ((frame)->cat)
#define IVM_FRAME_GET_NO_REG(frame) ((frame)->no_reg)

#define IVM_FRAME_SET_BP(frame, val) ((frame)->bp = (val))
#define IVM_FRAME_SET_CATCH(frame, val) ((frame)->cat = (val))
#define IVM_FRAME_SET_NO_REG(frame, val) ((frame)->no_reg = (val))

#define IVM_FRAME_GET(obj, member) IVM_GET((obj), IVM_FRAME, member)
#define IVM_FRAME_SET(obj, member, val) IVM_SET((obj), IVM_FRAME, member, (val))

typedef struct ivm_frame_stack_t_tag {
	ivm_uint_t alloc;
	ivm_uint_t top;
	ivm_frame_t *frames;
} ivm_frame_stack_t;

typedef ivm_frame_t *ivm_frame_stack_iterator_t;

ivm_frame_stack_t *
ivm_frame_stack_new();

void
ivm_frame_stack_free(ivm_frame_stack_t *stack);

void
ivm_frame_stack_init(ivm_frame_stack_t *stack);

void
ivm_frame_stack_dump(ivm_frame_stack_t *stack);

#define ivm_frame_stack_size(stack) ((stack)->top)
#define ivm_frame_stack_at(stack, i) ((stack)->frames + (i))

#define IVM_FRAME_STACK_ITER_SET(iter, val) (*(iter) = (val))
#define IVM_FRAME_STACK_ITER_GET(iter) (iter)
#define IVM_FRAME_STACK_EACHPTR(stack, iter) \
	ivm_frame_stack_iterator_t __fs_end_##iter##__; \
	for ((iter) = (stack)->frames, \
		 __fs_end_##iter##__ = (iter) + (stack)->top; \
		 (iter) != __fs_end_##iter##__; \
		 (iter)++)

typedef ivm_ptpool_t ivm_frame_pool_t;

#define ivm_frame_pool_new(count) (ivm_ptpool_new((count), sizeof(ivm_frame_t)))
#define ivm_frame_pool_free ivm_ptpool_free
#define ivm_frame_pool_alloc(pool) ((ivm_frame_t *)ivm_ptpool_alloc(pool))
#define ivm_frame_pool_dump ivm_ptpool_dump

IVM_COM_END

#endif
