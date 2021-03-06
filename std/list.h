#ifndef _IVM_STD_LIST_H_
#define _IVM_STD_LIST_H_

#include "pub/com.h"
#include "pub/const.h"
#include "pub/type.h"
#include "pub/err.h"

#include "mem.h"

IVM_COM_HEADER

IVM_INLINE
ivm_long_t
ivm_list_realIndex(ivm_long_t size,
				   ivm_long_t i)
{
	if (!size) {
		return i < 0 ? 0 : i;
	}

	if (i < 0) {
		i = -i % size;

		if (i) {
			i = size - i;
		}
	}

	return i;
}

IVM_INLINE
void
ivm_list_reverse_c(void *lst,
				   ivm_size_t size,
				   ivm_size_t esize)
{
	void *end = (void *)((ivm_ptr_t)lst + (size - 1) * esize);
	ivm_byte_t tmp[esize];

	while (end > lst) {
		STD_MEMCPY(tmp, lst, esize);
		STD_MEMCPY(lst, end, esize);
		STD_MEMCPY(end, tmp, esize);

		end = (void *)((ivm_ptr_t)end - esize);
		lst = (void *)((ivm_ptr_t)lst + esize);
	}

	return;
}

IVM_INLINE
void
ivm_ptlist_reverse_c(void **lst, ivm_size_t size)
{
	void **end = lst + size - 1;
	void **tmp;

	while (end > lst) {
		tmp = *lst;
		*lst = *end;
		*end = tmp;

		end--;
		lst++;
	}

	return;
}

typedef struct {
	ivm_size_t alloc;
	ivm_size_t cur;
	void **lst;
} ivm_ptlist_t;

ivm_ptlist_t *
ivm_ptlist_new_c(ivm_size_t buf_size);

#define ivm_ptlist_new() (ivm_ptlist_new_c(IVM_DEFAULT_PTLIST_BUFFER_SIZE))

void
ivm_ptlist_free(ivm_ptlist_t *ptlist);

void
ivm_ptlist_init_c(ivm_ptlist_t *ptlist, ivm_size_t buf_size);

void
ivm_ptlist_init_t(ivm_ptlist_t *ptlist,
				  void **lst,
				  ivm_size_t size);

IVM_INLINE
void
ivm_ptlist_dump(ivm_ptlist_t *ptlist)
{
	if (ptlist) {
		STD_FREE(ptlist->lst);
	}

	return;
}

#define ivm_ptlist_setBufferSize(ptlist, size) ((ptlist)->buf_size = (size))

#define ivm_ptlist_size(ptlist) ((ptlist)->cur)
#define ivm_ptlist_at(ptlist, i) ((ptlist)->lst[i])
#define ivm_ptlist_ptrAt(ptlist, i) ((ptlist)->lst + (i))
#define ivm_ptlist_set(ptlist, i, val) ((ptlist)->lst[i] = (val))

IVM_INLINE
void
ivm_ptlist_reverse(ivm_ptlist_t *ptlist)
{
	ivm_ptlist_reverse_c(ptlist->lst, ptlist->cur);
	return;
}

#define ivm_ptlist_core(ptlist) ((ptlist)->lst)

IVM_INLINE
void
ivm_ptlist_inc(ivm_ptlist_t *ptlist)
{
	ptlist->lst = STD_REALLOC(ptlist->lst, sizeof(*ptlist->lst) * (ptlist->alloc <<= 1));
	IVM_MEMCHECK(ptlist->lst);

	return;
}

IVM_INLINE
ivm_size_t
ivm_ptlist_push(ivm_ptlist_t *ptlist, void *p)
{
	if (ptlist->cur >= ptlist->alloc) {
		ivm_ptlist_inc(ptlist);
	}

	ptlist->lst[ptlist->cur] = p;

	return ptlist->cur++;
}

IVM_INLINE
ivm_size_t
ivm_ptlist_pushFront(ivm_ptlist_t *ptlist, void *p)
{
	if (ptlist->cur >= ptlist->alloc) {
		ivm_ptlist_inc(ptlist);
	}

	STD_MEMMOVE(ptlist->lst + 1, ptlist->lst, sizeof(*ptlist->lst) * ptlist->cur);

	ptlist->lst[0] = p;

	return ptlist->cur++;
}

