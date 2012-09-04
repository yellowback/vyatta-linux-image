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

static struct kobject *prio_cls_kobj = NULL;

static ssize_t prio_cls_help(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int off = 0;
	off += mvOsSPrintf(buf+off, "cat                       help                   - Print this help.\n");
	off += mvOsSPrintf(buf+off, "cat                       prio_policy_get        - Print priority classification policy.\n");
	off += mvOsSPrintf(buf+off, "echo [0 | 1 | 2 | 3]    > prio_policy_set        - Set priority classification policy.\n");
	off += mvOsSPrintf(buf+off, "echo iif prio           > iif_to_prio            - Map interface to priority\n");
	off += mvOsSPrintf(buf+off, "echo iif vlan_prio prio > iif_vlan_prio_to_prio  - Map interface + Vlan Priority to priority\n");
	off += mvOsSPrintf(buf+off, "echo iif dscp prio      > iif_dscp_to_prio       - Map interface + DSCP to priority\n");
	off += mvOsSPrintf(buf+off, "echo oif prio mh        > prio_to_mh             - Map interface + priority to MH\n");
	off += mvOsSPrintf(buf+off, "echo oif prio txq       > prio_to_txq            - Map interface + priority to TXQ\n");
	off += mvOsSPrintf(buf+off, "echo oif prio txp       > prio_to_txp            - Map interface + priority to TXP\n");
	off += mvOsSPrintf(buf+off, "echo oif prio vlan_prio > prio_to_vlan_prio      - Map interface + priority to Vlan Priority\n");
	off += mvOsSPrintf(buf+off, "echo oif prio dscp      > prio_to_dscp           - Map interface + priority to DSCP\n");
	off += mvOsSPrintf(buf+off, "echo iif                > ingress_prio_dump      - Show interface ingress priority classification data\n");
	off += mvOsSPrintf(buf+off, "echo oif                > egress_prio_dump       - Show interface egress priority classification data\n");

	off += mvOsSPrintf(buf+off, "\n\nPolicy: 0 = Highest , 1 = Lowest.\n");
	return off;
}


static ssize_t prio_cls_policy_set(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int res = 0, err = 0, policy;
	const char *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	res = sscanf(buf, "%d", &policy);
	if (res < 1)
		goto cls_err;

	if (policy > MV_NFP_CLASSIFY_POLICY_LOWEST || policy < MV_NFP_CLASSIFY_POLICY_HIGHEST)
		goto cls_err;

	if (!strcmp(name, "prio_policy_set"))
		nfp_classify_prio_policy_set(policy);

cls_out:
	return err ? -EINVAL : len;
cls_err:
	printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	err = 1;
	goto cls_out;
}

static ssize_t prio_cls_policy_get(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int off = 0;
	MV_NFP_CLASSIFY_POLICY policy = MV_NFP_CLASSIFY_POLICY_HIGHEST;
	const char *name = attr->attr.name, *str;

	if (!strcmp(name, "prio_policy_get"))
		policy = nfp_classify_prio_policy_get();

	switch (policy) {
		case MV_NFP_CLASSIFY_POLICY_HIGHEST:
			str = "Highest";
			break;
		case MV_NFP_CLASSIFY_POLICY_LOWEST:
			str = "Lowest";
			break;
		default:
			str = "Invalid";
			break;
	}

	off += mvOsSPrintf(buf+off, "%s: %s\n", name, str);
	return off;
}

