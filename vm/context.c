#include "pub/mem.h"
#include "pub/com.h"
#include "pub/vm.h"
#include "pub/err.h"
#include "pub/inlines.h"

#include "std/ref.h"

#include "context.h"

#define GET_CONTEXT(chain_sub) ((chain_sub)->slots)

ivm_ctchain_t *
ivm_ctchain_new(ivm_vmstate_t *state, ivm_int_t len)
{
	ivm_ctchain_t *ret = ivm_vmstate_allocContext(state, len);

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("context chain"));

	return ret;
}

ivm_object_t *
ivm_ctchain_search(ivm_ctchain_t *chain,
				   ivm_vmstate_t *state,
				   const ivm_string_t *key)
{
	ivm_object_t *ret = IVM_NULL;
	ivm_context_t *i, *end;

	for (i = ivm_ctchain_contextStart(chain),
		 end = i + chain->len;
		 i != end; i++) {
		ret = ivm_context_getSlot(i, state, key);
		if (ret) break;
	}

	return ret;
}

ivm_bool_t
ivm_ctchain_setExistSlot(ivm_ctchain_t *chain,
						 struct ivm_vmstate_t_tag *state,
						 const ivm_string_t *key,
						 ivm_object_t *val)
{
	ivm_bool_t ret = IVM_FALSE;
	ivm_context_t *i, *end;

	for (i = ivm_ctchain_contextStart(chain),
		 end = i + chain->len;
		 i != end; i++) {
		ret = ivm_context_setExistSlot(i, state, key, val);
		if (ret) break;
	}

	return ret;
}

ivm_context_pool_t *
ivm_context_pool_new(ivm_size_t ecount)
{
	ivm_context_pool_t *ret = MEM_ALLOC(sizeof(*ret),
										ivm_context_pool_t *);
	ivm_ptpool_t **i, **end;
	ivm_int_t len = 0;

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("context pool"));

	for (i = ret->pools,
		 end = i + IVM_CONTEXT_POOL_MAX_CACHE_LEN;
		 i != end; i++, len++) {
		*i = ivm_ptpool_new(ecount, ivm_ctchain_getSize(len));
	}

	return ret;
}

void
ivm_context_pool_free(ivm_context_pool_t *pool)
{
	ivm_ptpool_t **i, **end;

	if (pool) {
		for (i = pool->pools,
			 end = i + IVM_CONTEXT_POOL_MAX_CACHE_LEN;
			 i != end; i++) {
			ivm_ptpool_free(*i);
		}

		MEM_FREE(pool);
	}

	return;
}

void
ivm_context_pool_init(ivm_context_pool_t *pool,
					  ivm_size_t ecount)
{
	ivm_ptpool_t **i, **end;
	ivm_int_t len = 0;

	for (i = pool->pools,
		 end = i + IVM_CONTEXT_POOL_MAX_CACHE_LEN;
		 i != end; i++, len++) {
		*i = ivm_ptpool_new(ecount, ivm_ctchain_getSize(len));
	}

	return;
}

void
ivm_context_pool_destruct(ivm_context_pool_t *pool)
{
	ivm_ptpool_t **i, **end;

	if (pool) {
		for (i = pool->pools,
			 end = i + IVM_CONTEXT_POOL_MAX_CACHE_LEN;
			 i != end; i++) {
			ivm_ptpool_free(*i);
		}
	}

	return;
}