IVM_INLINE
void *
ivm_ptlist_pop(ivm_ptlist_t *ptlist)
{
	return ptlist->cur ? ptlist->lst[--ptlist->cur] : IVM_NULL;
}

#define ivm_ptlist_setCur(ptlist, t) ((ptlist)->cur = (t))
#define ivm_ptlist_incCur(ptlist, t) ((ptlist)->cur += (t))
#define ivm_ptlist_empty(ptlist) (ivm_ptlist_setCur((ptlist), 0))
#define ivm_ptlist_has(ptlist, i) ((ptlist)->cur > (i))

void
ivm_ptlist_compact(ivm_ptlist_t *ptlist);

#define ivm_ptlist_offset(ptlist, ptr) \
	(((ivm_ptr_t)(ptr) - (ivm_ptr_t)(ptlist)->lst) / sizeof(*ptr))

typedef void **ivm_ptlist_iterator_t;

#define IVM_PTLIST_ITER_TYPE(elem_type) elem_type *
#define IVM_PTLIST_ITER_BEGIN(ptlist) ((ptlist)->lst)
#define IVM_PTLIST_ITER_END(ptlist) ((ptlist)->lst + (ptlist)->cur)
#define IVM_PTLIST_ITER_AT(ptlist, i) ((ptlist)->lst + (i))
#define IVM_PTLIST_ITER_INDEX(ptlist, ptr) ivm_ptlist_offset((ptlist), (ptr))
#define IVM_PTLIST_ITER_SET(iter, val) (*(iter) = val)
#define IVM_PTLIST_ITER_GET(iter) (*(iter))
#define IVM_PTLIST_ITER_IS_LAST(ptlist, iter) ((void *)(iter + 1) == (ptlist)->lst + (ptlist)->cur)
#define IVM_PTLIST_ITER_IS_FIRST(ptlist, iter) ((void *)iter == (ptlist)->lst)
#define IVM_PTLIST_EACHPTR(ptlist, iter, type) \
	type *__pl_end_##iter##__; \
	for ((iter) = (type *)((ptlist)->lst), \
		 __pl_end_##iter##__ = (iter) + (ptlist)->cur; \
		 (iter) != __pl_end_##iter##__; \
		 (iter)++)

#define IVM_PTLIST_EACHPTR_R(ptlist, iter, type) \
	type *__pl_begin_##iter##__; \
	for (__pl_begin_##iter##__ = (type *)((ptlist)->lst) - 1, \
		 (iter) = __pl_begin_##iter##__ + (ptlist)->cur; \
		 (iter) != __pl_begin_##iter##__; \
		 (iter)--)

typedef int (*ivm_ptlist_comparer_t)(const void *, const void *);

ivm_size_t
ivm_ptlist_indexOf_c(ivm_ptlist_t *ptlist, void *ptr, ivm_ptlist_comparer_t comp);

#define ivm_ptlist_indexOf(ptlist, ptr, comp) \
	(ivm_ptlist_indexOf_c((ptlist), \
						  (ptr), \
						  (ivm_ptlist_comparer_t)(comp)))

/* find a pointer with the identical address */
IVM_INLINE
ivm_size_t
ivm_ptlist_find(ivm_ptlist_t *ptlist,
				void *ptr)
{
	void **i, **end;

	for (i = ptlist->lst,
		 end = i + ptlist->cur;
		 i != end; i++) {
		if (*i == ptr)
			return ivm_ptlist_offset(ptlist, i);
	}

	return -1;
}

IVM_INLINE
void
ivm_ptlist_incTo(ivm_ptlist_t *ptlist,
				 ivm_size_t size)
{
	if (size > ptlist->alloc) {
		ptlist->alloc = size;
		ptlist->lst = STD_REALLOC(
			ptlist->lst,
			sizeof(*ptlist->lst) * ptlist->alloc
		);
		IVM_MEMCHECK(ptlist->lst);
	}

	return;
}

IVM_INLINE
void
ivm_ptlist_insert(ivm_ptlist_t *ptlist,
				  ivm_size_t i, void *p)
{
	if (i >= ptlist->cur) {
		ivm_ptlist_incTo(ptlist, i + 1);
		STD_INIT(ptlist->lst + ptlist->cur,
				 sizeof(*ptlist->lst) * (i - ptlist->cur));
		ptlist->cur = i + 1;
	}

	ptlist->lst[i] = p;

	return;
}