static ssize_t prio_cls_classification_set(struct device *dev,
			 struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned int err = 0, if_index,a,b;
	const char  *name = attr->attr.name;
	char if_name[NAME_SIZE];
	struct net_device *netdev;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	memset(if_name, 0, NAME_SIZE);

	sscanf(buf, "%s %d %d", if_name, &a, &b);

	netdev = dev_get_by_name(&init_net, if_name);
	if (netdev == NULL) {
		return 1;
	}
	if_index = netdev->ifindex;
	dev_put(netdev);

	if (!strcmp(name, "iif_to_prio")) {
		nfp_iif_to_prio_set(if_index, a);
	} else if (!strcmp(name, "iif_vlan_prio_to_prio")) {
		nfp_iif_vlan_prio_to_prio_set(if_index, a, b);
	} else if (!strcmp(name, "iif_dscp_to_prio")) {
		nfp_iif_dscp_to_prio_set(if_index, a, b);
	} else if (!strcmp(name, "prio_to_mh")) {
		nfp_prio_to_mh_set(if_index, a, b);
	} else if (!strcmp(name, "prio_to_txq")) {
		nfp_prio_to_txq_set(if_index, a, b);
	} else if (!strcmp(name, "prio_to_txp")) {
		nfp_prio_to_txp_set(if_index, a, b);
	} else if (!strcmp(name, "prio_to_vlan_prio")) {
		nfp_prio_to_vlan_prio_set(if_index, a, b);
	} else if (!strcmp(name, "prio_to_dscp")) {
		nfp_prio_to_dscp_set(if_index, a, b);
	} else if (!strcmp(name, "ingress_prio_dump")) {
		nfp_ingress_prio_dump(if_index);
	} else if (!strcmp(name, "egress_prio_dump")) {
		nfp_egress_prio_dump(if_index);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(iif_to_prio,		S_IWUSR, NULL, prio_cls_classification_set);
static DEVICE_ATTR(iif_vlan_prio_to_prio,   S_IWUSR, NULL, prio_cls_classification_set);
static DEVICE_ATTR(iif_dscp_to_prio,	S_IWUSR, NULL, prio_cls_classification_set);

static DEVICE_ATTR(prio_to_dscp,		S_IWUSR, NULL, prio_cls_classification_set);
static DEVICE_ATTR(prio_to_vlan_prio,	S_IWUSR, NULL, prio_cls_classification_set);
static DEVICE_ATTR(prio_to_txp,		S_IWUSR, NULL, prio_cls_classification_set);
static DEVICE_ATTR(prio_to_txq,		S_IWUSR, NULL, prio_cls_classification_set);
static DEVICE_ATTR(prio_to_mh,		S_IWUSR, NULL, prio_cls_classification_set);

static DEVICE_ATTR(ingress_prio_dump,		S_IWUSR, NULL, prio_cls_classification_set);
static DEVICE_ATTR(egress_prio_dump, 		S_IWUSR, NULL, prio_cls_classification_set);



static DEVICE_ATTR(prio_policy_set, S_IWUSR, NULL, prio_cls_policy_set);
static DEVICE_ATTR(prio_policy_get, S_IRUSR, prio_cls_policy_get, NULL);
static DEVICE_ATTR(help, S_IRUSR, prio_cls_help, NULL);

static struct attribute *nfp_prio_cls_attrs[] = {
	&dev_attr_prio_policy_set.attr,
	&dev_attr_prio_policy_get.attr,
	&dev_attr_help.attr,
	&dev_attr_iif_to_prio.attr,
	&dev_attr_iif_vlan_prio_to_prio.attr,
	&dev_attr_iif_dscp_to_prio.attr,
	&dev_attr_prio_to_dscp.attr,
	&dev_attr_prio_to_vlan_prio.attr,
	&dev_attr_prio_to_txp.attr,
	&dev_attr_prio_to_txq.attr,
	&dev_attr_prio_to_mh.attr,
	&dev_attr_ingress_prio_dump.attr,
	&dev_attr_egress_prio_dump.attr,
	NULL
};

static struct attribute_group nfp_prio_cls_group = {
	.attrs = nfp_prio_cls_attrs,
};


int __init nfp_prio_cls_sysfs_init(struct kobject *nfp_kobj)
{
		int err;

		prio_cls_kobj = kobject_create_and_add("prio", nfp_kobj);
		if (!prio_cls_kobj) {
			printk(KERN_INFO "could not create classification kobjects \n");
			return -ENOMEM;
		}

		err = sysfs_create_group(prio_cls_kobj, &nfp_prio_cls_group);
		if (err) {
			printk(KERN_INFO "priority classification sysfs group failed %d\n", err);
			goto out;
		}
out:
		return err;
}

void nfp_prio_cls_sysfs_exit(void)
{
	remove_group_kobj_put(prio_cls_kobj, &nfp_prio_cls_group);
}
