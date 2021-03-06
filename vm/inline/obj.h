#ifndef _IVM_VM_INLINE_OBJ_H_
#define _IVM_VM_INLINE_OBJ_H_

#include "pub/com.h"
#include "pub/vm.h"

#include "std/string.h"

#include "vm/instr.h"
#include "vm/obj.h"
#include "vm/slot.h"

IVM_COM_HEADER

IVM_INLINE
void
ivm_object_init(ivm_object_t *obj,
				ivm_type_t *type)
{
	// STD_INIT(&obj->slots, sizeof(obj->slots) + sizeof(obj->mark));
	// obj->proto = ivm_type_getProto(
	// 	obj->type = ivm_vmstate_getType(state, type)
	// );

	STD_MEMCPY(obj, ivm_type_getHeader(type), sizeof(obj->type) + sizeof(obj->proto));
	STD_INIT(&obj->slots, sizeof(obj->slots) + sizeof(obj->mark));

	return;
}

IVM_INLINE
ivm_object_t *
ivm_object_clone(ivm_object_t *obj,
				 ivm_vmstate_t *state)
{
	ivm_type_t *type = IVM_TYPE_OF(obj);
	ivm_size_t size = type->size;
	ivm_object_t *ret = ivm_vmstate_alloc(state, size);

	STD_MEMCPY(ret, obj, size);

	IVM_OBJECT_SET(ret, GEN, 0);
	IVM_OBJECT_SET(ret, WB, 0);
	ret->slots = ivm_slot_table_copyShared(ret->slots, state);

	if (type->clone) {
		type->clone(ret, state);
	}

	return ret;
}

IVM_INLINE
ivm_object_t *
ivm_object_new(ivm_vmstate_t *state)
{
	ivm_object_t *ret = ivm_vmstate_alloc(state, sizeof(*ret));

	ivm_object_init(ret, IVM_BTTYPE(state, IVM_OBJECT_T));

	return ret;
}

IVM_INLINE
ivm_object_t *
ivm_object_new_c(ivm_vmstate_t *state,
				 ivm_size_t prealloc)
{
	ivm_object_t *ret = ivm_vmstate_alloc(state, sizeof(*ret));

	ivm_object_init(ret, IVM_BTTYPE(state, IVM_OBJECT_T));
	ret->slots = ivm_slot_table_new_c(state, prealloc);

	return ret;
}

IVM_INLINE
ivm_object_t *
ivm_object_new_t(ivm_vmstate_t *state,
				 ivm_slot_table_t *slots)
{
	ivm_object_t *ret = ivm_vmstate_alloc(state, sizeof(*ret));

	ivm_object_init(ret, IVM_BTTYPE(state, IVM_OBJECT_T));
	ret->slots = slots;

	if (slots) {
		ret->mark.sub.oop = IVM_TRUE;
	}

	return ret;
}

IVM_INLINE
void
ivm_object_initSlots(ivm_object_t *obj,
					 ivm_vmstate_t *state,
					 ivm_size_t prealloc)
{
	obj->slots = ivm_slot_table_new_c(state, prealloc);
	return;
}

IVM_INLINE
ivm_object_t *
ivm_none_new(ivm_vmstate_t *state)
{
	ivm_object_t *ret = ivm_vmstate_alloc(state, sizeof(*ret));

	ivm_object_init(ret, IVM_BTTYPE(state, IVM_NONE_T));
	ret->mark.sub.locked = IVM_TRUE;

	return ret;
}

IVM_INLINE
ivm_bool_t
ivm_object_hasProto(ivm_object_t *obj,
					ivm_object_t *proto)
{
	while (obj) {
		if (obj == proto) return IVM_TRUE;
		obj = obj->proto;
	}

	return IVM_FALSE;
}

IVM_INLINE
void
ivm_object_setProto(ivm_object_t *obj,
					ivm_vmstate_t *state,
					ivm_object_t *proto)
{
	if (proto) {
		IVM_WBOBJ(state, obj, proto);
		// obj->mark.sub.oop |= proto->mark.sub.oop;
	}
	
	obj->proto = proto;

	return;
}

IVM_INLINE
ivm_object_t *
ivm_object_getProto(ivm_object_t *obj)
{
	return obj->proto;
}

IVM_INLINE
ivm_bool_t
ivm_object_isBTProto(ivm_object_t *obj)
{
	return IVM_OBJECT_GET(obj, BTPROTO); // ivm_type_getProto(obj->type) == obj; // && ivm_type_isBuiltin(obj->type);
}

#define IVM_NONE(state) ivm_vmstate_getNone(state)
#define IVM_IS_NONE(state, obj) ((obj) == ivm_vmstate_getNone(state))

#define _IVM_OBJECT_OPSEARCH(obj, found_oop, search_bt, failed) \
	register ivm_object_t *tmp;                                               \
	register ivm_type_t *otype = obj->type;                                   \
	register ivm_type_t *objt = IVM_BTTYPE(state, IVM_OBJECT_T);              \
	                                                                          \
	for (; obj; obj = obj->proto) {                                           \
		if (!ivm_object_hasOop(obj)) {                                        \
BACK:                                                                         \
			if ((obj->type == otype || obj->type == objt) &&                  \
				ivm_object_isBTProto(obj)) {                                  \
				search_bt;                                                    \
			}                                                                 \
		} else {                                                              \
			tmp = ivm_slot_table_getOop(obj->slots, state, oop_id);           \
			if (tmp) {                                                        \
				if (tmp == IVM_OOP_BLOCK) continue;                           \
				/* don't search built-in ops(explicitly deleted) */           \
				found_oop;                                                    \
			}                                                                 \
			goto BACK;                                                        \
		}                                                                     \
                                                                              \
	}                                                                         \
	failed;

