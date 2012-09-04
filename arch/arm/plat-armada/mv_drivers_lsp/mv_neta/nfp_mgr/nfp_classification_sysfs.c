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

static struct kobject *cls_kobj = NULL;

static ssize_t cls_help(struct device *dev, struct device_attribute *attr, char *buf)
{
	int off = 0;
	off += mvOsSPrintf(buf+off, "cat                        help                   - Print this help.\n");
	off += mvOsSPrintf(buf+off, "cat                        print_policy           - Print classification policy.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3 | 4]   > dscp_mode_set        - Set classify mode for DSCP.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3 | 4]   > vlan_prio_mode_set   - Set classify mode for VLAN Priority.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3 | 4]   > txq_mode_set         - Set classify mode for TXQ.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3 | 4]   > txp_mode_set         - Set classify mode for TXP.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3 | 4]   > mh_mode_set          - Set classify mode for MH.\n");
	off += mvOsSPrintf(buf+off, "\n\nMode: 0 = Disabled , 1 = Exact , 2 = Priority , 3 = Highest , 4 = Lowest .\n");
	return off;
}


static ssize_t cls_print_policy(struct device *dev, struct device_attribute *attr, char *buf)
{
	const char *mode_type[] = {"Disabled", "Exact", "Priority", "Highest", "Lowest", "Invalid"};
	MV_NFP_CLASSIFY_MODE mode = MV_NFP_CLASSIFY_MODE_DISABLED;
	int off = 0;

	mode = mvNfpClassifyModeGet(MV_NFP_CLASSIFY_FEATURE_DSCP);
	off += mvOsSPrintf(buf+off, "dscp_mode		= %s.\n", mode_type[mode]);

	mode = mvNfpClassifyModeGet(MV_NFP_CLASSIFY_FEATURE_VPRIO);
	off += mvOsSPrintf(buf+off, "vlan_prio_mode		= %s.\n", mode_type[mode]);

	mode = mvNfpClassifyModeGet(MV_NFP_CLASSIFY_FEATURE_TXQ);
	off += mvOsSPrintf(buf+off, "txq_mode		= %s.\n", mode_type[mode]);

	mode = mvNfpClassifyModeGet(MV_NFP_CLASSIFY_FEATURE_TXP);
	off += mvOsSPrintf(buf+off, "txp_mode		= %s.\n", mode_type[mode]);

	mode = mvNfpClassifyModeGet(MV_NFP_CLASSIFY_FEATURE_MH);
	off += mvOsSPrintf(buf+off, "mh_mode			= %s.\n", mode_type[mode]);
	return off;
}


static ssize_t cls_mode_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int res = 0, err = 0, mode;
	const char *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	res = sscanf(buf, "%d", &mode);
	if (res < 1)
		goto cls_err;

	if (mode > MV_NFP_CLASSIFY_MODE_LOWEST || mode < MV_NFP_CLASSIFY_MODE_DISABLED)
		goto cls_err;

	if (!strcmp(name, "dscp_mode_set"))
		nfp_classify_mode_set(MV_NFP_CLASSIFY_FEATURE_DSCP, mode);
	else if (!strcmp(name, "vlan_prio_mode_set"))
		nfp_classify_mode_set(MV_NFP_CLASSIFY_FEATURE_VPRIO, mode);
	else if (!strcmp(name, "txq_mode_set"))
		nfp_classify_mode_set(MV_NFP_CLASSIFY_FEATURE_TXQ, mode);
	else if (!strcmp(name, "txp_mode_set"))
		nfp_classify_mode_set(MV_NFP_CLASSIFY_FEATURE_TXP, mode);
	else if (!strcmp(name, "mh_mode_set"))
		nfp_classify_mode_set(MV_NFP_CLASSIFY_FEATURE_MH, mode);

cls_out:
	return err ? -EINVAL : len;
cls_err:
	printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	err = 1;
	goto cls_out;
}


static DEVICE_ATTR(dscp_mode_set, S_IWUSR, NULL, cls_mode_set);
static DEVICE_ATTR(vlan_prio_mode_set, S_IWUSR, NULL, cls_mode_set);
static DEVICE_ATTR(txq_mode_set, S_IWUSR, NULL, cls_mode_set);
static DEVICE_ATTR(txp_mode_set, S_IWUSR, NULL, cls_mode_set);
static DEVICE_ATTR(mh_mode_set, S_IWUSR, NULL, cls_mode_set);
static DEVICE_ATTR(print_policy, S_IRUSR, cls_print_policy, NULL);
static DEVICE_ATTR(help, S_IRUSR, cls_help, NULL);

static struct attribute *nfp_cls_attrs[] = {
	&dev_attr_dscp_mode_set.attr,
	&dev_attr_vlan_prio_mode_set.attr,
	&dev_attr_txq_mode_set.attr,
	&dev_attr_txp_mode_set.attr,
	&dev_attr_mh_mode_set.attr,
	&dev_attr_print_policy.attr,
	&dev_attr_help.attr,
	NULL
};
static struct attribute_group nfp_cls_group = {
	.attrs = nfp_cls_attrs,
};

int __init nfp_cls_sysfs_init(struct kobject *nfp_kobj)
{
		int err;

		cls_kobj = kobject_create_and_add("classify", nfp_kobj);
		if (!cls_kobj) {
			printk(KERN_INFO "could not create classify kobjects \n");
			return -ENOMEM;
		}

		err = sysfs_create_group(cls_kobj, &nfp_cls_group);
		if (err) {
			printk(KERN_INFO "classify sysfs group failed %d\n", err);
			goto out;
		}

		nfp_exact_cls_sysfs_init(cls_kobj);
		nfp_prio_cls_sysfs_init(cls_kobj);
out:
		return err;
}

void nfp_cls_sysfs_exit(void)
{
	nfp_exact_cls_sysfs_exit();
	nfp_prio_cls_sysfs_exit();
	remove_group_kobj_put(cls_kobj, &nfp_cls_group);
}
