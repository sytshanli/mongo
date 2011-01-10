/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2010 WiredTiger, Inc.
 *	All rights reserved.
 *
 * $Id$
 */

#include "wt_internal.h"

/*
 * __wt_cache_create --
 *	Create the underlying cache.
 */
int
__wt_cache_create(ENV *env)
{
	IENV *ienv;
	WT_CACHE *cache;
	int ret;

	ienv = env->ienv;
	ret = 0;

	WT_VERBOSE(env, WT_VERB_CACHE, (env,
		"cache initialization: %lu MB", (u_long)env->cache_size));

	WT_RET(__wt_calloc(env, 1, sizeof(WT_CACHE), &ienv->cache));
	cache = ienv->cache;

	WT_ERR(__wt_mtx_alloc(env, "cache drain server", 1, &cache->mtx_drain));
	WT_ERR(__wt_mtx_alloc(env, "cache read server", 1, &cache->mtx_read));
	WT_ERR(__wt_mtx_alloc(env, "reconciliation", 0, &cache->mtx_reconcile));

	WT_ERR(__wt_stat_alloc_cache_stats(env, &cache->stats));

	WT_STAT_SET(
	    cache->stats, CACHE_BYTES_MAX, env->cache_size * WT_MEGABYTE);

	return (0);

err:	(void)__wt_cache_destroy(env);
	return (ret);
}

/*
 * __wt_cache_pages_inuse --
 *	Return the number of pages in use.
 */
inline uint64_t
__wt_cache_pages_inuse(WT_CACHE *cache)
{
	uint64_t pages_in, pages_out;

	/*
	 * Other threads of control may be modifying these fields -- we don't
	 * need exact values, but we do not want garbage, so read first, then
	 * use local variables for calculation, ensuring a reasonable return.
	 */
	pages_in = cache->stat_pages_in;
	pages_out = cache->stat_pages_out;
	return (pages_in > pages_out ? pages_in - pages_out : 0);
}

/*
 * __wt_cache_bytes_inuse --
 *	Return the number of bytes in use.
 */
inline uint64_t
__wt_cache_bytes_inuse(WT_CACHE *cache)
{
	uint64_t bytes_in, bytes_out;

	/*
	 * Other threads of control may be modifying these fields -- we don't
	 * need exact values, but we do not want garbage, so read first, then
	 * use local variables for calculation, ensuring a reasonable return.
	 */
	bytes_in = cache->stat_bytes_in;
	bytes_out = cache->stat_bytes_out;
	return (bytes_in > bytes_out ? bytes_in - bytes_out : 0);
}

/*
 * __wt_cache_stats --
 *	Update the cache statistics for return to the application.
 */
void
__wt_cache_stats(ENV *env)
{
	WT_CACHE *cache;
	WT_STATS *stats;

	cache = env->ienv->cache;
	stats = cache->stats;

	WT_STAT_SET(stats, CACHE_BYTES_INUSE, __wt_cache_bytes_inuse(cache));
	WT_STAT_SET(stats, CACHE_PAGES_INUSE, __wt_cache_pages_inuse(cache));
}

/*
 * __wt_cache_destroy --
 *	Discard the underlying cache.
 */
int
__wt_cache_destroy(ENV *env)
{
	IENV *ienv;
	WT_CACHE *cache;
	int ret;

	ienv = env->ienv;
	cache = ienv->cache;
	ret = 0;

	if (cache == NULL)
		return (0);

	/* Discard mutexes. */
	if (cache->mtx_drain != NULL)
		(void)__wt_mtx_destroy(env, cache->mtx_drain);
	if (cache->mtx_read != NULL)
		__wt_mtx_destroy(env, cache->mtx_read);
	if (cache->mtx_reconcile != NULL)
		__wt_mtx_destroy(env, cache->mtx_reconcile);

	/* Discard allocated memory, and clear. */
	__wt_free(env, cache->stats, 0);
	__wt_free(env, ienv->cache, sizeof(WT_CACHE));

	return (ret);
}
