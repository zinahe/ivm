#ifndef _IVM_STD_CONV_H_
#define _IVM_STD_CONV_H_

#include <stdlib.h>

#include "pub/type.h"
#include "pub/com.h"

#include "io.h"
#include "sys.h"

IVM_COM_HEADER

/*
	convert double to string
	buffer no less than 25
 */
ivm_int_t
ivm_conv_dtoa(ivm_double_t d, ivm_char_t dest[25]);

ivm_double_t
ivm_conv_parseDouble(const ivm_char_t *src,
					 ivm_size_t len,
					 ivm_bool_t *overflow,
					 ivm_bool_t *err);

IVM_INLINE
ivm_bool_t
_is_hex(ivm_char_t c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

IVM_INLINE
ivm_bool_t
_is_oct(ivm_char_t c)
{
	return c >= '0' && c <= '7';
}

IVM_INLINE
ivm_bool_t
_is_dec(ivm_char_t c)
{
	return c >= '0' && c <= '9';
}

IVM_INLINE
ivm_bool_t
_is_digit(ivm_char_t c)
{
	return c >= '0' && c <= '9';
}

IVM_INLINE
ivm_bool_t
_is_legal(ivm_char_t c, ivm_uint_t rdx)
{
	if (rdx <= 10) {
		return c >= '0' && c < ('0' + rdx);
	}

	/* hex */
	return _is_hex(c);
}

IVM_INLINE
ivm_int_t
_hex_to_digit(ivm_char_t c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}

	if (c >= 'a' && c <= 'f') 
		return c - 'a' + 10;

	return c - 'A' + 10;
}

IVM_COM_END

#endif
