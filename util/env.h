#ifndef _IVM_UTIL_ENV_H_
#define _IVM_UTIL_ENV_H_

#include "pub/com.h"
#include "pub/vm.h"

IVM_COM_HEADER

typedef struct {
	ivm_string_pool_t *str_pool;
	ivm_exec_list_t *exec_list;
} ivm_gen_env_t;

int
ivm_env_init();

IVM_COM_END

#endif
