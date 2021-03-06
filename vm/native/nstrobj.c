#include "pub/type.h"
#include "pub/const.h"
#include "pub/inlines.h"
#include "pub/vm.h"

#include "std/string.h"
#include "std/enc.h"

#include "vm/strobj.h"
#include "vm/num.h"

#include "priv.h"
#include "nstrobj.h"

IVM_NATIVE_FUNC(_string_cons)
{
	ivm_object_t *arg, *to_s;
	ivm_object_t *base;
	ivm_function_object_t *func;

	CHECK_ARG_COUNT(1);

	arg = NAT_ARG_AT(1);

	if (IVM_IS_BTTYPE(arg, NAT_STATE(), IVM_STRING_OBJECT_T)) {
		return ivm_object_clone(arg, NAT_STATE());
	} else if (IVM_IS_BTTYPE(arg, NAT_STATE(), IVM_NUMERIC_T)) {
		ivm_char_t buf[25];
		ivm_conv_dtoa(ivm_numeric_getValue(arg), buf);
		return ivm_string_object_new(NAT_STATE(), ivm_vmstate_constantize_r(NAT_STATE(), buf));
	} else if (IVM_IS_NONE(NAT_STATE(), arg)) {
		return ivm_string_object_new(NAT_STATE(), IVM_VMSTATE_CONST(NAT_STATE(), C_NONE));
	} else {
		to_s = ivm_object_getSlot(arg, NAT_STATE(), IVM_VMSTATE_CONST(NAT_STATE(), C_TO_S));
		
		if (to_s && (func = ivm_object_callable(to_s, NAT_STATE(), &base))) {
			return ivm_coro_callBase_0(NAT_CORO(), NAT_STATE(), func, base ? base : arg);
		}

		RTM_FATAL(IVM_ERROR_MSG_ILLEGAL_TO_S);
	}

	RTM_FATAL(IVM_ERROR_MSG_UNABLE_TO_CONVERT_STR(IVM_OBJECT_GET(arg, TYPE_NAME)));

	return IVM_NONE(NAT_STATE());
}

IVM_NATIVE_FUNC(_string_len)
{
	const ivm_string_t *str;

	CHECK_BASE(IVM_STRING_OBJECT_T);

	str = ivm_string_object_getValue(IVM_AS(NAT_BASE(), ivm_string_object_t));

	return ivm_numeric_new(NAT_STATE(), ivm_string_length(str));
}

IVM_NATIVE_FUNC(_string_ord)
{
	ivm_number_t idx = 0;
	ivm_size_t len, i;
	const ivm_string_t *str;

	CHECK_BASE(IVM_STRING_OBJECT_T);
	MATCH_ARG("*n", &idx);

	str = ivm_string_object_getValue(NAT_BASE());
	len = ivm_string_length(str);
	i = (ivm_size_t)idx;

	RTM_ASSERT(i < len, IVM_ERROR_MSG_STRING_IDX_EXCEED(i, len));

	return ivm_numeric_new(NAT_STATE(), ivm_string_charAt(str, i));
}

IVM_NATIVE_FUNC(_string_ords)
{
	const ivm_string_t *str;
	const ivm_char_t *i, *end;
	ivm_list_object_t *ret;
	ivm_number_t nlen;
	ivm_size_t len = -1;

	CHECK_BASE(IVM_STRING_OBJECT_T);

	str = ivm_string_object_getValue(NAT_BASE());
	nlen = ivm_string_length(str);

	MATCH_ARG("*n", &nlen);

	len = nlen;
	RTM_ASSERT(len <= ivm_string_length(str), IVM_ERROR_MSG_STRING_IDX_EXCEED(len, (ivm_size_t)ivm_string_length(str)));
	ret = IVM_AS(ivm_list_object_new_b(NAT_STATE(), len), ivm_list_object_t);

	for (i = ivm_string_trimHead(str),
		 end = i + len;
		 i != end; i++) {
		ivm_list_object_push(ret, NAT_STATE(), ivm_numeric_new(NAT_STATE(), *i));
	}

	return IVM_AS_OBJ(ret);
}

