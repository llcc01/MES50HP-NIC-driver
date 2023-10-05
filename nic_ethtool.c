// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 1999 - 2006 Intel Corporation. */

/*
for nic
2023 lc
*/

/* ethtool support for nic */

#include "nic.h"
#include <linux/jiffies.h>
#include <linux/uaccess.h>


static const struct ethtool_ops nic_ethtool_ops = {
	// .supported_coalesce_params = ETHTOOL_COALESCE_RX_USECS,
	// .get_drvinfo		= nic_get_drvinfo,
	// .get_regs_len		= nic_get_regs_len,
	// .get_regs		= nic_get_regs,
	// .get_wol		= nic_get_wol,
	// .set_wol		= nic_set_wol,
	// .get_msglevel		= nic_get_msglevel,
	// .set_msglevel		= nic_set_msglevel,
	// .nway_reset		= nic_nway_reset,
	// .get_link		= nic_get_link,
	// .get_eeprom_len		= nic_get_eeprom_len,
	// .get_eeprom		= nic_get_eeprom,
	// .set_eeprom		= nic_set_eeprom,
	// .get_ringparam		= nic_get_ringparam,
	// .set_ringparam		= nic_set_ringparam,
	// .get_pauseparam		= nic_get_pauseparam,
	// .set_pauseparam		= nic_set_pauseparam,
	// .self_test		= nic_diag_test,
	// .get_strings		= nic_get_strings,
	// .set_phys_id		= nic_set_phys_id,
	// .get_ethtool_stats	= nic_get_ethtool_stats,
	// .get_sset_count		= nic_get_sset_count,
	// .get_coalesce		= nic_get_coalesce,
	// .set_coalesce		= nic_set_coalesce,
	// .get_ts_info		= ethtool_op_get_ts_info,
	// .get_link_ksettings	= nic_get_link_ksettings,
	// .set_link_ksettings	= nic_set_link_ksettings,
};

void nic_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &nic_ethtool_ops;
}
