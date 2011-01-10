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
 * __wt_db_col_get --
 *	Db.col_get method.
 */
int
__wt_db_col_get(WT_TOC *toc, uint64_t recno, DBT *data)
{
	DB *db;
	IDB *idb;
	int ret;

	db = toc->db;
	idb = db->idb;

	/* Search the column store for the key. */
	if (!F_ISSET(idb, WT_COLUMN)) {
		__wt_api_db_errx(db,
		    "row database records cannot be retrieved by record "
		    "number");
		return (WT_ERROR);
	}

	WT_ERR(__wt_bt_search_col(toc, recno, WT_NOLEVEL, 0));
	ret = __wt_bt_dbt_return(toc, NULL, data, 0);

err:	if (toc->srch_page != idb->root_page.page)
		__wt_hazard_clear(toc, toc->srch_page);
	return (ret);
}