IVM_NATIVE_FUNC(_string_chars)
{
	const ivm_string_t *str;
	const ivm_char_t *i, *end;
	ivm_list_object_t *ret;
	ivm_number_t nlen;
	ivm_size_t len = -1;

	CHECK_BASE(IVM_STRING_OBJECT_T);

	str = ivm_string_object_getValue(NAT_BASE());
	nlen = ivm_string_length(str);

	MATCH_ARG("*n", &nlen);

	len = nlen;
	RTM_ASSERT(len <= ivm_string_length(str), IVM_ERROR_MSG_STRING_IDX_EXCEED(len, (ivm_size_t)ivm_string_length(str)));
	ret = IVM_AS(ivm_list_object_new_b(NAT_STATE(), len), ivm_list_object_t);

	for (i = ivm_string_trimHead(str),
		 end = i + len;
		 i != end; i++) {
		ivm_list_object_push(ret, NAT_STATE(), ivm_string_object_newChar(NAT_STATE(), *i));
	}

	return IVM_AS_OBJ(ret);
}

IVM_NATIVE_FUNC(_string_to_s)
{
	CHECK_BASE(IVM_STRING_OBJECT_T);
	return ivm_object_clone(NAT_BASE(), NAT_STATE());
}

IVM_NATIVE_FUNC(_string_split)
{
	const ivm_string_t *tok, *base;
	const ivm_char_t *rtok, *rbase;
	ivm_size_t tlen, blen, beg, i;
	ivm_list_object_t *ret;
	ivm_object_t *tmp;

	CHECK_BASE(IVM_STRING_OBJECT_T);

	base = ivm_string_object_getValue(NAT_BASE());
	blen = ivm_string_length(base);
	rbase = ivm_string_trimHead(base);

#define TOSPLIT(sth, tok_len) \
	for (tlen = (tok_len), beg = 0, i = 0; \
		 blen - i >= tlen;) { \
		sth; i++; \
	} \
	tmp = ivm_string_object_new_rl(NAT_STATE(), rbase + beg, blen - beg); \
	ivm_list_object_push(ret, NAT_STATE(), tmp);

#define SPLIT() \
	tmp = ivm_string_object_new_rl(NAT_STATE(), rbase + beg, i - beg); \
	ivm_list_object_push(ret, NAT_STATE(), tmp); \
	beg = i + tlen; \
	i += tlen;

#define SPLITIF(cond, dob) \
	if (cond) { \
		SPLIT(); \
		dob; \
		continue; \
	}

	switch (NAT_ARGC()) {
		case 0:
		/*
			ret = IVM_AS(ivm_list_object_new(NAT_STATE()), ivm_list_object_t);
			
			TOSPLIT(
				SPLITIF(rbase[i] == ' ' ||
						rbase[i] == '\t' ||
						rbase[i] == '\r' ||
						rbase[i] == '\n',
					{
						do {
							i++;
						} while (i < blen &&
								 (rbase[i] == ' ' ||
								  rbase[i] == '\t' ||
								  rbase[i] == '\r' ||
								  rbase[i] == '\n'));
						if (i >= blen)
							beg = i - 1;
						else
							beg = i;
					}
				),
				1
			);

			return IVM_AS_OBJ(ret);
		*/
			RTM_FATAL(IVM_ERROR_MSG_TODO_ERROR);

		case 1:
			MATCH_ARG("s", &tok);

			tlen = ivm_string_length(tok);
			rtok = ivm_string_trimHead(tok);

			RTM_ASSERT(tlen, IVM_ERROR_MSG_EMPTY_SEPARATOR);

			ret = IVM_AS(ivm_list_object_new(NAT_STATE()), ivm_list_object_t);
			
			TOSPLIT(
				SPLITIF(!IVM_STRNCMP(rbase + i, tlen, rtok, tlen), 0),
				tlen
			);

			return IVM_AS_OBJ(ret);
	}

#undef TOSPLIT
#undef SPLIT
#undef SPLITIF

	RTM_FATAL(IVM_ERROR_MSG_TODO_ERROR);

	return IVM_NONE(NAT_STATE());
}