typedef struct {
	ivm_byte_t *lst;
	ivm_size_t esize;
	ivm_size_t alloc;
	ivm_size_t cur;
} ivm_list_t;

ivm_list_t *
ivm_list_new_c(ivm_size_t esize, ivm_size_t buf_size);

#define ivm_list_new(size) (ivm_list_new_c((size), IVM_DEFAULT_LIST_BUFFER_SIZE))

void
ivm_list_init_c(ivm_list_t *list,
				ivm_size_t esize,
				ivm_size_t buf_size);

void
ivm_list_free(ivm_list_t *list);

IVM_INLINE
void
ivm_list_dump(ivm_list_t *list)
{
	if (list) {
		STD_FREE(list->lst);
	}

	return;
}

IVM_INLINE
void
_ivm_list_expand(ivm_list_t *list)
{
	list->alloc <<= 1;
	list->lst = STD_REALLOC(list->lst, list->esize * list->alloc);
	return;
}

IVM_INLINE
ivm_size_t
ivm_list_push(ivm_list_t *list, void *e)
{
	if (list->cur >= list->alloc) {
		_ivm_list_expand(list);
	}

	STD_MEMCPY(list->lst + (list->cur * list->esize),
			 e, list->esize);

	return ++list->cur;
}

IVM_INLINE
ivm_size_t
ivm_list_prepush(ivm_list_t *list, void **ptr)
{
	if (list->cur >= list->alloc) {
		_ivm_list_expand(list);
	}

	*ptr = list->lst + (list->cur * list->esize);

	return ++list->cur;
}

IVM_INLINE
void *
ivm_list_at(ivm_list_t *list, ivm_size_t i)
{
	return list->lst + (i * list->esize);
}

IVM_INLINE
void
ivm_list_reverse(ivm_list_t *list)
{
	ivm_list_reverse_c(list->lst, list->cur, list->esize);
	return;
}

#define ivm_list_has(list, i) ((list)->cur > (i))

#define ivm_list_size(list) ((list)->cur)
#define ivm_list_empty(list) ((list)->cur = 0)
#define ivm_list_core(list) ((list)->lst)

IVM_INLINE
void *
ivm_list_pop(ivm_list_t *list)
{
	return list->lst + --list->cur;
}

#define ivm_list_isEmpty(list) ((list)->cur == 0)
#define ivm_list_isLast(list) (((ivm_ptr_t)(iter) - (ivm_ptr_t)(list)->lst) / (list)->esize == (list)->cur - 1)

#define ivm_list_offset(list, ptr) \
	(((ivm_ptr_t)(ptr) - (ivm_ptr_t)(list)->lst) / (list)->esize)

#define IVM_LIST_ITER_TYPE(elem_type) elem_type *
#define IVM_LIST_ITER_SET(iter, val, type) (*((type *)(iter)) = val)
#define IVM_LIST_ITER_GET(iter, type) (*((type *)(iter)))
#define IVM_LIST_ITER_GET_PTR(iter, type) ((type *)(iter))
#define IVM_LIST_ITER_INDEX(list, iter) ivm_list_offset((list), (iter))
#define IVM_LIST_ITER_IS_LAST(list, iter, type) ((type *)(iter) + 1 == (type *)(list)->lst + (list)->cur)
#define IVM_LIST_ITER_IS_FIRST(list, iter, type) ((type *)(iter) == (type *)(list)->lst)
#define IVM_LIST_EACHPTR(list, iter, type) \
	type *__l_end_##iter##__; \
	for ((iter) = (type *)((list)->lst), \
		 __l_end_##iter##__ = (type *)(iter) + (list)->cur; \
		 (iter) != __l_end_##iter##__; \
		 (iter)++)
#define IVM_LIST_EACHPTR_R(list, iter, type) \
	type *__l_begin_##iter##__; \
	for (__l_begin_##iter##__ = (type *)((list)->lst) - 1, \
		 (iter) = __l_begin_##iter##__ + (list)->cur; \
		 (iter) != __l_begin_##iter##__; \
		 (iter)--)

IVM_COM_END

#endif
