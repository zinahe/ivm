#ifndef _IVM_VM_EXEC_H_
#define _IVM_VM_EXEC_H_

#include "pub/com.h"
#include "pub/const.h"
#include "pub/type.h"

#include "std/list.h"
#include "std/string.h"
#include "std/pool.h"
#include "std/ref.h"

#include "opcode.h"
#include "instr.h"

IVM_COM_HEADER

struct ivm_vmstate_t_tag;
struct ivm_context_t_tag;
struct ivm_object_t_tag;
struct ivm_function_t_tag;

typedef struct ivm_source_pos_t_tag {
	IVM_REF_HEADER
	ivm_char_t *file;
} ivm_source_pos_t;

ivm_source_pos_t *
ivm_source_pos_new(const ivm_char_t *file);

void
ivm_source_pos_free(ivm_source_pos_t *pos);

IVM_INLINE
const ivm_char_t *
ivm_source_pos_getFile(ivm_source_pos_t *pos)
{
	return pos ? pos->file : "<untraceable>";
}

typedef struct {
	const ivm_string_t *name;
	ivm_bool_t is_varg;
} ivm_param_t;

#define ivm_param_name(p) ((p)->name)
#define ivm_param_isVarg(p) ((p)->is_varg)

typedef struct {
	ivm_param_t *param;
	ivm_size_t count;
	ivm_bool_t has_varg;
	ivm_bool_t legacy; // don't match the argument but directly push them to the stack
} ivm_param_list_t;

void
ivm_param_list_init(ivm_param_list_t *plist,
					ivm_size_t count);

void
ivm_param_list_initLegacy(ivm_param_list_t *plist);

void
ivm_param_list_dump(ivm_param_list_t *plist);

void
ivm_param_list_setParam(ivm_param_list_t *plist,
						ivm_size_t idx, const ivm_string_t *name,
						ivm_bool_t is_varg);

#define ivm_param_list_getParam(list, i) ((list)->param + (i))

#define ivm_param_list_isLegacy(list) ((list)->legacy)
#define ivm_param_list_count(list) ((list)->count)

typedef struct ivm_exec_t_tag {
	IVM_REF_HEADER

	ivm_uint_t offset;

	ivm_string_pool_t *pool;
	ivm_source_pos_t *pos;

	// ivm_int_t max_stack; // max stack size needed
	// ivm_int_t fin_stack; // final stack size
	ivm_instr_t *instrs;
	ivm_uint_t alloc;
	ivm_uint_t next;

	ivm_param_list_t param;

	ivm_bool_t cached;
} ivm_exec_t;

ivm_exec_t *
ivm_exec_new(ivm_string_pool_t *pool,
			 ivm_size_t param);

void
ivm_exec_free(ivm_exec_t *exec);

void
ivm_exec_dump(ivm_exec_t *exec);

void
ivm_exec_copy(ivm_exec_t *exec,
			  ivm_exec_t *dest);

ivm_size_t
ivm_exec_addInstr_c(ivm_exec_t *exec,
					ivm_instr_t instr);

// (exec, instr, arg)
#define ivm_exec_addInstr(exec, ...) \
	(ivm_exec_addInstr_c((exec), IVM_INSTR_GEN(__VA_ARGS__, (exec))))

// (exec, lineno, instr, arg)
#define ivm_exec_addInstr_l(exec, ...) \
	(ivm_exec_addInstr_c((exec), IVM_INSTR_GEN_L(__VA_ARGS__, (exec))))

// (exec, lineno, instr, strlen, str)
#define ivm_exec_addInstr_nl(exec, ...) \
	(ivm_exec_addInstr_c((exec), IVM_INSTR_GEN_NL(__VA_ARGS__, (exec))))

#define ivm_exec_registerString(exec, str) ivm_string_pool_registerRaw_i((exec)->pool, (str))
#define ivm_exec_registerString_c(exec, str) ivm_string_pool_registerRaw((exec)->pool, (str))
#define ivm_exec_registerString_nc(exec, str, len) ivm_string_pool_registerRaw_n((exec)->pool, (str), (len))
#define ivm_exec_registerString_n(exec, str, len) ivm_string_pool_registerRaw_ni((exec)->pool, (str), (len))
#define ivm_exec_getString(exec, i) ivm_string_pool_get((exec)->pool, (i))

#define ivm_exec_length(exec) ((exec)->next)
#define ivm_exec_cur ivm_exec_length
#define ivm_exec_procAt(exec, i) ((exec)->instrs[i].proc)
#define ivm_exec_instrAt(exec, i) ((exec)->instrs[i])
#define ivm_exec_instrPtrAt(exec, i) ((exec)->instrs + (i))
#define ivm_exec_instrPtrStart(exec) ((exec)->instrs)
#define ivm_exec_instrPtrEnd(exec) ((exec)->instrs + (exec)->next)

#define ivm_exec_setParam(exec, i, name, is_varg) ivm_param_list_setParam(&(exec)->param, (i), (name), (is_varg))
#define ivm_exec_getParam(exec) (&(exec)->param)

IVM_INLINE
void
ivm_exec_empty(ivm_exec_t *exec)
{
	exec->cached = IVM_FALSE;
	exec->next = 0;
	return;
}

