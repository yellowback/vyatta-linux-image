/*******************************************************************************
Copyright (C) Marvell Interclsional Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
Interclsional Ltd. and/or its affiliates ("Marvell") under the following
alterclsive licensing terms.  Once you have made an eleclsion to distribute the
File under one of the following license alterclsives, please (i) delete this
introduclsory statement regarding license alterclsives, (ii) delete the two
license alterclsives that you have not eleclsed to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "mvOs.h"
#include "gbe/mvNeta.h"
#include "nfp/mvNfp.h"

#include "mv_nfp_mgr.h"
#include "nfp_sysfs.h"
#include "net_dev/mv_netdev.h"

static struct kobject *exact_cls_kobj = NULL;

static ssize_t exact_cls_help(struct device *dev, struct device_attribute *attr, char *buf)
{
	int off = 0;
	off += mvOsSPrintf(buf+off, "cat                      help                 - Print this help.\n");
	off += mvOsSPrintf(buf+off, "cat                      print_policy         - Print exact classification policy.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3]   > dscp_policy_set      - Set exact classification  policy for DSCP.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3]   > vlan_prio_policy_set - Set exact classification policy for VLAN Priority.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3]   > txq_policy_set       - Set exact classification policy for TXQ.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3]   > txp_policy_set       - Set exact classification policy for TXP.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3]   > mh_policy_set        - Set exact classification policy for MH.\n");

	off += mvOsSPrintf(buf+off, "\n\nPolicy: 0 = Highest , 1 = Lowest , 2 = First , 3 = Last.\n");
	return off;
}


static ssize_t exact_cls_policy_set(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int res = 0, err = 0, policy;
	const char *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	res = sscanf(buf, "%d", &policy);
	if (res < 1)
		goto cls_err;

	if (policy > MV_NFP_CLASSIFY_POLICY_LAST || policy < MV_NFP_CLASSIFY_POLICY_HIGHEST)
		goto cls_err;

	if (!strcmp(name, "dscp_policy_set"))
		nfp_classify_exact_policy_set(MV_NFP_CLASSIFY_FEATURE_DSCP, policy);
	else if (!strcmp(name, "vlan_prio_policy_set"))
		nfp_classify_exact_policy_set(MV_NFP_CLASSIFY_FEATURE_VPRIO, policy);
	else if (!strcmp(name, "txq_policy_set"))
		nfp_classify_exact_policy_set(MV_NFP_CLASSIFY_FEATURE_TXQ, policy);
	else if (!strcmp(name, "txp_policy_set"))
		nfp_classify_exact_policy_set(MV_NFP_CLASSIFY_FEATURE_TXP, policy);
	else if (!strcmp(name, "mh_policy_set"))
		nfp_classify_exact_policy_set(MV_NFP_CLASSIFY_FEATURE_MH, policy);

cls_out:
	return err ? -EINVAL : len;
cls_err:
	printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	err = 1;
	goto cls_out;
}



static ssize_t exact_cls_print_policy(struct device *dev, struct device_attribute *attr, char *buf)
{
	const char *policy_type[] = {"Highest", "Lowest", "First", "Last", "Invalid", "Invalid", "Invalid"};
	MV_NFP_CLASSIFY_MODE policy = MV_NFP_CLASSIFY_MODE_HIGHEST;
	int off = 0;

	policy = nfp_classify_exact_policy_get(MV_NFP_CLASSIFY_FEATURE_DSCP);
	off += mvOsSPrintf(buf+off, "dscp_exact_policy		= %s.\n", policy_type[policy]);

	policy = nfp_classify_exact_policy_get(MV_NFP_CLASSIFY_FEATURE_VPRIO);
	off += mvOsSPrintf(buf+off, "vlan_prio_exact_policy		= %s.\n", policy_type[policy]);

	policy = nfp_classify_exact_policy_get(MV_NFP_CLASSIFY_FEATURE_TXQ);
	off += mvOsSPrintf(buf+off, "txq_exact_policy		= %s.\n", policy_type[policy]);

	policy = nfp_classify_exact_policy_get(MV_NFP_CLASSIFY_FEATURE_TXP);
	off += mvOsSPrintf(buf+off, "txp_exact_policy		= %s.\n", policy_type[policy]);

	policy = nfp_classify_exact_policy_get(MV_NFP_CLASSIFY_FEATURE_MH);
	off += mvOsSPrintf(buf+off, "mh_exact_policy			= %s.\n", policy_type[policy]);
	return off;
}


static DEVICE_ATTR(dscp_policy_set, S_IWUSR, NULL, exact_cls_policy_set);
static DEVICE_ATTR(vlan_prio_policy_set, S_IWUSR, NULL, exact_cls_policy_set);
static DEVICE_ATTR(txq_policy_set, S_IWUSR, NULL, exact_cls_policy_set);
static DEVICE_ATTR(txp_policy_set, S_IWUSR, NULL, exact_cls_policy_set);
static DEVICE_ATTR(mh_policy_set, S_IWUSR, NULL, exact_cls_policy_set);
static DEVICE_ATTR(print_policy, S_IRUSR, exact_cls_print_policy, NULL);
static DEVICE_ATTR(help, S_IRUSR, exact_cls_help, NULL);

static struct attribute *nfp_exact_cls_attrs[] = {
	&dev_attr_dscp_policy_set.attr,
	&dev_attr_vlan_prio_policy_set.attr,
	&dev_attr_txq_policy_set.attr,
	&dev_attr_txp_policy_set.attr,
	&dev_attr_mh_policy_set.attr,
	&dev_attr_print_policy.attr,
	&dev_attr_help.attr,
	NULL
};

static struct attribute_group nfp_exact_cls_group = {
	.attrs = nfp_exact_cls_attrs,
};


int __init nfp_exact_cls_sysfs_init(struct kobject *cls_kobj)
{
		int err;

		exact_cls_kobj = kobject_create_and_add("exact", cls_kobj);
		if (!exact_cls_kobj) {
			printk(KERN_INFO "could not create classification kobjects \n");
			return -ENOMEM;
		}

		err = sysfs_create_group(exact_cls_kobj, &nfp_exact_cls_group);
		if (err) {
			printk(KERN_INFO "exact classification sysfs group failed %d\n", err);
			goto out;
		}
out:
		return err;
}

void nfp_exact_cls_sysfs_exit(void)
{
	remove_group_kobj_put(exact_cls_kobj, &nfp_exact_cls_group);
}
