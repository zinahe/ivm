#include "pub/mem.h"
#include "list.h"
#include "err.h"

ivm_ptlist_t *
ivm_ptlist_new_c(ivm_size_t buf_size)
{
	ivm_ptlist_t *ret = MEM_ALLOC(sizeof(*ret),
								  ivm_ptlist_t *);

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("ptlist"));

	ret->alloc
	= buf_size;

	ret->cur = 0;
	ret->lst = MEM_ALLOC(sizeof(*ret->lst)
						 * buf_size,
						 void **);

	IVM_ASSERT(ret->lst, IVM_ERROR_MSG_FAILED_ALLOC_NEW("ptlist buffer"));

	return ret;
}

void
ivm_ptlist_free(ivm_ptlist_t *ptlist)
{
	if (ptlist) {
		MEM_FREE(ptlist->lst);
		MEM_FREE(ptlist);
	}

	return;
}

#define VALUE_AT(ptlist, i) ((ptlist)->lst[i])

void
ivm_ptlist_foreach(ivm_ptlist_t *ptlist, ivm_ptlist_foreach_proc_t proc)
{
	ivm_size_t i;

	for (i = 0; i < ptlist->cur; i++)
		proc(VALUE_AT(ptlist, i));

	return;
}

void
ivm_ptlist_foreach_arg(ivm_ptlist_t *ptlist,
					   ivm_ptlist_foreach_proc_arg_t proc,
					   void *arg)
{
	ivm_size_t i;

	for (i = 0; i < ptlist->cur; i++)
		proc(VALUE_AT(ptlist, i), arg);

	return;
}

void
ivm_ptlist_compact(ivm_ptlist_t *ptlist)
{
	ptlist->lst = MEM_REALLOC(ptlist->lst,
							  sizeof(*ptlist->lst) * ptlist->cur,
							  void **);
	IVM_ASSERT(ptlist->lst, IVM_ERROR_MSG_FAILED_ALLOC_NEW("compacted pointer list"));
	ptlist->alloc = ptlist->cur;

	return;
}

ivm_size_t
ivm_ptlist_indexOf_c(ivm_ptlist_t *ptlist, void *ptr,
					 ivm_ptlist_comparer_t comp)
{
	ivm_size_t i;

	for (i = 0; i < ptlist->cur; i++) {
		if (!comp(ptlist->lst[i], ptr))
			return i;
	}

	return -1;
}
