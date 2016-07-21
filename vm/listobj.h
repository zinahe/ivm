#ifndef _IVM_VM_LISTOBJ_H_
#define _IVM_VM_LISTOBJ_H_

#include "pub/com.h"
#include "pub/const.h"
#include "pub/type.h"

#include "obj.h"

IVM_COM_HEADER

struct ivm_vmstate_t_tag;
struct ivm_heap_t_tag;

typedef struct {
	IVM_OBJECT_HEADER
	ivm_size_t alloc;
	ivm_size_t size;
	ivm_object_t **lst;
} ivm_list_object_t;

ivm_object_t *
ivm_list_object_new(struct ivm_vmstate_t_tag *state,
					ivm_size_t size);

ivm_object_t *
ivm_list_object_new_c(struct ivm_vmstate_t_tag *state,
					  ivm_object_t **init,
					  ivm_size_t size);

IVM_INLINE
ivm_object_t *
ivm_list_object_get(ivm_list_object_t *list,
					ivm_long_t i)
{
	if (i >= 0) {
		if (i < list->size) {
			return list->lst[i];
		}
	} else {
		i = -i % list->size;

		if (i) {
			return list->lst[list->size - i];
		} else {
			return *list->lst;
		}
	}

	return IVM_NULL;
}

ivm_object_t *
ivm_list_object_set(ivm_list_object_t *list,
					struct ivm_vmstate_t_tag *state,
					ivm_long_t i,
					ivm_object_t *obj);

ivm_object_t *
ivm_list_object_link(ivm_list_object_t *list1,
					 ivm_list_object_t *list2,
					 struct ivm_vmstate_t_tag *state);

void
ivm_list_object_destructor(ivm_object_t *obj,
						   struct ivm_vmstate_t_tag *state);

void
ivm_list_object_traverser(ivm_object_t *obj,
						  ivm_traverser_arg_t *arg);

IVM_COM_END

#endif
