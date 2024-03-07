/*
 * (C) 2012-2016 by Pablo Neira Ayuso <pablo@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>

#include <linux/netfilter/nf_tables.h>

#include "internal.h"
#include <libmnl/libmnl.h>
#include <libnftnl/object.h>

#include "obj.h"

static int nftnl_obj_quota_set(struct nftnl_obj *e, uint16_t type,
				const void *data, uint32_t data_len)
{
	struct nftnl_obj_quota *quota = nftnl_obj_data(e);

	switch (type) {
	case NFTNL_OBJ_QUOTA_BYTES:
		memcpy(&quota->bytes, data, sizeof(quota->bytes));
		break;
	case NFTNL_OBJ_QUOTA_CONSUMED:
		memcpy(&quota->consumed, data, sizeof(quota->consumed));
		break;
	case NFTNL_OBJ_QUOTA_FLAGS:
		memcpy(&quota->flags, data, sizeof(quota->flags));
		break;
	}
	return 0;
}

static const void *nftnl_obj_quota_get(const struct nftnl_obj *e,
					uint16_t type, uint32_t *data_len)
{
	struct nftnl_obj_quota *quota = nftnl_obj_data(e);

	switch (type) {
	case NFTNL_OBJ_QUOTA_BYTES:
		*data_len = sizeof(quota->bytes);
		return &quota->bytes;
	case NFTNL_OBJ_QUOTA_CONSUMED:
		*data_len = sizeof(quota->consumed);
		return &quota->consumed;
	case NFTNL_OBJ_QUOTA_FLAGS:
		*data_len = sizeof(quota->flags);
		return &quota->flags;
	}
	return NULL;
}

static int nftnl_obj_quota_cb(const struct nlattr *attr, void *data)
{
	int type = mnl_attr_get_type(attr);
	const struct nlattr **tb = data;

	if (mnl_attr_type_valid(attr, NFTA_QUOTA_MAX) < 0)
		return MNL_CB_OK;

	switch(type) {
	case NFTA_QUOTA_BYTES:
	case NFTA_QUOTA_CONSUMED:
		if (mnl_attr_validate(attr, MNL_TYPE_U64) < 0)
			abi_breakage();
		break;
	case NFTA_QUOTA_FLAGS:
		if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
			abi_breakage();
		break;
	}

	tb[type] = attr;
	return MNL_CB_OK;
}

static void
nftnl_obj_quota_build(struct nlmsghdr *nlh, const struct nftnl_obj *e)
{
	struct nftnl_obj_quota *quota = nftnl_obj_data(e);

	if (e->flags & (1 << NFTNL_OBJ_QUOTA_BYTES))
		mnl_attr_put_u64(nlh, NFTA_QUOTA_BYTES, htobe64(quota->bytes));
	if (e->flags & (1 << NFTNL_OBJ_QUOTA_CONSUMED))
		mnl_attr_put_u64(nlh, NFTA_QUOTA_CONSUMED,
				 htobe64(quota->consumed));
	if (e->flags & (1 << NFTNL_OBJ_QUOTA_FLAGS))
		mnl_attr_put_u32(nlh, NFTA_QUOTA_FLAGS, htonl(quota->flags));
}

static int
nftnl_obj_quota_parse(struct nftnl_obj *e, struct nlattr *attr)
{
	struct nftnl_obj_quota *quota = nftnl_obj_data(e);
	struct nlattr *tb[NFTA_QUOTA_MAX + 1] = {};

	if (mnl_attr_parse_nested(attr, nftnl_obj_quota_cb, tb) < 0)
		return -1;

	if (tb[NFTA_QUOTA_BYTES]) {
		quota->bytes = be64toh(mnl_attr_get_u64(tb[NFTA_QUOTA_BYTES]));
		e->flags |= (1 << NFTNL_OBJ_QUOTA_BYTES);
	}
	if (tb[NFTA_QUOTA_CONSUMED]) {
		quota->consumed =
			be64toh(mnl_attr_get_u64(tb[NFTA_QUOTA_CONSUMED]));
		e->flags |= (1 << NFTNL_OBJ_QUOTA_CONSUMED);
	}
	if (tb[NFTA_QUOTA_FLAGS]) {
		quota->flags = ntohl(mnl_attr_get_u32(tb[NFTA_QUOTA_FLAGS]));
		e->flags |= (1 << NFTNL_OBJ_QUOTA_FLAGS);
	}

	return 0;
}

static int nftnl_obj_quota_snprintf(char *buf, size_t len,
				       uint32_t flags,
				       const struct nftnl_obj *e)
{
	struct nftnl_obj_quota *quota = nftnl_obj_data(e);

	return snprintf(buf, len, "bytes %"PRIu64" flags %u ",
			quota->bytes, quota->flags);
}

struct obj_ops obj_ops_quota = {
	.name		= "quota",
	.type		= NFT_OBJECT_QUOTA,
	.alloc_len	= sizeof(struct nftnl_obj_quota),
	.nftnl_max_attr	= __NFTNL_OBJ_QUOTA_MAX - 1,
	.set		= nftnl_obj_quota_set,
	.get		= nftnl_obj_quota_get,
	.parse		= nftnl_obj_quota_parse,
	.build		= nftnl_obj_quota_build,
	.output		= nftnl_obj_quota_snprintf,
};