IVM_INLINE
ivm_binop_proc_t /* null for finding oop */
ivm_object_getBinOp(ivm_object_t *obj, ivm_vmstate_t *state,
					ivm_int_t op, ivm_int_t oop_id,
					ivm_object_t *op2,
					ivm_object_t **oop)
{
	register ivm_binop_proc_t tmp_proc;

	_IVM_OBJECT_OPSEARCH(obj, {
		*oop = tmp;
		return IVM_NULL;
	}, {
		tmp_proc = IVM_OBJECT_GET_BINOP_PROC_R(obj, op, op2);
		if (tmp_proc) return tmp_proc;

		tmp_proc = IVM_OBJECT_GET_BINOP_PROC_RT(obj, op, IVM_OBJECT_T);
		if (tmp_proc) return tmp_proc;
	}, {
		*oop = IVM_NULL;
		return IVM_NULL;
	});
}

IVM_INLINE
ivm_binop_proc_t /* native handlers */
ivm_object_getBinOp_n(ivm_object_t *obj, ivm_vmstate_t *state,
					  ivm_int_t op, ivm_int_t oop_id,
					  ivm_object_t *op2,
					  ivm_object_t **oop,
					  ivm_native_function_t self)
{
	register ivm_binop_proc_t tmp_proc;

	_IVM_OBJECT_OPSEARCH(obj, {
		if (IVM_IS_BTTYPE(tmp, state, IVM_FUNCTION_OBJECT_T) &&
			ivm_function_object_checkNative_c(tmp, self)) goto BACK;
		*oop = tmp;
		return IVM_NULL;
	}, {
		tmp_proc = IVM_OBJECT_GET_BINOP_PROC_R(obj, op, op2);
		if (tmp_proc) return tmp_proc;

		tmp_proc = IVM_OBJECT_GET_BINOP_PROC_RT(obj, op, IVM_OBJECT_T);
		if (tmp_proc) return tmp_proc;
	}, {
		*oop = IVM_NULL;
		return IVM_NULL;
	});
}

IVM_INLINE
ivm_uniop_proc_t /* null for finding oop */
ivm_object_getUniOp(ivm_object_t *obj, ivm_vmstate_t *state,
					ivm_int_t op, ivm_int_t oop_id,
					ivm_object_t **oop)
{
	register ivm_uniop_proc_t tmp_proc;

	_IVM_OBJECT_OPSEARCH(obj, {
		*oop = tmp;
		return IVM_NULL;
	}, {
		tmp_proc = IVM_OBJECT_GET_UNIOP_PROC_R(obj, op);
		if (tmp_proc) return tmp_proc;
	}, {
		*oop = IVM_NULL;
		return IVM_NULL;
	});
}

IVM_INLINE
ivm_slot_table_t *
ivm_object_copyOnWrite(ivm_object_t *obj,
					   ivm_vmstate_t *state)
{
	if (obj->slots && ivm_slot_table_isShared(obj->slots)) {
		obj->slots = ivm_slot_table_copyOnWrite(obj->slots, state);
		IVM_WBOBJ_SLOT(state, obj, obj->slots);
	}

	return obj->slots;
}

IVM_INLINE
void
ivm_object_merge(ivm_object_t *obj,
				 ivm_vmstate_t *state,
				 ivm_object_t *mergee,
				 ivm_bool_t overw)
{
	if (obj->slots && mergee->slots) {
		ivm_object_copyOnWrite(obj, state);
		ivm_slot_table_merge(obj->slots, state, mergee->slots, overw);
		obj->mark.sub.oop |= mergee->mark.sub.oop;
	} else if (mergee->slots) {
		obj->slots = ivm_slot_table_copy_state(mergee->slots, state);
		obj->mark.sub.oop = mergee->mark.sub.oop;
		IVM_WBOBJ_SLOT(state, obj, obj->slots);
	}

	return;
}

IVM_INLINE
ivm_function_object_t *
ivm_object_callable(ivm_object_t *obj,
					ivm_vmstate_t *state,
					ivm_object_t **base_p)
{
	if (IVM_IS_BTTYPE(obj, state, IVM_FUNCTION_OBJECT_T)) {
		*base_p = IVM_NULL;
		return IVM_AS(obj, ivm_function_object_t);
	}

	ivm_object_t *bas = IVM_NULL;

	do {
		bas = obj;
		obj = ivm_object_getOop(obj, state, IVM_OOP_ID(CALL));
		if (!obj) {
			*base_p = IVM_NULL;
			return IVM_AS(obj, ivm_function_object_t);
		}
	} while (!IVM_IS_BTTYPE(obj, state, IVM_FUNCTION_OBJECT_T));

	*base_p = bas;

	return IVM_AS(obj, ivm_function_object_t);
}

IVM_COM_END

#endif
