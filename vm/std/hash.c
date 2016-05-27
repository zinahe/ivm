#include "pub/com.h"
#include "pub/mem.h"
#include "hash.h"
#include "err.h"

ivm_hash_table_t *
ivm_hash_table_new(ivm_size_t tsize,
				   ivm_hash_table_comparer_t cmp,
				   ivm_hash_function_t hash)
{
	ivm_hash_table_t *ret = MEM_ALLOC(sizeof(*ret),
									  ivm_hash_table_t *);

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("hash table"));
	IVM_ASSERT(tsize > 1, IVM_ERROR_MSG_TOO_SMALL_VALUE_FOR("hash table init size", tsize));

	ret->tsize = tsize;
	ret->table = MEM_ALLOC_INIT(sizeof(*ret->table) * tsize,
								ivm_ptpair_t *);

	IVM_ASSERT(ret->table, IVM_ERROR_MSG_FAILED_ALLOC_NEW("hash table data"));

	ret->cmp = cmp;
	ret->hash = hash;

	IVM_ASSERT(ret->cmp && ret->hash,
			   IVM_ERROR_MSG_NULL_PTR("hash comparer or hash function"));

	return ret;
}

void
ivm_hash_table_free(ivm_hash_table_t *table)
{
	if (table) {
		MEM_FREE(table->table);
		MEM_FREE(table);
	}

	return;
}

#define IS_EMPTY_SLOT(pair) (!(pair)->k)

IVM_PRIVATE
void
ivm_hash_table_expand(ivm_hash_table_t *table) /* includes rehashing */
{
	ivm_size_t osize = table->tsize,
			   dsize = osize << 1; /* dest size */
	ivm_ptpair_t *otable = table->table;
	ivm_size_t i;

	table->table = MEM_ALLOC_INIT(sizeof(*table->table) * dsize,
								  ivm_ptpair_t *);

	IVM_ASSERT(table->table, IVM_ERROR_MSG_FAILED_ALLOC_NEW("expanded hash table data"));

	table->tsize = dsize;

	for (i = 0; i < osize; i++) {
		if (otable[i].k != IVM_NULL)
			ivm_hash_table_insert(table, otable[i].k, otable[i].v);
	}

	MEM_FREE(otable);

	return;
}

void
ivm_hash_table_insert(ivm_hash_table_t *table,
					  void *key, void *value)
{
	ivm_hash_val_t hash = table->hash(key);
	ivm_size_t size;
	ivm_uint_t h1, h2;
	ivm_uint_t i, j;

	ivm_ptpair_t *tmp;

	while (1) {
		size = table->tsize;
		h1 = hash % size;
		h2 = 1 + hash % (size - 1);

		for (i = h1, j = 0;
			 j < size;
			 i += h2, j++) {
			tmp = &table->table[i % size];
			if (IS_EMPTY_SLOT(tmp)) {
				tmp->k = key;
				tmp->v = value;
				goto END;
			} else if (table->cmp(key, tmp->k) == 0) {
				tmp->v = value;
				goto END;
			}
		}

		/* allocate new space */
		ivm_hash_table_expand(table);
	}

END:

	return;
}

#define SET_SUCCESS(flag) ((flag) ? *(flag) = IVM_TRUE : IVM_TRUE)
#define SET_FAILED(flag) ((flag) ? *(flag) = IVM_FALSE : IVM_FALSE)

void *
ivm_hash_table_getValue(ivm_hash_table_t *table,
						void *key, ivm_bool_t *suc)
{
	ivm_hash_val_t hash = table->hash(key);
	ivm_size_t size;
	ivm_uint_t h1, h2;
	ivm_uint_t i, j;
	void *ret = IVM_NULL;

	ivm_ptpair_t *tmp;

	size = table->tsize;
	h1 = hash % size;
	h2 = 1 + hash % (size - 1);

	for (i = h1, j = 0;
		 j < size;
		 i += h2, j++) {
		tmp = &table->table[i % size];
		if (IS_EMPTY_SLOT(tmp)) {
			SET_FAILED(suc);
			goto END;
		} else if (table->cmp(key, tmp->k) == 0) {
			SET_SUCCESS(suc);
			ret = tmp->v;
			goto END;
		}
	}

	SET_FAILED(suc);

END:

	return ret;
}

#if 0

IVM_PRIVATE
ivm_ptpair_t * /* NULL value means new space were allocated */
ivm_hash_table_getOverflow(ivm_hash_table_t *table,
						   ivm_size_t i)
{
	ivm_size_t orig;

	if (i >= table->osize) {
		orig = table->osize;
		table->osize = i + 1;
		table->otable = MEM_REALLOC(table->otable,
									sizeof(*table->otable) * table->osize,
									ivm_ptpair_t *);
		MEM_INIT(&table->otable[orig],
				 sizeof(*table->otable)
				 * (table->osize - orig));
	}

	return &table->otable[i];
}

void
ivm_hash_table_setMap(ivm_hash_table_t *table,
					  void *key, void *value)
{
	ivm_hash_val_t hash = table->hash(key);
	ivm_uint_t i = hash % table->bsize;
	ivm_ptpair_t *tmp = &table->base[i];
	ivm_size_t j;

	IVM_ASSERT(key, IVM_ERROR_MSG_FAILED_ALLOC_NEW("key"));

	if (IS_EMPTY_SLOT(tmp)) {
		tmp->k = key;
		tmp->v = value;
	} else if (table->cmp(key, tmp->k) == 0) {
		tmp->v = value;
	} else {
		/* overflow */
		for (j = i; ; j += (table->bsize >> 2) + 1) {
			tmp = ivm_hash_table_getOverflow(table, j);
			if (IS_EMPTY_SLOT(tmp)) {
				tmp->k = key;
				tmp->v = value;
				break;
			} else if (table->cmp(key, tmp->k) == 0) {
				tmp->v = value;
				break;
			}
		}
	}

	return;
}

#define SET_SUCCESS(flag) ((flag) ? *(flag) = IVM_TRUE : IVM_TRUE)
#define SET_FAILED(flag) ((flag) ? *(flag) = IVM_FALSE : IVM_FALSE)

void *
ivm_hash_table_getValue(ivm_hash_table_t *table,
						void *key, ivm_bool_t *suc)
{
	ivm_hash_val_t hash = table->hash(key);
	ivm_uint_t i = hash % table->bsize;
	ivm_ptpair_t *tmp = &table->base[i];
	void *ret = IVM_NULL;
	ivm_size_t j;

	IVM_ASSERT(key, IVM_ERROR_MSG_FAILED_ALLOC_NEW("key"));

	if (IS_EMPTY_SLOT(tmp)) {
		SET_FAILED(suc);
	} else if (table->cmp(key, tmp->k) == 0) {
		ret = tmp->v;
		SET_SUCCESS(suc);
	} else {
		/* check overflow */
		for (j = i; ; j += (table->bsize >> 2) + 1) {
			tmp = ivm_hash_table_getOverflow(table, j);
			if (IS_EMPTY_SLOT(tmp)) {
				SET_FAILED(suc);
				break;
			} else if (table->cmp(key, tmp->k) == 0) {
				ret = tmp->v;
				SET_SUCCESS(suc);
				break;
			}
		}
	}

	return ret;
}

#endif