IVM_INLINE
void
ivm_exec_setSourcePos(ivm_exec_t *exec,
					  ivm_source_pos_t *pos)
{
	if (exec->pos) {
		ivm_source_pos_free(exec->pos);
	}

	if (pos) {
		ivm_ref_inc(pos);
	}

	exec->pos = pos;
	
	return;
}

#define ivm_exec_getSourcePos(exec) ((exec)->pos)

#define ivm_exec_setArgAt(exec, i, val) ((exec)->instrs[i].arg = ivm_opcode_arg_fromInt(val))

#define ivm_exec_argAt(exec, i) ((exec)->instrs[i].arg)
#define ivm_exec_opcAt(exec, i) ((exec)->instrs[i].opc)

#define ivm_exec_cached(exec) ((exec)->cached)
#define ivm_exec_setCached(exec, val) ((exec)->cached = (val))
#define ivm_exec_pool(exec) ((exec)->pool)
#define ivm_exec_instrs(exec) ((exec)->instrs)
#define ivm_exec_offset(exec) ((exec)->offset)

#define ivm_exec_setOffset(exec, val) ((exec)->offset = (val))
	
void
ivm_exec_preproc(ivm_exec_t *exec,
				 struct ivm_vmstate_t_tag *state);

IVM_INLINE
void
ivm_exec_initInstrCache(ivm_exec_t *exec)
{
	ivm_instr_t *i, *end;

	for (i = exec->instrs, end = i + exec->next;
		 i != end; i++) {
		ivm_instr_initCache(i);
	}

	return;
}

typedef ivm_ptpool_t ivm_exec_pool_t;

#define ivm_exec_pool_new(count) (ivm_ptpool_new((count), sizeof(ivm_exec_t)))
#define ivm_exec_pool_free ivm_ptpool_free
#define ivm_exec_pool_alloc(pool) ((ivm_exec_t *)ivm_ptpool_alloc(pool))
#define ivm_exec_pool_dump ivm_ptpool_dump

typedef ivm_ptlist_t ivm_exec_list_t;
typedef IVM_PTLIST_ITER_TYPE(ivm_exec_t *) ivm_exec_list_iterator_t;

#define ivm_exec_list_new() (ivm_ptlist_new_c(IVM_DEFAULT_EXEC_LIST_BUFFER_SIZE))
#define ivm_exec_list_size ivm_ptlist_size
#define ivm_exec_list_at(list, i) ((ivm_exec_t *)ivm_ptlist_at((list), (i)))

#define IVM_EXEC_LIST_ITER_SET(iter, val) IVM_PTLIST_ITER_SET((iter), (val))
#define IVM_EXEC_LIST_ITER_GET(iter) IVM_PTLIST_ITER_GET(iter)
#define IVM_EXEC_LIST_EACHPTR(list, iter) IVM_PTLIST_EACHPTR((list), iter, ivm_exec_t *)

IVM_INLINE
ivm_size_t
ivm_exec_list_push(ivm_exec_list_t *list,
				   ivm_exec_t *exec)
{
	ivm_ref_inc(exec);
	return ivm_ptlist_push(list, exec);
}

IVM_INLINE
void
ivm_exec_list_free(ivm_exec_list_t *list)
{
	ivm_exec_list_iterator_t eiter;

	if (list) {
		IVM_EXEC_LIST_EACHPTR(list, eiter) {
			ivm_exec_free(IVM_EXEC_LIST_ITER_GET(eiter));
		}
		ivm_ptlist_free(list);
	}

	return;
}

IVM_INLINE
void
ivm_exec_list_empty(ivm_exec_list_t *list)
{
	ivm_exec_list_iterator_t eiter;

	IVM_EXEC_LIST_EACHPTR(list, eiter) {
		ivm_exec_free(IVM_EXEC_LIST_ITER_GET(eiter));
	}
	ivm_ptlist_empty(list);

	return;
}

typedef struct {
	ivm_size_t root;
	ivm_source_pos_t *pos;
	ivm_exec_list_t *execs;
} ivm_exec_unit_t;

ivm_exec_unit_t *
ivm_exec_unit_new(ivm_size_t root,
				  ivm_exec_list_t *execs);

void
ivm_exec_unit_free(ivm_exec_unit_t *unit);

#define ivm_exec_unit_execList(unit) ((unit)->execs)
#define ivm_exec_unit_root(unit) ((unit)->root)

IVM_INLINE
void
ivm_exec_unit_setSourcePos(ivm_exec_unit_t *unit,
						   ivm_source_pos_t *pos)
{
	if (unit->pos) {
		ivm_source_pos_free(unit->pos);
	}

	if (pos) {
		ivm_ref_inc(pos);
	}

	unit->pos = pos;
	
	return;
}

#define ivm_exec_unit_getSourcePos(unit) ((unit)->pos)

IVM_INLINE
ivm_size_t
ivm_exec_unit_registerExec(ivm_exec_unit_t *unit,
						   ivm_exec_t *exec)
{
	return ivm_exec_list_push(unit->execs, exec);
}

struct ivm_function_t_tag * /* root function */
ivm_exec_unit_mergeToVM(ivm_exec_unit_t *unit,
						struct ivm_vmstate_t_tag *state);

struct ivm_vmstate_t_tag *
ivm_exec_unit_generateVM(ivm_exec_unit_t *unit);

IVM_COM_END

#endif
