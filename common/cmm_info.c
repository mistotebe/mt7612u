/****************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************

    Module Name:
	cmm_info.c

    Abstract:

    Revision History:
    Who          When          What
    ---------    ----------    ----------------------------------------------
 */

#include	"rt_config.h"



/*
    ==========================================================================
    Description:
        Get Driver version.

    Return:
    ==========================================================================
*/
INT Set_DriverVersion_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		DBGPRINT(RT_DEBUG_OFF, ("Driver version-%s\n", AP_DRIVER_VERSION));
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		DBGPRINT(RT_DEBUG_OFF, ("Driver version-%s\n", STA_DRIVER_VERSION));
#endif /* CONFIG_STA_SUPPORT */


    return true;
}

/*
    ==========================================================================
    Description:
        Set Country Region.
        This command will not work, if the field of CountryRegion in eeprom is programmed.
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT Set_CountryRegion_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	int retval;

	retval = RT_CfgSetCountryRegion(pAd, arg, BAND_24G);
	if (retval == false)
		return false;

	/* if set country region, driver needs to be reset*/
	BuildChannelList(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_CountryRegion_Proc::(CountryRegion=%d)\n", pAd->CommonCfg.CountryRegion));

	return true;
}


/*
    ==========================================================================
    Description:
        Set Country Region for A band.
        This command will not work, if the field of CountryRegion in eeprom is programmed.
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT Set_CountryRegionABand_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	int retval;

	retval = RT_CfgSetCountryRegion(pAd, arg, BAND_5G);
	if (retval == false)
		return false;

	/* if set country region, driver needs to be reset*/
	BuildChannelList(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_CountryRegionABand_Proc::(CountryRegion=%d)\n", pAd->CommonCfg.CountryRegionForABand));

	return true;
}


INT	Set_Cmm_WirelessMode_Proc(
	IN struct rtmp_adapter *pAd,
	IN char *arg,
	IN bool FlgIsDiffMbssModeUsed)
{
	INT	success = true;
#ifdef CONFIG_AP_SUPPORT
	uint32_t i = 0;
#ifdef MBSS_SUPPORT
	struct os_cookie *pObj = pAd->OS_Cookie;

	if (FlgIsDiffMbssModeUsed) {
		LONG cfg_mode = simple_strtol(arg, 0, 10);

		/* assign wireless mode for the BSS */
		pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.PhyMode =
					cfgmode_2_wmode((u8)cfg_mode);

		/*
			If the band is different with other BSS, we will correct it in
			RT_CfgSetMbssWirelessMode()
		*/
		success = RT_CfgSetMbssWirelessMode(pAd, arg);
	}
	else
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
	success = RT_CfgSetWirelessMode(pAd, arg);

	if (success)
	{
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			/* recover Wmm Capable for "each" BSS */
			/* all phy mode of MBSS are the same */
			for(i=0; i<pAd->ApCfg.BssidNum; i++)
			{
				pAd->ApCfg.MBSSID[i].wdev.bWmmCapable = \
										pAd->ApCfg.MBSSID[i].bWmmCapableOrg;

#ifdef MBSS_SUPPORT
				/* In Same-MBSS Mode, all phy modes are the same */
				if (FlgIsDiffMbssModeUsed == 0)
					pAd->ApCfg.MBSSID[i].wdev.PhyMode = pAd->CommonCfg.PhyMode;
#endif /* MBSS_SUPPORT */
			}

			RTMPSetPhyMode(pAd, pAd->CommonCfg.PhyMode);
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		{
			/* clean up previous SCAN result */
			BssTableInit(&pAd->ScanTab);
			pAd->StaCfg.LastScanTime = 0;

			RTMPSetPhyMode(pAd, pAd->CommonCfg.PhyMode);
			if (WMODE_CAP_N(pAd->CommonCfg.PhyMode))
			{
				pAd->CommonCfg.BACapability.field.AutoBA = true;
				pAd->CommonCfg.REGBACapability.field.AutoBA = true;
			}
			else
			{
				pAd->CommonCfg.BACapability.field.AutoBA = false;
				pAd->CommonCfg.REGBACapability.field.AutoBA = false;
			}
			/* Set AdhocMode rates*/
			if (pAd->StaCfg.BssType == BSS_ADHOC)
			{
				MlmeUpdateTxRates(pAd, false, 0);
				MakeIbssBeacon(pAd);           /* re-build BEACON frame*/
				AsicEnableIbssSync(pAd);       /* copy to on-chip memory*/
			}
		}
#endif /* CONFIG_STA_SUPPORT */


#ifdef CONFIG_AP_SUPPORT
#ifdef MBSS_SUPPORT
		DBGPRINT(RT_DEBUG_TRACE, ("Set_Cmm_WirelessMode_Proc::(=%d)\n", pAd->CommonCfg.PhyMode));
		DBGPRINT(RT_DEBUG_TRACE, ("Set_Cmm_WirelessMode_Proc::(BSS%d=%d)\n",
				pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.PhyMode));

		for(i=0; i<pAd->ApCfg.BssidNum; i++)
		{
			/*
				When last mode is not 11B-only, new mode is 11B, we need to re-make
				beacon frame content.

				Because we put support rate/extend support rate element in
				APMakeBssBeacon(), not APUpdateBeaconFrame().
			*/
			APMakeBssBeacon(pAd, i);
		}
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Set_WirelessMode_Proc::parameters out of range\n"));
	}

	return success;
}


#ifdef CONFIG_AP_SUPPORT
#ifdef MBSS_SUPPORT
/*
    ==========================================================================
    Description:
        Set Wireless Mode for MBSS
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_MBSS_WirelessMode_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	return Set_Cmm_WirelessMode_Proc(pAd, arg, 1);
}
#endif /* MBSS_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

/*
    ==========================================================================
    Description:
        Set Wireless Mode
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_WirelessMode_Proc(struct rtmp_adapter *pAd, char *arg)
{
	return Set_Cmm_WirelessMode_Proc(pAd, arg, 0);
}

#ifdef RT_CFG80211_SUPPORT
INT Set_DisableCfg2040Scan_Proc(struct rtmp_adapter *pAd, char *arg)
{
	pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan = (u8) simple_strtol(arg, 0, 10);
	DBGPRINT(RT_DEBUG_TRACE, ("pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan  %d \n",pAd->cfg80211_ctrl.FlgCfg8021Disable2040Scan ));
	return true;
}
#endif



/*
    ==========================================================================
    Description:
        Set Channel
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_Channel_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
#ifdef CONFIG_AP_SUPPORT
	INT		i;
#endif /* CONFIG_AP_SUPPORT */
 	INT		success = true;
	u8 Channel;

	Channel = (u8) simple_strtol(arg, 0, 10);

#ifdef APCLI_AUTO_CONNECT_SUPPORT
	if (pAd->ApCfg.ApCliAutoConnectChannelSwitching == false)
		pAd->ApCfg.ApCliAutoConnectChannelSwitching = true;
#endif /* APCLI_AUTO_CONNECT_SUPPORT */

	/* check if this channel is valid*/
	if (ChannelSanity(pAd, Channel) == true)
	{
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		{
			pAd->CommonCfg.Channel = Channel;
			/* Save the channel on MlmeAux for CntlOidRTBssidProc used. */
			pAd->MlmeAux.Channel = Channel;

			if (MONITOR_ON(pAd))
			{
				u8 rf_channel;

				N_ChannelCheck(pAd);
				if (WMODE_CAP_N(pAd->CommonCfg.PhyMode) &&
					pAd->CommonCfg.RegTransmitSetting.field.BW == BW_40)
					rf_channel = N_SetCenCh(pAd, pAd->CommonCfg.Channel);
				else
					rf_channel = pAd->CommonCfg.Channel;

				AsicSwitchChannel(pAd, rf_channel, false);
				AsicLockChannel(pAd, rf_channel);
				DBGPRINT(RT_DEBUG_TRACE, ("%s(): CtrlChannel(%d), CentralChannel(%d) \n",
							__FUNCTION__, pAd->CommonCfg.Channel,
							pAd->CommonCfg.CentralChannel));
			}
		}
#endif /* CONFIG_STA_SUPPORT */
		success = true;
	}
	else
	{
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			Channel = FirstChannel(pAd);
			DBGPRINT(RT_DEBUG_WARN,("This channel is out of channel list, set as the first channel(%d) \n ", Channel));
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		success = false;
		DBGPRINT(RT_DEBUG_WARN,("This channel is out of channel list, nothing to do!\n "));
#endif /* CONFIG_STA_SUPPORT */
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if ((WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
			&& (pAd->CommonCfg.bIEEE80211H == true))
		{
			for (i = 0; i < pAd->ChannelListNum; i++)
			{
				if (pAd->ChannelList[i].Channel == Channel)
				{
					if (pAd->ChannelList[i].RemainingTimeForUse > 0)
					{
						DBGPRINT(RT_DEBUG_ERROR, ("ERROR: previous detection of a radar on this channel(Channel=%d)\n", Channel));
						success = false;
						break;
					}
					else
					{
						DBGPRINT(RT_DEBUG_INFO, ("RemainingTimeForUse %d ,Channel %d\n",
								pAd->ChannelList[i].RemainingTimeForUse, Channel));
					}
				}
			}
		}

		if (success == true)
		{

			if ((pAd->CommonCfg.Channel > 14 )
				&& (pAd->CommonCfg.bIEEE80211H == true))
			{
				pAd->Dot11_H.org_ch = pAd->CommonCfg.Channel;
			}

			pAd->CommonCfg.Channel = Channel;
			N_ChannelCheck(pAd);

			if ((pAd->CommonCfg.Channel > 14 )
				&& (pAd->CommonCfg.bIEEE80211H == true))
			{
				if (pAd->Dot11_H.RDMode == RD_SILENCE_MODE)
				{
					APStop(pAd);
					APStartUp(pAd);
				}
				else
				{
					NotifyChSwAnnToPeerAPs(pAd, ZERO_MAC_ADDR, pAd->CurrentAddress, 1, pAd->CommonCfg.Channel);
					pAd->Dot11_H.RDMode = RD_SWITCHING_MODE;
					pAd->Dot11_H.CSCount = 0;
					pAd->Dot11_H.new_channel = Channel;
				}
			}
			else
			{
				APStop(pAd);
				APStartUp(pAd);
			}
		}
	}
#endif /* CONFIG_AP_SUPPORT */

	if (success == true) {
		DBGPRINT(RT_DEBUG_TRACE, ("Set_Channel_Proc::(Channel=%d)\n",
					pAd->CommonCfg.Channel));
#ifdef DFS_ATP_SUPPORT
		pAd->CommonCfg.RadarDetect.atp_set_channel_ready = true;
		if ((pAd->CommonCfg.RadarDetect.atp_set_ht_bw
				|| pAd->CommonCfg.RadarDetect.atp_set_vht_bw)
				&& pAd->CommonCfg.RadarDetect.atp_set_channel_ready) {
			printk ("BW and Channel is ready\n");
		}
#endif /* DFS_ATP_SUPPORT */
	}

#ifdef APCLI_AUTO_CONNECT_SUPPORT
		pAd->ApCfg.ApCliAutoConnectChannelSwitching = false;
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
	return success;
}


/*
    ==========================================================================
    Description:
        Set Short Slot Time Enable or Disable
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_ShortSlot_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	int retval;

	retval = RT_CfgSetShortSlot(pAd, arg);
	if (retval == true)
		DBGPRINT(RT_DEBUG_TRACE, ("Set_ShortSlot_Proc::(ShortSlot=%d)\n", pAd->CommonCfg.bUseShortSlotTime));

	return retval;
}


/*
    ==========================================================================
    Description:
        Set Tx power
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_TxPower_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	LONG TxPower;
	INT   success = false;

	TxPower = simple_strtol(arg, 0, 10);
	if (TxPower <= 100)
	{
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			pAd->CommonCfg.TxPowerPercentage = TxPower;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		{
			pAd->CommonCfg.TxPowerDefault = TxPower;
			pAd->CommonCfg.TxPowerPercentage = pAd->CommonCfg.TxPowerDefault;
		}
#endif /* CONFIG_STA_SUPPORT */
		success = true;
	}
	else
		success = false;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_TxPower_Proc::(TxPowerPercentage=%ld)\n", pAd->CommonCfg.TxPowerPercentage));

	return success;
}

/*
    ==========================================================================
    Description:
        Set 11B/11G Protection
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_BGProtection_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	switch (simple_strtol(arg, 0, 10))
	{
		case 0: /*AUTO*/
			pAd->CommonCfg.UseBGProtection = 0;
			break;
		case 1: /*Always On*/
			pAd->CommonCfg.UseBGProtection = 1;
			break;
		case 2: /*Always OFF*/
			pAd->CommonCfg.UseBGProtection = 2;
			break;
		default:  /*Invalid argument */
			return false;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		APUpdateCapabilityAndErpIe(pAd);
#endif /* CONFIG_AP_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_BGProtection_Proc::(BGProtection=%ld)\n", pAd->CommonCfg.UseBGProtection));

	return true;
}

/*
    ==========================================================================
    Description:
        Set TxPreamble
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_TxPreamble_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	RT_802_11_PREAMBLE	Preamble;

	Preamble = (RT_802_11_PREAMBLE)simple_strtol(arg, 0, 10);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	if (Preamble == Rt802_11PreambleAuto)
		return false;
#endif /* CONFIG_AP_SUPPORT */

	switch (Preamble)
	{
		case Rt802_11PreambleShort:
			pAd->CommonCfg.TxPreamble = Preamble;
#ifdef CONFIG_STA_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
				MlmeSetTxPreamble(pAd, Rt802_11PreambleShort);
#endif /* CONFIG_STA_SUPPORT */
			break;
		case Rt802_11PreambleLong:
#ifdef CONFIG_STA_SUPPORT
		case Rt802_11PreambleAuto:
			/*
				If user wants AUTO, initialize to LONG here, then change according to AP's
				capability upon association
			*/
#endif /* CONFIG_STA_SUPPORT */
			pAd->CommonCfg.TxPreamble = Preamble;
#ifdef CONFIG_STA_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
				MlmeSetTxPreamble(pAd, Rt802_11PreambleLong);
#endif /* CONFIG_STA_SUPPORT */
			break;
		default: /*Invalid argument */
			return false;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_TxPreamble_Proc::(TxPreamble=%ld)\n", pAd->CommonCfg.TxPreamble));

	return true;
}

/*
    ==========================================================================
    Description:
        Set RTS Threshold
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_RTSThreshold_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	 NDIS_802_11_RTS_THRESHOLD           RtsThresh;

	RtsThresh = simple_strtol(arg, 0, 10);

	if((RtsThresh > 0) && (RtsThresh <= MAX_RTS_THRESHOLD))
		pAd->CommonCfg.RtsThreshold  = (unsigned short)RtsThresh;
#ifdef CONFIG_STA_SUPPORT
	else if (RtsThresh == 0)
		pAd->CommonCfg.RtsThreshold = MAX_RTS_THRESHOLD;
#endif /* CONFIG_STA_SUPPORT */
	else
		return false; /*Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_RTSThreshold_Proc::(RTSThreshold=%d)\n", pAd->CommonCfg.RtsThreshold));

	return true;
}

/*
    ==========================================================================
    Description:
        Set Fragment Threshold
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_FragThreshold_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	 NDIS_802_11_FRAGMENTATION_THRESHOLD     FragThresh;

	FragThresh = simple_strtol(arg, 0, 10);

	if (FragThresh > MAX_FRAG_THRESHOLD || FragThresh < MIN_FRAG_THRESHOLD)
	{
		/*Illegal FragThresh so we set it to default*/
		pAd->CommonCfg.FragmentThreshold = MAX_FRAG_THRESHOLD;
	}
	else if (FragThresh % 2 == 1)
	{
		/*
			The length of each fragment shall always be an even number of octets,
			except for the last fragment of an MSDU or MMPDU, which may be either
			an even or an odd number of octets.
		*/
		pAd->CommonCfg.FragmentThreshold = (unsigned short)(FragThresh - 1);
	}
	else
	{
		pAd->CommonCfg.FragmentThreshold = (unsigned short)FragThresh;
	}

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (pAd->CommonCfg.FragmentThreshold == MAX_FRAG_THRESHOLD)
			pAd->CommonCfg.bUseZeroToDisableFragment = true;
		else
			pAd->CommonCfg.bUseZeroToDisableFragment = false;
	}
#endif /* CONFIG_STA_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_FragThreshold_Proc::(FragThreshold=%d)\n", pAd->CommonCfg.FragmentThreshold));

	return true;
}

/*
    ==========================================================================
    Description:
        Set TxBurst
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_TxBurst_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	LONG TxBurst;

	TxBurst = simple_strtol(arg, 0, 10);
	if (TxBurst == 1)
		pAd->CommonCfg.bEnableTxBurst = true;
	else if (TxBurst == 0)
		pAd->CommonCfg.bEnableTxBurst = false;
	else
		return false;  /*Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_TxBurst_Proc::(TxBurst=%d)\n", pAd->CommonCfg.bEnableTxBurst));

	return true;
}




#ifdef AGGREGATION_SUPPORT
/*
    ==========================================================================
    Description:
        Set TxBurst
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_PktAggregate_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	LONG aggre;

	aggre = simple_strtol(arg, 0, 10);

	if (aggre == 1)
		pAd->CommonCfg.bAggregationCapable = true;
	else if (aggre == 0)
		pAd->CommonCfg.bAggregationCapable = false;
	else
		return false;  /*Invalid argument */

#ifdef CONFIG_AP_SUPPORT
#ifdef PIGGYBACK_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		pAd->CommonCfg.bPiggyBackCapable = pAd->CommonCfg.bAggregationCapable;
		RTMPSetPiggyBack(pAd, pAd->CommonCfg.bPiggyBackCapable);
	}
#endif /* PIGGYBACK_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_PktAggregate_Proc::(AGGRE=%d)\n", pAd->CommonCfg.bAggregationCapable));

	return true;
}
#endif


#ifdef INF_PPA_SUPPORT
INT	Set_INF_AMAZON_SE_PPA_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	u8 *		arg)
{
	UINT status;
	u8 aggre;
	uint32_t g_if_id;
	int re_val;

	aggre = simple_strtol(arg, 0, 10);

	if (ppa_hook_directpath_register_dev_fn == NULL)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s::There is no PPA module\n", __FUNCTION__));
		return false;
	}

	if (aggre == 1)
	{
		if (pAd->PPAEnable == true)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("PPA already enabled\n"));
		}
		else
		{
			if (pAd->pDirectpathCb == NULL)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("Allocate memory for pDirectpathCb\n"));
				pAd->pDirectpathCb =
					kmalloc(sizeof(PPA_DIRECTPATH_CB), GFP_ATOMIC);

				if (pAd->pDirectpathCb == NULL)
					return false;
			}

			/* Register callback */
			pAd->pDirectpathCb->rx_fn = ifx_ra_start_xmit;
			pAd->pDirectpathCb->stop_tx_fn = NULL;
			pAd->pDirectpathCb->start_tx_fn = NULL;

			status = ppa_hook_directpath_register_dev_fn(
						&g_if_id, pAd->net_dev, pAd->pDirectpathCb, PPA_F_DIRECTPATH_REGISTER|PPA_F_DIRECTPATH_ETH_IF);

			if (status == IFX_SUCCESS)
			{
				pAd->g_if_id = g_if_id;
				pAd->PPAEnable = true;
				DBGPRINT(RT_DEBUG_TRACE, ("Register PPA success::ret=%d, id=%d\n", status, pAd->g_if_id));
			}
			else
				DBGPRINT(RT_DEBUG_TRACE, ("Register PPA fail::ret=%d\n", status));
		}
	}
	else if (aggre == 0)
	{
		if (pAd->PPAEnable == false)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("PPA already disabled\n"));
		}
		else
		{
			g_if_id = pAd->g_if_id;
			status = ppa_hook_directpath_register_dev_fn(&g_if_id, pAd->net_dev, NULL, 0 /*PPA_F_DIRECTPATH_REGISTER*/);

			if (status == IFX_SUCCESS)
			{
				pAd->g_if_id = 0;
				pAd->PPAEnable = false;
				DBGPRINT(RT_DEBUG_TRACE, ("Unregister PPA success::ret=%d, if_id=%d\n", status, pAd->g_if_id));
			}
			else
				DBGPRINT(RT_DEBUG_TRACE, ("Unregister PPA fail::ret=%d\n", status));
		}
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s::Invalid argument=%d\n", __FUNCTION__, aggre));
		return false;
	}

	return true;
}
#endif /* INF_PPA_SUPPORT */


/*
    ==========================================================================
    Description:
        Set IEEE80211H.
        This parameter is 1 when needs radar detection, otherwise 0
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_IEEE80211H_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
    LONG ieee80211h;

	ieee80211h = simple_strtol(arg, 0, 10);

	if (ieee80211h == 1)
		pAd->CommonCfg.bIEEE80211H = true;
	else if (ieee80211h == 0)
		pAd->CommonCfg.bIEEE80211H = false;
	else
		return false;  /*Invalid argument */

	DBGPRINT(RT_DEBUG_TRACE, ("Set_IEEE80211H_Proc::(IEEE80211H=%d)\n", pAd->CommonCfg.bIEEE80211H));

	return true;
}


#ifdef DBG
INT rx_temp_dbg = 0;

/*
    ==========================================================================
    Description:
        For Debug information
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_Debug_Proc(struct rtmp_adapter *pAd, char *arg)
{
	ULONG dbg;

	DBGPRINT_S(RT_DEBUG_TRACE, ("==>%s()\n", __FUNCTION__));

	dbg = simple_strtol(arg, 0, 10);
	if( dbg <= RT_DEBUG_LOUD)
		RTDebugLevel = dbg;

	DBGPRINT_S(RT_DEBUG_TRACE, ("<==%s(RTDebugLevel = %ld)\n",
				__FUNCTION__, RTDebugLevel));

	return true;
}


/*
    ==========================================================================
    Description:
        For DebugFunc information
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_DebugFunc_Proc(
	IN struct rtmp_adapter *pAd,
	IN char *arg)
{
	DBGPRINT_S(RT_DEBUG_TRACE, ("==>%s()\n", __FUNCTION__));
	RTDebugFunc = simple_strtol(arg, 0, 10);
	DBGPRINT_S(RT_DEBUG_TRACE, ("Set RTDebugFunc = 0x%lx\n", RTDebugFunc));

	return true;
}
#endif


INT	Show_DescInfo_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{

	return true;
}

/*
    ==========================================================================
    Description:
        Reset statistics counter

    Arguments:
        pAd            Pointer to our adapter
        arg

    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_ResetStatCounter_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	DBGPRINT(RT_DEBUG_TRACE, ("==>Set_ResetStatCounter_Proc\n"));

	/* add the most up-to-date h/w raw counters into software counters*/
	NICUpdateRawCounters(pAd);

	memset(&pAd->WlanCounters, 0, sizeof(COUNTER_802_11));
	memset(&pAd->Counters8023, 0, sizeof(COUNTER_802_3));
	memset(&pAd->RalinkCounters, 0, sizeof(COUNTER_RALINK));

#ifdef CONFIG_AP_SUPPORT
#endif /* CONFIG_AP_SUPPORT */

	if (pAd->chipCap.FlgHwTxBfCap)
	{
		int i;
		for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
			memset(&pAd->MacTab.Content[i].TxBFCounters, 0, sizeof(pAd->MacTab.Content[i].TxBFCounters));
	}

	return true;
}


bool RTMPCheckStrPrintAble(
    IN  CHAR *pInPutStr,
    IN  u8 strLen)
{
    u8 i=0;

    for (i=0; i<strLen; i++)
    {
        if ((pInPutStr[i] < 0x20) || (pInPutStr[i] > 0x7E))
            return false;
    }

    return true;
}


/*
	========================================================================

	Routine Description:
		Remove WPA Key process

	Arguments:
		pAd 					Pointer to our adapter
		pBuf							Pointer to the where the key stored

	Return Value:
		NDIS_SUCCESS					Add key successfully

	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
#ifdef CONFIG_STA_SUPPORT
VOID RTMPSetDesiredRates(struct rtmp_adapter *pAd, LONG Rates)
{
	NDIS_802_11_RATES aryRates;
	struct rtmp_wifi_dev *wdev = &pAd->StaCfg.wdev;

	memset(&aryRates, 0x00, sizeof(NDIS_802_11_RATES));
	switch (pAd->CommonCfg.PhyMode)
	{
        case (WMODE_A): /* A only*/
            switch (Rates)
            {
                case 6000000: /*6M*/
                    aryRates[0] = 0x0c; /* 6M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_0;
                    break;
                case 9000000: /*9M*/
                    aryRates[0] = 0x12; /* 9M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_1;
                    break;
                case 12000000: /*12M*/
                    aryRates[0] = 0x18; /* 12M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_2;
                    break;
                case 18000000: /*18M*/
                    aryRates[0] = 0x24; /* 18M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_3;
                    break;
                case 24000000: /*24M*/
                    aryRates[0] = 0x30; /* 24M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_4;
                    break;
                case 36000000: /*36M*/
                    aryRates[0] = 0x48; /* 36M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_5;
                    break;
                case 48000000: /*48M*/
                    aryRates[0] = 0x60; /* 48M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_6;
                    break;
                case 54000000: /*54M*/
                    aryRates[0] = 0x6c; /* 54M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_7;
                    break;
                case -1: /*Auto*/
                default:
                    aryRates[0] = 0x6c; /* 54Mbps*/
                    aryRates[1] = 0x60; /* 48Mbps*/
                    aryRates[2] = 0x48; /* 36Mbps*/
                    aryRates[3] = 0x30; /* 24Mbps*/
                    aryRates[4] = 0x24; /* 18M*/
                    aryRates[5] = 0x18; /* 12M*/
                    aryRates[6] = 0x12; /* 9M*/
                    aryRates[7] = 0x0c; /* 6M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_AUTO;
                    break;
            }
            break;
        case (WMODE_B | WMODE_G): /* B/G Mixed*/
        case (WMODE_B): /* B only*/
        case (WMODE_A | WMODE_B | WMODE_G): /* A/B/G Mixed*/
        default:
            switch (Rates)
            {
                case 1000000: /*1M*/
                    aryRates[0] = 0x02;
                    wdev->DesiredTransmitSetting.field.MCS = MCS_0;
                    break;
                case 2000000: /*2M*/
                    aryRates[0] = 0x04;
                    wdev->DesiredTransmitSetting.field.MCS = MCS_1;
                    break;
                case 5000000: /*5.5M*/
                    aryRates[0] = 0x0b; /* 5.5M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_2;
                    break;
                case 11000000: /*11M*/
                    aryRates[0] = 0x16; /* 11M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_3;
                    break;
                case 6000000: /*6M*/
                    aryRates[0] = 0x0c; /* 6M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_0;
                    break;
                case 9000000: /*9M*/
                    aryRates[0] = 0x12; /* 9M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_1;
                    break;
                case 12000000: /*12M*/
                    aryRates[0] = 0x18; /* 12M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_2;
                    break;
                case 18000000: /*18M*/
                    aryRates[0] = 0x24; /* 18M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_3;
                    break;
                case 24000000: /*24M*/
                    aryRates[0] = 0x30; /* 24M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_4;
                    break;
                case 36000000: /*36M*/
                    aryRates[0] = 0x48; /* 36M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_5;
                    break;
                case 48000000: /*48M*/
                    aryRates[0] = 0x60; /* 48M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_6;
                    break;
                case 54000000: /*54M*/
                    aryRates[0] = 0x6c; /* 54M*/
                    wdev->DesiredTransmitSetting.field.MCS = MCS_7;
                    break;
                case -1: /*Auto*/
                default:
                    if (pAd->CommonCfg.PhyMode == WMODE_B)
                    { /*B Only*/
                        aryRates[0] = 0x16; /* 11Mbps*/
                        aryRates[1] = 0x0b; /* 5.5Mbps*/
                        aryRates[2] = 0x04; /* 2Mbps*/
                        aryRates[3] = 0x02; /* 1Mbps*/
                    }
                    else
                    { /*(B/G) Mixed or (A/B/G) Mixed*/
                        aryRates[0] = 0x6c; /* 54Mbps*/
                        aryRates[1] = 0x60; /* 48Mbps*/
                        aryRates[2] = 0x48; /* 36Mbps*/
                        aryRates[3] = 0x30; /* 24Mbps*/
                        aryRates[4] = 0x16; /* 11Mbps*/
                        aryRates[5] = 0x0b; /* 5.5Mbps*/
                        aryRates[6] = 0x04; /* 2Mbps*/
                        aryRates[7] = 0x02; /* 1Mbps*/
                    }
                    wdev->DesiredTransmitSetting.field.MCS = MCS_AUTO;
                    break;
            }
            break;
    }

    memset(pAd->CommonCfg.DesireRate, 0, MAX_LEN_OF_SUPPORTED_RATES);
    memmove(pAd->CommonCfg.DesireRate, &aryRates, sizeof(NDIS_802_11_RATES));
    DBGPRINT(RT_DEBUG_TRACE, (" RTMPSetDesiredRates (%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x)\n",
        pAd->CommonCfg.DesireRate[0], pAd->CommonCfg.DesireRate[1],
        pAd->CommonCfg.DesireRate[2], pAd->CommonCfg.DesireRate[3],
        pAd->CommonCfg.DesireRate[4], pAd->CommonCfg.DesireRate[5],
        pAd->CommonCfg.DesireRate[6], pAd->CommonCfg.DesireRate[7] ));
    /* Changing DesiredRate may affect the MAX TX rate we used to TX frames out*/
    MlmeUpdateTxRates(pAd, false, 0);
}
#endif /* CONFIG_STA_SUPPORT */


#if defined(CONFIG_STA_SUPPORT) || defined(WPA_SUPPLICANT_SUPPORT)
int RTMPWPARemoveKeyProc(
	IN	struct rtmp_adapter *pAd,
	IN	PVOID			pBuf)
{
	PNDIS_802_11_REMOVE_KEY pKey;
	ULONG					KeyIdx;
	int 			Status = NDIS_STATUS_FAILURE;
	bool 	bTxKey; 		/* Set the key as transmit key*/
	bool 	bPairwise;		/* Indicate the key is pairwise key*/
	bool 	bKeyRSC;		/* indicate the receive  SC set by KeyRSC value.*/
								/* Otherwise, it will set by the NIC.*/
	bool 	bAuthenticator; /* indicate key is set by authenticator.*/
	INT 		i;
	DBGPRINT(RT_DEBUG_TRACE,("---> RTMPWPARemoveKeyProc\n"));

	pKey = (PNDIS_802_11_REMOVE_KEY) pBuf;
	KeyIdx = pKey->KeyIndex & 0xff;
	/* Bit 31 of Add-key, Tx Key*/
	bTxKey = (pKey->KeyIndex & 0x80000000) ? true : false;
	/* Bit 30 of Add-key PairwiseKey*/
	bPairwise = (pKey->KeyIndex & 0x40000000) ? true : false;
	/* Bit 29 of Add-key KeyRSC*/
	bKeyRSC = (pKey->KeyIndex & 0x20000000) ? true : false;
	/* Bit 28 of Add-key Authenticator*/
	bAuthenticator = (pKey->KeyIndex & 0x10000000) ? true : false;

	/* 1. If bTx is true, return failure information*/
	if (bTxKey == true)
		return(NDIS_STATUS_INVALID_DATA);

	/* 2. Check Pairwise Key*/
	if (bPairwise)
	{
		/* a. If BSSID is broadcast, remove all pairwise keys.*/
		/* b. If not broadcast, remove the pairwise specified by BSSID*/
		for (i = 0; i < SHARE_KEY_NUM; i++)
		{
			{
#ifdef CONFIG_STA_SUPPORT
				if (MAC_ADDR_EQUAL(pAd->SharedKey[BSS0][i].BssId, pKey->BSSID))
				{
					DBGPRINT(RT_DEBUG_TRACE,("RTMPWPARemoveKeyProc(KeyIdx=%d)\n", i));
					pAd->SharedKey[BSS0][i].KeyLen = 0;
					pAd->SharedKey[BSS0][i].CipherAlg = CIPHER_NONE;
					AsicRemoveSharedKeyEntry(pAd, BSS0, (u8)i);
					Status = NDIS_STATUS_SUCCESS;
					break;
				}
#endif/*CONFIG_STA_SUPPORT*/
			}
		}
	}
	/* 3. Group Key*/
	else
	{
		/* a. If BSSID is broadcast, remove all group keys indexed*/
		/* b. If BSSID matched, delete the group key indexed.*/
		DBGPRINT(RT_DEBUG_TRACE,("RTMPWPARemoveKeyProc(KeyIdx=%ld)\n", KeyIdx));
		pAd->SharedKey[BSS0][KeyIdx].KeyLen = 0;
		pAd->SharedKey[BSS0][KeyIdx].CipherAlg = CIPHER_NONE;
		AsicRemoveSharedKeyEntry(pAd, BSS0, (u8)KeyIdx);
		Status = NDIS_STATUS_SUCCESS;
	}

	return (Status);
}
#endif /* defined(CONFIG_STA_SUPPORT) || defined(WPA_SUPPLICANT_SUPPORT) */


#ifdef CONFIG_STA_SUPPORT
/*
	========================================================================

	Routine Description:
		Remove All WPA Keys

	Arguments:
		pAd 					Pointer to our adapter

	Return Value:
		None

	IRQL = DISPATCH_LEVEL

	Note:

	========================================================================
*/
VOID RTMPWPARemoveAllKeys(struct rtmp_adapter *pAd)
{
	u8 i;
	struct rtmp_wifi_dev *wdev = &pAd->StaCfg.wdev;


	DBGPRINT(RT_DEBUG_TRACE,("RTMPWPARemoveAllKeys(AuthMode=%d, WepStatus=%d)\n",
				wdev->AuthMode, wdev->WepStatus));
	/*
		For WEP/CKIP/WPA-None, there is no need to remove it, since WinXP won't set it
		again after Link up. And it will be replaced if user changed it.
	*/
	if (wdev->AuthMode < Ndis802_11AuthModeWPA ||
		wdev->AuthMode == Ndis802_11AuthModeWPANone)
		return;

	/* set BSSID wcid entry of the Pair-wise Key table as no-security mode*/
	AsicRemovePairwiseKeyEntry(pAd, BSSID_WCID);

	/* set all shared key mode as no-security. */
	for (i = 0; i < SHARE_KEY_NUM; i++)
    {
		DBGPRINT(RT_DEBUG_TRACE,("remove %s key #%d\n", CipherName[pAd->SharedKey[BSS0][i].CipherAlg], i));
		memset(&pAd->SharedKey[BSS0][i], 0, sizeof(CIPHER_KEY));

		AsicRemoveSharedKeyEntry(pAd, BSS0, i);
	}
}
#endif /* CONFIG_STA_SUPPORT */


/*
	========================================================================
	Routine Description:
		Change NIC PHY mode. Re-association may be necessary

	Arguments:
		pAd - Pointer to our adapter
		phymode  -

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL

	========================================================================
*/
VOID RTMPSetPhyMode(struct rtmp_adapter *pAd, ULONG phymode)
{
	INT i;
	/* the selected phymode must be supported by the RF IC encoded in E2PROM*/

	pAd->CommonCfg.PhyMode = (u8)phymode;

	DBGPRINT(RT_DEBUG_TRACE,("RTMPSetPhyMode : PhyMode=%d, channel=%d \n", pAd->CommonCfg.PhyMode, pAd->CommonCfg.Channel));
	BuildChannelList(pAd);

	/* sanity check user setting*/
	for (i = 0; i < pAd->ChannelListNum; i++)
	{
		if (pAd->CommonCfg.Channel == pAd->ChannelList[i].Channel)
			break;
	}

	if (i == pAd->ChannelListNum)
	{
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		if (pAd->CommonCfg.Channel != 0)
				pAd->CommonCfg.Channel = FirstChannel(pAd);
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
			pAd->CommonCfg.Channel = FirstChannel(pAd);
#endif /* CONFIG_STA_SUPPORT */
		DBGPRINT(RT_DEBUG_ERROR, ("RTMPSetPhyMode: channel is out of range, use first channel=%d \n", pAd->CommonCfg.Channel));
	}

	memset(pAd->CommonCfg.SupRate, 0, MAX_LEN_OF_SUPPORTED_RATES);
	memset(pAd->CommonCfg.ExtRate, 0, MAX_LEN_OF_SUPPORTED_RATES);
	memset(pAd->CommonCfg.DesireRate, 0, MAX_LEN_OF_SUPPORTED_RATES);
	switch (phymode) {
		case (WMODE_B):
			pAd->CommonCfg.SupRate[0]  = 0x82;	  /* 1 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[1]  = 0x84;	  /* 2 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[2]  = 0x8B;	  /* 5.5 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[3]  = 0x96;	  /* 11 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRateLen  = 4;
			pAd->CommonCfg.ExtRateLen  = 0;
			pAd->CommonCfg.DesireRate[0]  = 2;	   /* 1 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[1]  = 4;	   /* 2 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[2]  = 11;    /* 5.5 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[3]  = 22;    /* 11 mbps, in units of 0.5 Mbps*/
			/*pAd->CommonCfg.HTPhyMode.field.MODE = MODE_CCK;  This MODE is only FYI. not use*/
			break;

		/*
			In current design, we will put supported/extended rate element in
			beacon even we are 11n-only mode.
			Or some 11n stations will not connect to us if we do not put
			supported/extended rate element in beacon.
		*/
		case (WMODE_B | WMODE_G):
		case (WMODE_A | WMODE_B | WMODE_G):
		case (WMODE_A | WMODE_B | WMODE_G | WMODE_GN | WMODE_AN):
		case (WMODE_B | WMODE_G | WMODE_GN):
		case  (WMODE_B | WMODE_G | WMODE_GN |WMODE_A | WMODE_AN | WMODE_AC):
			pAd->CommonCfg.SupRate[0]  = 0x82;	  /* 1 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[1]  = 0x84;	  /* 2 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[2]  = 0x8B;	  /* 5.5 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[3]  = 0x96;	  /* 11 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[4]  = 0x12;	  /* 9 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.SupRate[5]  = 0x24;	  /* 18 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.SupRate[6]  = 0x48;	  /* 36 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.SupRate[7]  = 0x6c;	  /* 54 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.SupRateLen  = 8;
			pAd->CommonCfg.ExtRate[0]  = 0x0C;	  /* 6 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.ExtRate[1]  = 0x18;	  /* 12 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.ExtRate[2]  = 0x30;	  /* 24 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.ExtRate[3]  = 0x60;	  /* 48 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.ExtRateLen  = 4;
			pAd->CommonCfg.DesireRate[0]  = 2;	   /* 1 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[1]  = 4;	   /* 2 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[2]  = 11;    /* 5.5 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[3]  = 22;    /* 11 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[4]  = 12;    /* 6 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[5]  = 18;    /* 9 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[6]  = 24;    /* 12 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[7]  = 36;    /* 18 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[8]  = 48;    /* 24 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[9]  = 72;    /* 36 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[10] = 96;    /* 48 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[11] = 108;   /* 54 mbps, in units of 0.5 Mbps*/
			break;

		case (WMODE_A):
		case (WMODE_G):
		case (WMODE_A | WMODE_AN):
		case (WMODE_A | WMODE_G | WMODE_GN | WMODE_AN):
		case (WMODE_G | WMODE_GN):
		case (WMODE_GN):
		case (WMODE_AN):
		case (WMODE_A | WMODE_AN | WMODE_AC):
		case (WMODE_AN | WMODE_AC):
		case (WMODE_G | WMODE_GN |WMODE_A | WMODE_AN | WMODE_AC):
			pAd->CommonCfg.SupRate[0]  = 0x8C;	  /* 6 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[1]  = 0x12;	  /* 9 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.SupRate[2]  = 0x98;	  /* 12 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[3]  = 0x24;	  /* 18 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.SupRate[4]  = 0xb0;	  /* 24 mbps, in units of 0.5 Mbps, basic rate*/
			pAd->CommonCfg.SupRate[5]  = 0x48;	  /* 36 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.SupRate[6]  = 0x60;	  /* 48 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.SupRate[7]  = 0x6c;	  /* 54 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.SupRateLen  = 8;
			pAd->CommonCfg.ExtRateLen  = 0;
			pAd->CommonCfg.DesireRate[0]  = 12;    /* 6 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[1]  = 18;    /* 9 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[2]  = 24;    /* 12 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[3]  = 36;    /* 18 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[4]  = 48;    /* 24 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[5]  = 72;    /* 36 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[6]  = 96;    /* 48 mbps, in units of 0.5 Mbps*/
			pAd->CommonCfg.DesireRate[7]  = 108;   /* 54 mbps, in units of 0.5 Mbps*/
			/*pAd->CommonCfg.HTPhyMode.field.MODE = MODE_OFDM;  This MODE is only FYI. not use*/
			break;

		default:
			break;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		UINT	apidx;

		for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
		{
			MlmeUpdateTxRates(pAd, false, apidx);
		}
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		printk("%s: Update for STA\n", __FUNCTION__);
		MlmeUpdateTxRates(pAd, false, BSS0);
	}
#endif /* CONFIG_STA_SUPPORT */


//CFG_TODO

	SetCommonHT(pAd);

	SetCommonVHT(pAd);
}


/*
    ==========================================================================
    Description:
        Parse encryption type
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
    ==========================================================================
*/
char *GetEncryptType(CHAR enc)
{
	switch (enc) {
		case Ndis802_11WEPDisabled:
			return "NONE";
		case Ndis802_11WEPEnabled:
			return "WEP";
		case Ndis802_11TKIPEnable:
			return "TKIP";
		case Ndis802_11AESEnable:
			return "AES";
		case Ndis802_11TKIPAESMix:
			return "TKIPAES";
		default:
			return "UNKNOW";
	}
}

char *GetAuthMode(CHAR auth)
{
	switch (auth) {
		case Ndis802_11AuthModeOpen:
			return "OPEN";
		case Ndis802_11AuthModeShared:
			return "SHARED";
		case Ndis802_11AuthModeAutoSwitch:
			return "AUTOWEP";
		case Ndis802_11AuthModeWPA:
			return "WPA";
		case Ndis802_11AuthModeWPAPSK:
			return "WPAPSK";
		case Ndis802_11AuthModeWPANone:
			return "WPANONE";
		case Ndis802_11AuthModeWPA2:
			return "WPA2";
		case Ndis802_11AuthModeWPA2PSK:
			return "WPA2PSK";
		case Ndis802_11AuthModeWPA1WPA2:
			return "WPA1WPA2";
		case Ndis802_11AuthModeWPA1PSKWPA2PSK:
			return "WPA1PSKWPA2PSK";
		default:
			return "UNKNOW";
	}
}


/*
    ==========================================================================
    Description:
        Get site survey results
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage:
        		1.) UI needs to wait 4 seconds after issue a site survey command
        		2.) iwpriv ra0 get_site_survey
        		3.) UI needs to prepare at least 4096bytes to get the results
    ==========================================================================
*/
#define	LINE_LEN	(4+33+20+23+9+7+7+3)	/* Channel+SSID+Bssid+Security+Signal+WiressMode+ExtCh+NetworkType*/

#ifdef CONFIG_STA_SUPPORT
#endif /* CONFIG_STA_SUPPORT */
VOID	RTMPCommSiteSurveyData(
	IN  char *msg,
	IN  BSS_ENTRY *pBss,
	IN  uint32_t MsgLen)
{
	INT         Rssi = 0;
	UINT        Rssi_Quality = 0;
	NDIS_802_11_NETWORK_TYPE    wireless_mode;
	CHAR		Ssid[MAX_LEN_OF_SSID +1];
	STRING		SecurityStr[32] = {0};
	NDIS_802_11_ENCRYPTION_STATUS	ap_cipher = Ndis802_11EncryptionDisabled;
	NDIS_802_11_AUTHENTICATION_MODE	ap_auth_mode = Ndis802_11AuthModeOpen;

		/*Channel*/
		sprintf(msg+strlen(msg),"%-4d", pBss->Channel);


		/*SSID*/
	memset(Ssid, 0, (MAX_LEN_OF_SSID +1));
	if (RTMPCheckStrPrintAble((PCHAR)pBss->Ssid, pBss->SsidLen))
		memmove(Ssid, pBss->Ssid, pBss->SsidLen);
	else
	{
		INT idx = 0;
		sprintf(Ssid, "0x");
		for (idx = 0; (idx < 14) && (idx < pBss->SsidLen); idx++)
			sprintf(Ssid + 2 + (idx*2), "%02X", (u8)pBss->Ssid[idx]);
	}
		sprintf(msg+strlen(msg),"%-33s", Ssid);

		/*BSSID*/
		sprintf(msg+strlen(msg),"%02x:%02x:%02x:%02x:%02x:%02x   ",
			pBss->Bssid[0],
			pBss->Bssid[1],
			pBss->Bssid[2],
			pBss->Bssid[3],
			pBss->Bssid[4],
			pBss->Bssid[5]);

	/*Security*/
	RTMPZeroMemory(SecurityStr, 32);
	if ((Ndis802_11AuthModeWPA <= pBss->AuthMode) &&
		(pBss->AuthMode <= Ndis802_11AuthModeWPA1PSKWPA2PSK))
	{
		if (pBss->AuthMode == Ndis802_11AuthModeWPANone)
		{
			ap_auth_mode = pBss->AuthMode;
				ap_cipher = pBss->WPA.PairCipher;
		}
		else if (pBss->AuthModeAux == Ndis802_11AuthModeOpen)
		{
			ap_auth_mode = pBss->AuthMode;
			if ((ap_auth_mode == Ndis802_11AuthModeWPA) ||
				(ap_auth_mode == Ndis802_11AuthModeWPAPSK))
			{
				if (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled)
					ap_cipher = pBss->WPA.PairCipher;
				else
					ap_cipher = Ndis802_11TKIPAESMix;
			}
			else if ((ap_auth_mode == Ndis802_11AuthModeWPA2) ||
					 (ap_auth_mode == Ndis802_11AuthModeWPA2PSK))
			{
				if (pBss->WPA2.PairCipherAux == Ndis802_11WEPDisabled)
					ap_cipher = pBss->WPA2.PairCipher;
				else
					ap_cipher = Ndis802_11TKIPAESMix;
			}
		}
		else if ((pBss->AuthMode == Ndis802_11AuthModeWPAPSK) ||
				 (pBss->AuthMode == Ndis802_11AuthModeWPA2PSK))
		{
			if ((pBss->AuthModeAux == Ndis802_11AuthModeWPAPSK) ||
				(pBss->AuthModeAux == Ndis802_11AuthModeWPA2PSK))
				ap_auth_mode = Ndis802_11AuthModeWPA1PSKWPA2PSK;
			else
				ap_auth_mode = pBss->AuthMode;

			if (pBss->WPA.PairCipher != pBss->WPA2.PairCipher)
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
					 (pBss->WPA.PairCipherAux != pBss->WPA2.PairCipherAux))
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
					 (pBss->WPA.PairCipherAux == pBss->WPA2.PairCipherAux) &&
					 (pBss->WPA.PairCipherAux != Ndis802_11WEPDisabled))
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
					 (pBss->WPA.PairCipherAux == pBss->WPA2.PairCipherAux) &&
					 (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled))
				ap_cipher = pBss->WPA.PairCipher;
		}
		else if ((pBss->AuthMode == Ndis802_11AuthModeWPA) ||
				 (pBss->AuthMode == Ndis802_11AuthModeWPA2))
		{
			if ((pBss->AuthModeAux == Ndis802_11AuthModeWPA) ||
				(pBss->AuthModeAux == Ndis802_11AuthModeWPA2))
				ap_auth_mode = Ndis802_11AuthModeWPA1WPA2;
			else
				ap_auth_mode = pBss->AuthMode;

			if (pBss->WPA.PairCipher != pBss->WPA2.PairCipher)
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
					 (pBss->WPA.PairCipherAux != pBss->WPA2.PairCipherAux))
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
					 (pBss->WPA.PairCipherAux == pBss->WPA2.PairCipherAux) &&
					 (pBss->WPA.PairCipherAux != Ndis802_11WEPDisabled))
				ap_cipher = Ndis802_11TKIPAESMix;
			else if ((pBss->WPA.PairCipher == pBss->WPA2.PairCipher) &&
					 (pBss->WPA.PairCipherAux == pBss->WPA2.PairCipherAux) &&
					 (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled))
				ap_cipher = pBss->WPA.PairCipher;
		}

		sprintf(SecurityStr, "%s/%s", GetAuthMode((CHAR)ap_auth_mode), GetEncryptType((CHAR)ap_cipher));
	}
	else
	{
		ap_auth_mode = pBss->AuthMode;
		ap_cipher = pBss->WepStatus;
		if (ap_cipher == Ndis802_11WEPDisabled)
			sprintf(SecurityStr, "NONE");
		else if (ap_cipher == Ndis802_11WEPEnabled)
			sprintf(SecurityStr, "WEP");
		else
			sprintf(SecurityStr, "%s/%s", GetAuthMode((CHAR)ap_auth_mode), GetEncryptType((CHAR)ap_cipher));
	}

	sprintf(msg+strlen(msg), "%-23s", SecurityStr);

		/* Rssi*/
		Rssi = (INT)pBss->Rssi;
		if (Rssi >= -50)
			Rssi_Quality = 100;
		else if (Rssi >= -80)    /* between -50 ~ -80dbm*/
			Rssi_Quality = (UINT)(24 + ((Rssi + 80) * 26)/10);
		else if (Rssi >= -90)   /* between -80 ~ -90dbm*/
			Rssi_Quality = (UINT)(((Rssi + 90) * 26)/10);
		else    /* < -84 dbm*/
			Rssi_Quality = 0;
		sprintf(msg+strlen(msg),"%-9d", Rssi_Quality);

		/* Wireless Mode*/
		wireless_mode = NetworkTypeInUseSanity(pBss);
		if (wireless_mode == Ndis802_11FH ||
			wireless_mode == Ndis802_11DS)
			sprintf(msg+strlen(msg),"%-7s", "11b");
		else if (wireless_mode == Ndis802_11OFDM5)
			sprintf(msg+strlen(msg),"%-7s", "11a");
		else if (wireless_mode == Ndis802_11OFDM5_N)
			sprintf(msg+strlen(msg),"%-7s", "11a/n");
		else if (wireless_mode == Ndis802_11OFDM5_AC)
			sprintf(msg+strlen(msg),"%-7s", "11a/n/ac");
		else if (wireless_mode == Ndis802_11OFDM24)
			sprintf(msg+strlen(msg),"%-7s", "11b/g");
		else if (wireless_mode == Ndis802_11OFDM24_N)
			sprintf(msg+strlen(msg),"%-7s", "11b/g/n");
		else
			sprintf(msg+strlen(msg),"%-7s", "unknow");

		/* Ext Channel*/
		if (pBss->AddHtInfoLen > 0)
		{
			if (pBss->AddHtInfo.AddHtInfo.ExtChanOffset == EXTCHA_ABOVE)
				sprintf(msg+strlen(msg),"%-7s", " ABOVE");
			else if (pBss->AddHtInfo.AddHtInfo.ExtChanOffset == EXTCHA_BELOW)
				sprintf(msg+strlen(msg),"%-7s", " BELOW");
			else
				sprintf(msg+strlen(msg),"%-7s", " NONE");
		}
		else
		{
			sprintf(msg+strlen(msg),"%-7s", " NONE");
		}

		/*Network Type		*/
		if (pBss->BssType == BSS_ADHOC)
			sprintf(msg+strlen(msg),"%-3s", " Ad");
		else
			sprintf(msg+strlen(msg),"%-3s", " In");

        sprintf(msg+strlen(msg),"\n");

	return;
}

#if defined (AP_SCAN_SUPPORT) || defined (CONFIG_STA_SUPPORT)
VOID RTMPIoctlGetSiteSurvey(
	IN	struct rtmp_adapter *pAdapter,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq)
{
	char *	msg;
	INT 		i=0;
	INT			WaitCnt;
	INT 		Status=0;
    INT         max_len = LINE_LEN;
	BSS_ENTRY *pBss;
	uint32_t TotalLen, BufLen = IW_SCAN_MAX_DATA;

#ifdef CONFIG_STA_SUPPORT
#endif /* CONFIG_STA_SUPPORT */

	TotalLen = sizeof(CHAR)*((MAX_LEN_OF_BSS_TABLE)*max_len) + 100;

	if (wrq->u.data.length == 0)
		BufLen = IW_SCAN_MAX_DATA;
	else
		BufLen = wrq->u.data.length;

	msg = kmalloc(TotalLen, GFP_ATOMIC);
	if (msg == NULL) {
		DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlGetSiteSurvey - msg memory alloc fail.\n"));
		return;
	}

	memset(msg, 0 , TotalLen);
	sprintf(msg,"%s","\n");

	sprintf(msg+strlen(msg),"%-4s%-33s%-20s%-23s%-9s%-7s%-7s%-3s\n",
	    "Ch", "SSID", "BSSID", "Security", "Siganl(%)", "W-Mode", " ExtCH"," NT");


#ifdef CONFIG_STA_SUPPORT
#endif /* CONFIG_STA_SUPPORT */

	WaitCnt = 0;
#ifdef CONFIG_STA_SUPPORT
	pAdapter->StaCfg.bSkipAutoScanConn = true;
#endif /* CONFIG_STA_SUPPORT */

	while ((ScanRunning(pAdapter) == true) && (WaitCnt++ < 200))
		msleep_interruptible(500);

	for(i=0; i<pAdapter->ScanTab.BssNr ;i++)
	{
		pBss = &pAdapter->ScanTab.BssEntry[i];

		if( pBss->Channel==0)
			break;

		if((strlen(msg)+100 ) >= BufLen)
			break;


		RTMPCommSiteSurveyData(msg, pBss, TotalLen);


#ifdef CONFIG_STA_SUPPORT
#endif /* CONFIG_STA_SUPPORT */
	}

#ifdef CONFIG_STA_SUPPORT
	pAdapter->StaCfg.bSkipAutoScanConn = false;
#endif /* CONFIG_STA_SUPPORT */
	wrq->u.data.length = strlen(msg);
	Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);

	DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlGetSiteSurvey - wrq->u.data.length = %d\n", wrq->u.data.length));
	kfree((u8 *)msg);
}
#endif


VOID RTMPIoctlGetMacTableStaInfo(
	IN struct rtmp_adapter *pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	INT i;
	RT_802_11_MAC_TABLE *pMacTab = NULL;
	PRT_802_11_MAC_ENTRY pDst;
	MAC_TABLE_ENTRY *pEntry;

	/* allocate memory */
	pMacTab = kmalloc(sizeof(RT_802_11_MAC_TABLE), GFP_ATOMIC);
	if (pMacTab == NULL) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return;
	}

	memset(pMacTab, 0, sizeof(RT_802_11_MAC_TABLE));
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		pEntry = &(pAd->MacTab.Content[i]);
		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{
			pDst = &pMacTab->Entry[pMacTab->Num];

			pDst->ApIdx = pEntry->apidx;
			COPY_MAC_ADDR(pDst->Addr, &pEntry->Addr);
			pDst->Aid = (u8)pEntry->Aid;
			pDst->Psm = pEntry->PsMode;

			pDst->MimoPs = pEntry->MmpsMode;

			/* Fill in RSSI per entry*/
			pDst->AvgRssi0 = pEntry->RssiSample.AvgRssi0;
			pDst->AvgRssi1 = pEntry->RssiSample.AvgRssi1;
			pDst->AvgRssi2 = pEntry->RssiSample.AvgRssi2;

			/* the connected time per entry*/
			pDst->ConnectedTime = pEntry->StaConnectTime;
			pDst->TxRate.word = pEntry->HTPhyMode.word;

			pDst->LastRxRate = pEntry->LastRxRate;

			pMacTab->Num += 1;
		}
	}

	wrq->u.data.length = sizeof(RT_802_11_MAC_TABLE);
	if (copy_to_user(wrq->u.data.pointer, pMacTab, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

	if (pMacTab != NULL)
		kfree(pMacTab);
}


#define	MAC_LINE_LEN	(1+14+4+4+4+4+10+10+10+6+6)	/* "\n"+Addr+aid+psm+datatime+rxbyte+txbyte+current tx rate+last tx rate+"\n" */
VOID RTMPIoctlGetMacTable(
	IN struct rtmp_adapter *pAd,
	IN RTMP_IOCTL_INPUT_STRUCT *wrq)
{
	INT i;
/*	RT_802_11_MAC_TABLE MacTab;*/
	RT_802_11_MAC_TABLE *pMacTab = NULL;
	RT_802_11_MAC_ENTRY *pDst;
	MAC_TABLE_ENTRY *pEntry;
	char *msg;

	/* allocate memory */
	pMacTab = kmalloc(sizeof(RT_802_11_MAC_TABLE), GFP_ATOMIC);
	if (pMacTab == NULL) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return;
	}

	memset(pMacTab, 0, sizeof(RT_802_11_MAC_TABLE));
	pMacTab->Num = 0;
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		pEntry = &(pAd->MacTab.Content[i]);

		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{
			pDst = &pMacTab->Entry[pMacTab->Num];


			pDst->ApIdx = (u8)pEntry->apidx;
			COPY_MAC_ADDR(pDst->Addr, &pEntry->Addr);
			pDst->Aid = (u8)pEntry->Aid;
			pDst->Psm = pEntry->PsMode;
			pDst->MimoPs = pEntry->MmpsMode;

			/* Fill in RSSI per entry*/
			pDst->AvgRssi0 = pEntry->RssiSample.AvgRssi0;
			pDst->AvgRssi1 = pEntry->RssiSample.AvgRssi1;
			pDst->AvgRssi2 = pEntry->RssiSample.AvgRssi2;

			/* the connected time per entry*/
			pDst->ConnectedTime = pEntry->StaConnectTime;
			pDst->TxRate.word = pEntry->HTPhyMode.word;


			pMacTab->Num += 1;
		}
	}

	wrq->u.data.length = sizeof(RT_802_11_MAC_TABLE);
	if (copy_to_user(wrq->u.data.pointer, pMacTab, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

	msg = kmalloc(sizeof(CHAR)*(MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN), GFP_ATOMIC);
	if (msg == NULL) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s():Alloc memory failed\n", __FUNCTION__));
		goto LabelOK;
	}
	memset(msg, 0 ,MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN );
	sprintf(msg,"%s","\n");
	sprintf(msg+strlen(msg),"%-14s%-4s%-4s%-4s%-4s%-6s%-6s%-10s%-10s%-10s\n",
		"MAC", "AP",  "AID", "PSM", "AUTH", "CTxR", "LTxR","LDT", "RxB", "TxB");

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[i];

		if (IS_ENTRY_CLIENT(pEntry) && (pEntry->Sst == SST_ASSOC))
		{
			if((strlen(msg)+MAC_LINE_LEN ) >= (MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN) )
				break;
			sprintf(msg+strlen(msg),"%02x%02x%02x%02x%02x%02x  ", PRINT_MAC(pEntry->Addr));
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->apidx);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->Aid);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->PsMode);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->AuthState);
			sprintf(msg+strlen(msg),"%-6d",RateIdToMbps[pAd->MacTab.Content[i].CurrTxRate]);
			sprintf(msg+strlen(msg),"%-6d",0/*RateIdToMbps[pAd->MacTab.Content[i].HTPhyMode.word]*/); /* ToDo*/
			sprintf(msg+strlen(msg),"%-10d",0/*pAd->MacTab.Content[i].HSCounter.LastDataPacketTime*/); /* ToDo*/
			sprintf(msg+strlen(msg),"%-10d",0/*pAd->MacTab.Content[i].HSCounter.TotalRxByteCount*/); /* ToDo*/
			sprintf(msg+strlen(msg),"%-10d\n",0/*pAd->MacTab.Content[i].HSCounter.TotalTxByteCount*/); /* ToDo*/

		}
	}
	/* for compatible with old API just do the printk to console*/

	DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));
	kfree(msg);

LabelOK:
	if (pMacTab != NULL)
		kfree(pMacTab);
}

INT	Set_BASetup_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
    u8 mac[6], tid;
	char *token;
	STRING sepValue[] = ":", DASH = '-';
	INT i;
    MAC_TABLE_ENTRY *pEntry;

/*
	The BASetup inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the tid value.
*/
	/*DBGPRINT(RT_DEBUG_TRACE,("\n%s\n", arg));*/

	if(strlen(arg) < 19)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.*/
		return false;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token)>1))
	{
		tid = (u8) simple_strtol((token+1), 0, 10);
		/* tid is 0 ~ 7; Or kernel will crash in BAOriSessionSetUp() */
		if (tid > (NUM_OF_TID-1))
			return false;

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++)
		{
			if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return false;
			AtoH(token, (&mac[i]), 1);
		}
		if(i != 6)
			return false;

		DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x\n",
								mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], tid));

	    pEntry = MacTableLookup(pAd, (u8 *) mac);

    	if (pEntry) {
        	DBGPRINT(RT_DEBUG_OFF, ("\nSetup BA Session: Tid = %d\n", tid));
	        BAOriSessionSetUp(pAd, pEntry, tid, 0, 100, true);
    	}

		return true;
	}

	return false;

}

INT	Set_BADecline_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG bBADecline;

	bBADecline = simple_strtol(arg, 0, 10);

	if (bBADecline == 0)
	{
		pAd->CommonCfg.bBADecline = false;
	}
	else if (bBADecline == 1)
	{
		pAd->CommonCfg.bBADecline = true;
	}
	else
	{
		return false; /*Invalid argument*/
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_BADecline_Proc::(BADecline=%d)\n", pAd->CommonCfg.bBADecline));

	return true;
}

INT	Set_BAOriTearDown_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
    u8 mac[6], tid;
	char *token;
	STRING sepValue[] = ":", DASH = '-';
	INT i;
    MAC_TABLE_ENTRY *pEntry;

    /*DBGPRINT(RT_DEBUG_TRACE,("\n%s\n", arg));*/
/*
	The BAOriTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the tid value.
*/
    if(strlen(arg) < 19)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.*/
		return false;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token)>1))
	{
		tid = simple_strtol((token+1), 0, 10);
		/* tid will be 0 ~ 7; Or kernel will crash in BAOriSessionTearDown() */
		if (tid > (NUM_OF_TID-1))
			return false;

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++)
		{
			if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return false;
			AtoH(token, (&mac[i]), 1);
		}
		if(i != 6)
			return false;

	    DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x",
								PRINT_MAC(mac), tid));

	    pEntry = MacTableLookup(pAd, (u8 *) mac);

	    if (pEntry) {
	        DBGPRINT(RT_DEBUG_OFF, ("\nTear down Ori BA Session: Tid = %d\n", tid));
	        BAOriSessionTearDown(pAd, pEntry->wcid, tid, false, true);
	    }

		return true;
	}

	return false;

}

INT	Set_BARecTearDown_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
    u8 mac[6], tid;
	char *token;
	STRING sepValue[] = ":", DASH = '-';
	INT i;
    MAC_TABLE_ENTRY *pEntry;

    /*DBGPRINT(RT_DEBUG_TRACE,("\n%s\n", arg));*/
/*
	The BARecTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the tid value.
*/
    if(strlen(arg) < 19)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.*/
		return false;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token)>1))
	{
		tid = simple_strtol((token+1), 0, 10);
		/* tid will be 0 ~ 7; Or kernel will crash in BARecSessionTearDown() */
		if (tid > (NUM_OF_TID-1))
			return false;

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++)
		{
			if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return false;
			AtoH(token, (&mac[i]), 1);
		}
		if(i != 6)
			return false;

		DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x",
								PRINT_MAC(mac), tid));

		pEntry = MacTableLookup(pAd, (u8 *) mac);

		if (pEntry) {
		    DBGPRINT(RT_DEBUG_OFF, ("\nTear down Rec BA Session: Tid = %d\n", tid));
		    BARecSessionTearDown(pAd, pEntry->wcid, tid, false);
		}

		return true;
	}

	return false;

}

INT	Set_HtBw_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG HtBw;

	HtBw = simple_strtol(arg, 0, 10);

	if (HtBw == BW_40)
		pAd->CommonCfg.RegTransmitSetting.field.BW  = BW_40;
	else if (HtBw == BW_20)
		pAd->CommonCfg.RegTransmitSetting.field.BW  = BW_20;
	else
		return false;  /*Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtBw_Proc::(HtBw=%d)\n", pAd->CommonCfg.RegTransmitSetting.field.BW));

#ifdef DFS_ATP_SUPPORT
	pAd->CommonCfg.RadarDetect.atp_set_ht_bw = true;
	if (pAd->CommonCfg.RadarDetect.atp_set_channel_ready) {
		printk ("BW and Channel is ready\n");
	}
#endif /* DFS_ATP_SUPPORT */

	return true;
}


INT	Set_HtMcs_Proc(struct rtmp_adapter *pAd, char *arg)
{
#ifdef CONFIG_AP_SUPPORT
	struct os_cookie *pObj = pAd->OS_Cookie;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	bool bAutoRate = false;
#endif /* CONFIG_STA_SUPPORT */
	u8 HtMcs = MCS_AUTO, Mcs_tmp, ValidMcs = 15;
	char *mcs_str, *ss_str;
	u8 ss = 0, mcs = 0;
	struct rtmp_wifi_dev *wdev;

	ss_str = arg;
	if ((mcs_str = rtstrchr(arg, ':'))!= NULL)
	{
		*mcs_str = 0;
		mcs_str++;

		DBGPRINT(RT_DEBUG_TRACE, ("%s(): ss_str=%s, mcs_str=%s\n",
						__FUNCTION__, ss_str, mcs_str));

		if (strlen(ss_str) && strlen(mcs_str)) {
			mcs = simple_strtol(mcs_str, 0, 10);
			ss = simple_strtol(ss_str, 0, 10);

			if ((ss <= pAd->CommonCfg.TxStream) && (mcs <= 9))
				HtMcs = ((ss - 1) <<4) | mcs;
			else {
				HtMcs = MCS_AUTO;
				ss = 0;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("%s(): %dSS-MCS%d, Auto=%s\n",
						__FUNCTION__, ss, mcs,
						(HtMcs == MCS_AUTO && ss == 0) ? "true" : "false"));
			Set_FixedTxMode_Proc(pAd, "VHT");
		}
	}
	else
	{

		Mcs_tmp = simple_strtol(arg, 0, 10);
		if (Mcs_tmp <= ValidMcs || Mcs_tmp == 32)
			HtMcs = Mcs_tmp;
		else
			HtMcs = MCS_AUTO;
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;

		wdev->DesiredTransmitSetting.field.MCS = HtMcs;
		DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMcs_Proc::(HtMcs=%d) for ra%d\n",
				wdev->DesiredTransmitSetting.field.MCS, pObj->ioctl_if));
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		wdev = &pAd->StaCfg.wdev;

		wdev->DesiredTransmitSetting.field.MCS = HtMcs;
		wdev->bAutoTxRateSwitch = (HtMcs == MCS_AUTO) ? true:false;
		DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMcs_Proc::(HtMcs=%d, bAutoTxRateSwitch = %d)\n",
					wdev->DesiredTransmitSetting.field.MCS, wdev->bAutoTxRateSwitch));

		if ((!WMODE_CAP_N(pAd->CommonCfg.PhyMode)) ||
			(pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.MODE < MODE_HTMIX))
		{
			if ((wdev->DesiredTransmitSetting.field.MCS != MCS_AUTO) &&
				(HtMcs <= 3) &&
				(wdev->DesiredTransmitSetting.field.FixedTxMode == FIXED_TXMODE_CCK))
			{
				RTMPSetDesiredRates(pAd, (LONG) (RateIdToMbps[HtMcs] * 1000000));
			}
			else if ((wdev->DesiredTransmitSetting.field.MCS != MCS_AUTO) &&
					(HtMcs <= 7) &&
	            			(wdev->DesiredTransmitSetting.field.FixedTxMode == FIXED_TXMODE_OFDM))
			{
				RTMPSetDesiredRates(pAd, (LONG) (RateIdToMbps[HtMcs+4] * 1000000));
			}
			else
				bAutoRate = true;

			if (bAutoRate)
			{
				wdev->DesiredTransmitSetting.field.MCS = MCS_AUTO;
				RTMPSetDesiredRates(pAd, -1);
			}
	        DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMcs_Proc::(FixedTxMode=%d)\n",
					wdev->DesiredTransmitSetting.field.FixedTxMode));
		}
        if (ADHOC_ON(pAd))
            return true;
	}
#endif /* CONFIG_STA_SUPPORT */

	SetCommonHT(pAd);

#ifdef WFA_VHT_PF
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		INT idx;

		RTMP_SEM_LOCK(&pAd->MacTabLock);
		for (idx = 1; idx < MAX_LEN_OF_MAC_TABLE; idx++)
		{
			MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[idx];

			if (IS_ENTRY_CLIENT(pEntry) && (pEntry->apidx == pObj->ioctl_if)) {
				if ((HtMcs == MCS_AUTO) &&  ss == 0) {
					u8 TableSize = 0;

					MlmeSelectTxRateTable(pAd, pEntry, &pEntry->pTable, &TableSize, &pEntry->CurrTxRateIndex);
					MlmeNewTxRate(pAd, pEntry);

					pEntry->bAutoTxRateSwitch = true;

#ifdef NEW_RATE_ADAPT_SUPPORT
					if (! ADAPT_RATE_TABLE(pEntry->pTable))
#endif /* NEW_RATE_ADAPT_SUPPORT */
						pEntry->HTPhyMode.field.ShortGI = GI_800;
				}
				else
				{
					pEntry->HTPhyMode.field.MCS = pMbss->HTPhyMode.field.MCS;
					pEntry->bAutoTxRateSwitch = false;

					/* If the legacy mode is set, overwrite the transmit setting of this entry. */
					RTMPUpdateLegacyTxSetting((u8)pMbss->DesiredTransmitSetting.field.FixedTxMode, pEntry);
				}
			}
		}
		RTMP_SEM_UNLOCK(&pAd->MacTabLock);
	}
#endif /* CONFIG_AP_SUPPORT */
#endif /* WFA_VHT_PF */

	return true;
}

INT	Set_HtGi_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG HtGi;

	HtGi = simple_strtol(arg, 0, 10);

	if ( HtGi == GI_400)
		pAd->CommonCfg.RegTransmitSetting.field.ShortGI = GI_400;
	else if ( HtGi == GI_800 )
		pAd->CommonCfg.RegTransmitSetting.field.ShortGI = GI_800;
	else
		return false; /* Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtGi_Proc::(ShortGI=%d)\n",pAd->CommonCfg.RegTransmitSetting.field.ShortGI));

	return true;
}


INT	Set_HtTxBASize_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	u8 Size;

	Size = simple_strtol(arg, 0, 10);

	if (Size <=0 || Size >=64)
	{
		Size = 8;
	}
	pAd->CommonCfg.TxBASize = Size-1;
	DBGPRINT(RT_DEBUG_ERROR, ("Set_HtTxBASize ::(TxBASize= %d)\n", Size));

	return true;
}

INT	Set_HtDisallowTKIP_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value == 1)
	{
		pAd->CommonCfg.HT_DisallowTKIP = true;
	}
	else
	{
		pAd->CommonCfg.HT_DisallowTKIP = false;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtDisallowTKIP_Proc ::%s\n",
				(pAd->CommonCfg.HT_DisallowTKIP == true) ? "enabled" : "disabled"));

	return true;
}

INT	Set_HtOpMode_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{

	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value == HTMODE_GF)
		pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_GF;
	else if ( Value == HTMODE_MM )
		pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_MM;
	else
		return false; /*Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtOpMode_Proc::(HtOpMode=%d)\n",pAd->CommonCfg.RegTransmitSetting.field.HTMODE));

	return true;

}

INT	Set_HtStbc_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{

	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value == STBC_USE)
		pAd->CommonCfg.RegTransmitSetting.field.STBC = STBC_USE;
	else if ( Value == STBC_NONE )
		pAd->CommonCfg.RegTransmitSetting.field.STBC = STBC_NONE;
	else
		return false; /*Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_Stbc_Proc::(HtStbc=%d)\n",pAd->CommonCfg.RegTransmitSetting.field.STBC));

	return true;
}

INT	Set_HtHtc_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{

	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->HTCEnable = false;
	else if ( Value ==1 )
        pAd->HTCEnable = true;
	else
		return false; /*Invalid argument 	*/

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtHtc_Proc::(HtHtc=%d)\n",pAd->HTCEnable));

	return true;
}

INT	Set_HtExtcha_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{

	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value == 0)
		pAd->CommonCfg.RegTransmitSetting.field.EXTCHA  = EXTCHA_BELOW;
	else if ( Value ==1 )
		pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = EXTCHA_ABOVE;
	else
		return false; /*Invalid argument 	*/

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtExtcha_Proc::(HtExtcha=%d)\n",pAd->CommonCfg.RegTransmitSetting.field.EXTCHA));

	return true;
}

INT	Set_HtMpduDensity_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value <=7)
		pAd->CommonCfg.BACapability.field.MpduDensity = Value;
	else
		pAd->CommonCfg.BACapability.field.MpduDensity = 4;

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMpduDensity_Proc::(HtMpduDensity=%d)\n",pAd->CommonCfg.BACapability.field.MpduDensity));

	return true;
}

INT	Set_HtBaWinSize_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

#ifdef CONFIG_AP_SUPPORT
	/* Intel IOT*/
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		Value = 64;
#endif /* CONFIG_AP_SUPPORT */

	if (Value >=1 && Value <= 64)
	{
		pAd->CommonCfg.REGBACapability.field.RxBAWinLimit = Value;
		pAd->CommonCfg.BACapability.field.RxBAWinLimit = Value;
	}
	else
	{
        pAd->CommonCfg.REGBACapability.field.RxBAWinLimit = 64;
		pAd->CommonCfg.BACapability.field.RxBAWinLimit = 64;
	}

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtBaWinSize_Proc::(HtBaWinSize=%d)\n",pAd->CommonCfg.BACapability.field.RxBAWinLimit));

	return true;
}

INT	Set_HtRdg_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value == 0)
		pAd->CommonCfg.bRdg = false;
	else if ( Value ==1 )
	{
		pAd->HTCEnable = true;
        pAd->CommonCfg.bRdg = true;
	}
	else
		return false; /*Invalid argument*/

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtRdg_Proc::(HtRdg=%d)\n",pAd->CommonCfg.bRdg));

	return true;
}

INT	Set_HtLinkAdapt_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->bLinkAdapt = false;
	else if ( Value ==1 )
	{
			pAd->HTCEnable = true;
			pAd->bLinkAdapt = true;
	}
	else
		return false; /*Invalid argument*/

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtLinkAdapt_Proc::(HtLinkAdapt=%d)\n",pAd->bLinkAdapt));

	return true;
}

INT	Set_HtAmsdu_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	pAd->CommonCfg.BACapability.field.AmsduEnable = (Value == 0) ? false : true;
	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtAmsdu_Proc::(HtAmsdu=%d)\n",pAd->CommonCfg.BACapability.field.AmsduEnable));

	return true;
}

INT	Set_HtAutoBa_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
	{
		pAd->CommonCfg.BACapability.field.AutoBA = false;
		pAd->CommonCfg.BACapability.field.Policy = BA_NOTUSE;
	}
    else if (Value == 1)
    {
		pAd->CommonCfg.BACapability.field.AutoBA = true;
		pAd->CommonCfg.BACapability.field.Policy = IMMED_BA;
    }
	else
		return false; /*Invalid argument*/

    pAd->CommonCfg.REGBACapability.field.AutoBA = pAd->CommonCfg.BACapability.field.AutoBA;
	pAd->CommonCfg.REGBACapability.field.Policy = pAd->CommonCfg.BACapability.field.Policy;
	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtAutoBa_Proc::(HtAutoBa=%d)\n",pAd->CommonCfg.BACapability.field.AutoBA));

	return true;

}

INT	Set_HtProtect_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->CommonCfg.bHTProtect = false;
    else if (Value == 1)
		pAd->CommonCfg.bHTProtect = true;
	else
		return false; /*Invalid argument*/

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtProtect_Proc::(HtProtect=%d)\n",pAd->CommonCfg.bHTProtect));

	return true;
}

INT	Set_SendSMPSAction_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
    u8 mac[6], mode;
	char *token;
	STRING sepValue[] = ":", DASH = '-';
	INT i;
    MAC_TABLE_ENTRY *pEntry;

    /*DBGPRINT(RT_DEBUG_TRACE,("\n%s\n", arg));*/
/*
	The BARecTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d,
		=>The six 2 digit hex-decimal number previous are the Mac address,
		=>The seventh decimal number is the mode value.
*/
    if(strlen(arg) < 19)  /*Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and mode value in decimal format.*/
		return false;

   	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token)>1))
	{
		mode = simple_strtol((token+1), 0, 10);
		if (mode > MMPS_DISABLE)
			return false;

		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++)
		{
			if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return false;
			AtoH(token, (&mac[i]), 1);
		}
		if(i != 6)
			return false;

		DBGPRINT(RT_DEBUG_OFF, ("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x",
					PRINT_MAC(mac), mode));

		pEntry = MacTableLookup(pAd, mac);

		if (pEntry) {
		    DBGPRINT(RT_DEBUG_OFF, ("\nSendSMPSAction SMPS mode = %d\n", mode));
		    SendSMPSAction(pAd, pEntry->wcid, mode);
		}

		return true;
	}

	return false;


}

INT	Set_HtMIMOPSmode_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value <=3)
		pAd->CommonCfg.BACapability.field.MMPSmode = Value;
	else
		pAd->CommonCfg.BACapability.field.MMPSmode = 3;

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMIMOPSmode_Proc::(MIMOPS mode=%d)\n",pAd->CommonCfg.BACapability.field.MMPSmode));

	return true;
}

#ifdef CONFIG_AP_SUPPORT
/*
    ==========================================================================
    Description:
        Set Tx Stream number
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_HtTxStream_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG 	Value;

	Value = simple_strtol(arg, 0, 10);

	if ((Value <= 3) && (Value >= 1) && (Value <= pAd->Antenna.field.TxPath)) /* 3*3*/
		pAd->CommonCfg.TxStream = Value;
	else
		pAd->CommonCfg.TxStream = pAd->Antenna.field.TxPath;

	if ((pAd->MACVersion < RALINK_2883_VERSION) &&
		(pAd->CommonCfg.TxStream > 2))
	{
		pAd->CommonCfg.TxStream = 2; /* only 2 TX streams for RT2860 series*/
	}

	SetCommonHT(pAd);

	APStop(pAd);
	APStartUp(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtTxStream_Proc::(Tx Stream=%d)\n",pAd->CommonCfg.TxStream));

	return true;
}

/*
    ==========================================================================
    Description:
        Set Rx Stream number
    Return:
        true if all parameters are OK, false otherwise
    ==========================================================================
*/
INT	Set_HtRxStream_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG 	Value;

	Value = simple_strtol(arg, 0, 10);

	if ((Value <= 3) && (Value >= 1) && (Value <= pAd->Antenna.field.RxPath))
		pAd->CommonCfg.RxStream = Value;
	else
		pAd->CommonCfg.RxStream = pAd->Antenna.field.RxPath;

	if ((pAd->MACVersion < RALINK_2883_VERSION) &&
		(pAd->CommonCfg.RxStream > 2)) /* 3*3*/
	{
		pAd->CommonCfg.RxStream = 2; /* only 2 RX streams for RT2860 series*/
	}

	SetCommonHT(pAd);

	APStop(pAd);
	APStartUp(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtRxStream_Proc::(Rx Stream=%d)\n",pAd->CommonCfg.RxStream));

	return true;
}

#endif /* CONFIG_AP_SUPPORT */

INT	Set_ForceShortGI_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->WIFItestbed.bShortGI = false;
	else if (Value == 1)
		pAd->WIFItestbed.bShortGI = true;
	else
		return false; /*Invalid argument*/

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_ForceShortGI_Proc::(ForceShortGI=%d)\n", pAd->WIFItestbed.bShortGI));

	return true;
}



INT	Set_ForceGF_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->WIFItestbed.bGreenField = false;
	else if (Value == 1)
		pAd->WIFItestbed.bGreenField = true;
	else
		return false; /*Invalid argument*/

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_ForceGF_Proc::(ForceGF=%d)\n", pAd->WIFItestbed.bGreenField));

	return true;
}

INT	Set_HtMimoPs_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->CommonCfg.bMIMOPSEnable = false;
	else if (Value == 1)
		pAd->CommonCfg.bMIMOPSEnable = true;
	else
		return false; /*Invalid argument*/

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMimoPs_Proc::(HtMimoPs=%d)\n",pAd->CommonCfg.bMIMOPSEnable));

	return true;
}


INT Set_HT_BssCoex_Proc(
	IN	struct rtmp_adapter *	pAd,
	IN	char *			pParam)
{
	u8 bBssCoexEnable = simple_strtol(pParam, 0, 10);

	pAd->CommonCfg.bBssCoexEnable = ((bBssCoexEnable == 1) ? true: false);

	DBGPRINT(RT_DEBUG_TRACE, ("Set bBssCoexEnable=%d!\n", pAd->CommonCfg.bBssCoexEnable));

	if ((pAd->CommonCfg.bBssCoexEnable == false)
			&& pAd->CommonCfg.bRcvBSSWidthTriggerEvents)
	{
		/* switch back 20/40 */
		DBGPRINT(RT_DEBUG_TRACE, ("Set bBssCoexEnable:  Switch back 20/40. \n"));
		pAd->CommonCfg.bRcvBSSWidthTriggerEvents = false;
		if ((pAd->CommonCfg.Channel <=14) && (pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth == BW_40))
		{
			pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth = 1;
			pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset = pAd->CommonCfg.RegTransmitSetting.field.EXTCHA;
		}
	}
	return true;
}


INT Set_HT_BssCoexApCntThr_Proc(
	IN	struct rtmp_adapter *	pAd,
	IN	char *			pParam)
{
	pAd->CommonCfg.BssCoexApCntThr = simple_strtol(pParam, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("Set BssCoexApCntThr=%d!\n", pAd->CommonCfg.BssCoexApCntThr));

	return true;
}



INT	Set_VhtBw_Proc(
	IN struct rtmp_adapter *pAd,
	IN char *arg)
{
	ULONG vht_cw;
	u8 cent_ch;
	vht_cw = simple_strtol(arg, 0, 10);


	if (vht_cw == VHT_BW_80)
		pAd->CommonCfg.vht_bw = VHT_BW_80;
	else
		pAd->CommonCfg.vht_bw = VHT_BW_2040;

	if (!WMODE_CAP_AC(pAd->CommonCfg.PhyMode))
		goto direct_done;

	SetCommonHT(pAd);
	if(pAd->CommonCfg.BBPCurrentBW == BW_80)
		cent_ch = pAd->CommonCfg.vht_cent_ch;
	else
		cent_ch = pAd->CommonCfg.CentralChannel;

	AsicSwitchChannel(pAd, cent_ch, false);
	AsicLockChannel(pAd, cent_ch);

	DBGPRINT(RT_DEBUG_TRACE, ("BW_%s, PrimaryChannel(%d), %s CentralChannel = %d, apply it immediately\n",
						(pAd->CommonCfg.BBPCurrentBW == BW_80 ? "80":
							(pAd->CommonCfg.BBPCurrentBW == BW_40 ? "40" :
							"20")),
						pAd->CommonCfg.Channel,
						(pAd->CommonCfg.BBPCurrentBW == BW_80 ? "VHT" : "HT"), cent_ch));

direct_done:
	DBGPRINT(RT_DEBUG_TRACE, ("Set_VhtBw_Proc::(VHT_BW=%d)\n", pAd->CommonCfg.vht_bw));

#ifdef DFS_ATP_SUPPORT
	pAd->CommonCfg.RadarDetect.atp_set_vht_bw = true;
	if (pAd->CommonCfg.RadarDetect.atp_set_channel_ready) {
		printk ("BW and Channel is ready\n");
	}
#endif /* DFS_ATP_SUPPORT */

	return true;
}


INT set_VhtBwSignal_Proc(struct rtmp_adapter *pAd, char *arg)
{
	ULONG bw_signal = simple_strtol(arg, 0, 10);

	if (bw_signal <= 2)
		pAd->CommonCfg.vht_bw_signal = bw_signal;
	else
		pAd->CommonCfg.vht_bw_signal = BW_SIGNALING_DISABLE;
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): vht_bw_signal=%d(%s)\n",
				__FUNCTION__, pAd->CommonCfg.vht_bw_signal,
				(pAd->CommonCfg.vht_bw_signal == BW_SIGNALING_DYNAMIC ? "Dynamic" :
				(pAd->CommonCfg.vht_bw_signal == BW_SIGNALING_STATIC ? "Static" : "Disable"))));

	return true;
}


INT	Set_VhtStbc_Proc(
	IN struct rtmp_adapter *pAd,
	IN char *arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value == STBC_USE)
		pAd->CommonCfg.vht_stbc = STBC_USE;
	else if ( Value == STBC_NONE )
		pAd->CommonCfg.vht_stbc = STBC_NONE;
	else
		return false; /*Invalid argument */

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_VhtStbc_Proc::(VhtStbc=%d)\n", pAd->CommonCfg.vht_stbc));

	return true;
}

INT	Set_VhtDisallowNonVHT_Proc(
	IN struct rtmp_adapter *pAd,
	IN char *arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (Value == 0)
		pAd->CommonCfg.bNonVhtDisallow = false;
	else
		pAd->CommonCfg.bNonVhtDisallow = true;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_VhtDisallowNonVHT_Proc::(bNonVhtDisallow=%d)\n", pAd->CommonCfg.bNonVhtDisallow));

	return true;
}



INT	Set_FixedTxMode_Proc(struct rtmp_adapter *pAd, char *arg)
{
	struct rtmp_wifi_dev *wdev = NULL;
#ifdef CONFIG_AP_SUPPORT
    struct os_cookie *pObj = pAd->OS_Cookie;
#endif /* CONFIG_AP_SUPPORT */
	INT	fix_tx_mode = RT_CfgSetFixedTxPhyMode(arg);


#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;
#endif /* CONFIG_STA_SUPPORT */

	if (wdev)
		wdev->DesiredTransmitSetting.field.FixedTxMode = fix_tx_mode;

	DBGPRINT(RT_DEBUG_TRACE, ("%s():(FixedTxMode=%d)\n",
								__FUNCTION__, fix_tx_mode));

	return true;
}

#ifdef CONFIG_APSTA_MIXED_SUPPORT
INT	Set_OpMode_Proc(struct rtmp_adapter *pAd, char *arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Can not switch operate mode on interface up !! \n"));
		return false;
	}

	if (Value == 0)
		pAd->OpMode = OPMODE_STA;
	else if (Value == 1)
		pAd->OpMode = OPMODE_AP;
	else
		return false; /*Invalid argument*/

	DBGPRINT(RT_DEBUG_TRACE, ("Set_OpMode_Proc::(OpMode=%s)\n", pAd->OpMode == 1 ? "AP Mode" : "STA Mode"));

	return true;
}
#endif /* CONFIG_APSTA_MIXED_SUPPORT */


#ifdef DBG_CTRL_SUPPORT
#ifdef INCLUDE_DEBUG_QUEUE
/* ---------------------- DEBUG QUEUE ------------------------*/

#define DBQ_LENGTH	512
#define DBQ_DATA_LENGTH	8

/* Define to include TX and RX HT Control field in log */
/* #define DBQ_INCLUDE_HTC */

typedef
struct {
	u8 type;					/* type of data*/
	ULONG timestamp;			/* sec/usec timestamp from gettimeofday*/
	u8 data[DBQ_DATA_LENGTH];	/* data*/
} DBQUEUE_ENTRY;

/* Type field definitions */
#define DBQ_TYPE_EMPTY	0
#define DBQ_TYPE_TXWI	0x70		/* TXWI*/
#define DBQ_TYPE_TXHDR	0x72		/* TX Header*/
#define DBQ_TYPE_TXFIFO	0x73		/* TX Stat FIFO*/
#define DBQ_TYPE_RXWI	0x78		/* RXWI uses 0x78 to 0x7A for 5 longs*/
#define DBQ_TYPE_RXHDR	0x7B		/* RX Header*/
#define DBQ_TYPE_TXQHTC	0x7c		/* RX Qos+HT Control field*/
#define DBQ_TYPE_RXQHTC	0x7d		/* RX Qos+HT Control field */
#define DBQ_TYPE_RALOG	0x7e		/* RA Log */

#define DBQ_INIT_SIG	0x4442484E	/* 'DBIN' - dbqInit initialized flag*/
#define DBQ_ENA_SIG		0x4442454E	/* 'DBEN' - dbqEnable enabled flag*/

static DBQUEUE_ENTRY dbQueue[DBQ_LENGTH];
static ULONG dbqTail=0;
static ULONG dbqEnable=0;
static ULONG dbqInit=0;

/* dbQueueInit - initialize Debug Queue variables and clear the queue*/
void dbQueueInit(void)
{
	int i;

	for (i=0; i<DBQ_LENGTH; i++)
		dbQueue[i].type = DBQ_TYPE_EMPTY;
	dbqTail = 0;
	dbqInit = DBQ_INIT_SIG;
}

/* dbQueueEnqueue - enqueue data*/
void dbQueueEnqueue(u8 type, u8 *data)
{
	DBQUEUE_ENTRY *oldTail;
	struct timeval tval;

	if (dbqEnable!=DBQ_ENA_SIG || data==NULL)
		return;

	if (dbqInit!=DBQ_INIT_SIG || dbqTail>=DBQ_LENGTH)
		dbQueueInit();

	oldTail = &dbQueue[dbqTail];

	/* Advance tail and mark as empty*/
	if (dbqTail >= DBQ_LENGTH-1)
		dbqTail = 0;
	else
		dbqTail++;
	dbQueue[dbqTail].type = DBQ_TYPE_EMPTY;

	/* Enqueue data*/
	oldTail->type = type;
	do_gettimeofday(&tval);
	oldTail->timestamp = tval.tv_sec*1000000L + tval.tv_usec;
	memcpy(oldTail->data, data, DBQ_DATA_LENGTH);
}

void dbQueueEnqueueTxFrame(u8 *pTxWI, u8 *pHeader_802_11)
{
	dbQueueEnqueue(DBQ_TYPE_TXWI, pTxWI);

	/* 802.11 Header */
	if (pHeader_802_11 != NULL) {
		dbQueueEnqueue(DBQ_TYPE_TXHDR, pHeader_802_11);
#ifdef DBQ_INCLUDE_HTC
		/* Qos+HT Control field */
		if ((pHeader_802_11[0] & 0x08) && (pHeader_802_11[1] & 0x80))
			dbQueueEnqueue(DBQ_TYPE_TXQHTC, pHeader_802_11+24);
#endif /* DBQ_INCLUDE_HTC */
	}
}

void dbQueueEnqueueRxFrame(u8 *pRxWI, u8 *pHeader_802_11, ULONG flags)
{
	/* Ignore Beacons if disabled */
	if ((flags & DBF_DBQ_NO_BCN) && (pHeader_802_11[0] & 0xfc)==0x80)
		return;

	/* RXWI */
	dbQueueEnqueue(DBQ_TYPE_RXWI, pRxWI);
	if (flags & DBF_DBQ_RXWI_FULL) {
		dbQueueEnqueue(DBQ_TYPE_RXWI+1, pRxWI+8);
		dbQueueEnqueue(DBQ_TYPE_RXWI+2, pRxWI+16);
	}

	/* 802.11 Header */
	dbQueueEnqueue(DBQ_TYPE_RXHDR, (u8 *)pHeader_802_11);

#ifdef DBQ_INCLUDE_HTC
	/* Qos+HT Control field */
	if ((pHeader_802_11[0] & 0x08) &&
		(pHeader_802_11[1] & 0x80))
		dbQueueEnqueue(DBQ_TYPE_RXQHTC, pHeader_802_11+24);
#endif /* DBQ_INCLUDE_HTC */
}


/* dbQueueDisplayPhy - Display PHY rate */
static void dbQueueDisplayPHY(unsigned short phyRate)
{
	static CHAR *mode[4] = {" C", "oM","mM", "gM"};

	DBGPRINT(RT_DEBUG_OFF, ("%2s%02d %c%c%c%c",
		//(phyRate>>8) & 0xFF, phyRate & 0xFF,
		mode[(phyRate>>14) & 0x3],							// Mode: c, o, m, g
		phyRate & 0x7F,										// MCS
		(phyRate & 0x0100)? 'S': 'L',						// Guard Int: S or L
		(phyRate & 0x0080)? '4': '2',						// BW: 4 or 2
		(phyRate & 0x0200)? 'S': 's',						// STBC:  S or s
		(phyRate & 0x2000)? 'I': ((phyRate & 0x800)? 'E': '_')	// Beamforming:  E or I or _
		) );
}

/* dbQueueDump - dump contents of debug queue*/
static void dbQueueDump(
	IN  struct rtmp_adapter *  pAd,
	bool decode)
{
	DBQUEUE_ENTRY *oldTail;
	int i, origMCS, succMCS;
	ULONG lastTimestamp=0;
	bool showTimestamp;
	unsigned short phyRate;

	if (dbqInit!=DBQ_INIT_SIG || dbqTail>=DBQ_LENGTH)
		return;

	oldTail = &dbQueue[dbqTail];

	for (i=0; i<DBQ_LENGTH; i++) {
		if (++oldTail >= &dbQueue[DBQ_LENGTH])
			oldTail = dbQueue;

		/* Skip empty entries*/
		if (oldTail->type == DBQ_TYPE_EMPTY)
			continue;

		showTimestamp = false;

		switch (oldTail->type) {
		case 0x70:	/* TXWI - 2 longs, MSB to LSB */
		case 0x78:	/* RXWI - 2 longs, MSB to LSB */
			showTimestamp = true;

			if (decode && oldTail->type==0x70) {
				DBGPRINT(RT_DEBUG_OFF, ("\nTxWI ") );
				dbQueueDisplayPHY(oldTail->data[3]*256 + oldTail->data[2]);
				DBGPRINT(RT_DEBUG_OFF, ("%c s=%03X %02X %s-",
						(oldTail->data[0] & 0x10)? 'A': '_',				// AMPDU
						(oldTail->data[7]*256 + oldTail->data[6]) & 0xFFF,	// Size
						oldTail->data[5],									// WCID
						(oldTail->data[4] & 0x01)? "AK": "NA" ));			// ACK/NACK
			}
			else if (decode && oldTail->type==0x78) {
				DBGPRINT(RT_DEBUG_OFF, ("\nRxWI ") );
				dbQueueDisplayPHY(oldTail->data[7]*256 + oldTail->data[6]);
				DBGPRINT(RT_DEBUG_OFF, (" s=%03X %02X %02X%01X-",
						(oldTail->data[3]*256 + oldTail->data[2]) & 0xFFF,	// Size
						oldTail->data[0],									// WCID
						oldTail->data[5], oldTail->data[4]>>4 ));			// Seq Number
			}
			else
				DBGPRINT(RT_DEBUG_OFF, ("\n%cxWI %02X%02X %02X%02X-%02X%02X %02X%02X----",
					oldTail->type==0x70? 'T': 'R',
					oldTail->data[3], oldTail->data[2], oldTail->data[1], oldTail->data[0],
					oldTail->data[7], oldTail->data[6], oldTail->data[5], oldTail->data[4]) );
			break;

		case 0x79:	/* RXWI - next 2 longs, MSB to LSB */
			if (decode) {
				DBGPRINT(RT_DEBUG_OFF, ("Rx2  %2d %2d %2d S:%d %d %d ",
						ConvertToRssi(pAd, (CHAR)oldTail->data[0], RSSI_0),
						ConvertToRssi(pAd, (CHAR)oldTail->data[1], RSSI_1),
						ConvertToRssi(pAd, (CHAR)oldTail->data[2], RSSI_2),
						(oldTail->data[4]*3 + 8)/16,
						(oldTail->data[5]*3 + 8)/16,
						(oldTail->data[6]*3 + 8)/16) );
			}
			else
				DBGPRINT(RT_DEBUG_OFF, ("Rx2  %02X%02X %02X%02X-%02X%02X %02X%02X    ",
						oldTail->data[3], oldTail->data[2], oldTail->data[1], oldTail->data[0],
						oldTail->data[7], oldTail->data[6], oldTail->data[5], oldTail->data[4]) );
			break;


		case 0x7c:	/* TX HTC+QoS, 6 bytes, MSB to LSB */
		case 0x7d:	/* RX HTC+QoS, 6 bytes, MSB to LSB */
			DBGPRINT(RT_DEBUG_OFF, ("%cxHTC  H:%02X%02X%02X%02X Q:%02X%02X   ",
					oldTail->type==0x7c? 'T': 'R',
					oldTail->data[5], oldTail->data[4], oldTail->data[3], oldTail->data[2],
					oldTail->data[1], oldTail->data[0]) );
			break;

		case 0x72:	/* Tx 802.11 header, MSB to LSB, translate type/subtype*/
		case 0x7b:	/* Rx*/
			{
			u8 tCode;
			struct _typeTableEntry {
				u8 code;	/* Type/subtype*/
				CHAR  str[4];
			} *pTab;
			static struct _typeTableEntry typeTable[] = {
				{0x00, "mARq"}, {0x01, "mARp"}, {0x02, "mRRq"}, {0x03, "mRRp"},
				{0x04, "mPRq"}, {0x05, "mPRp"}, {0x08, "mBcn"}, {0x09, "mATI"},
				{0x0a, "mDis"}, {0x0b, "mAut"}, {0x0c, "mDAu"}, {0x0d, "mAct"},
				{0x0e, "mANA"},
				{0x17, "cCWr"}, {0x18, "cBAR"}, {0x19, "cBAc"}, {0x1a, "cPSP"},
				{0x1b, "cRTS"}, {0x1c, "cCTS"}, {0x1d, "cACK"}, {0x1e, "cCFE"},
				{0x1f, "cCEA"},
				{0x20, "dDat"}, {0x21, "dDCA"}, {0x22, "dDCP"}, {0x23, "dDAP"},
				{0x24, "dNul"}, {0x25, "dCFA"}, {0x26, "dCFP"}, {0x27, "dCAP"},
				{0x28, "dQDa"}, {0x29, "dQCA"}, {0x2a, "dQCP"}, {0x2b, "dQAP"},
				{0x2c, "dQNu"}, {0x2e, "dQNP"}, {0x2f, "dQNA"},
				{0xFF, "RESV"}};

			tCode = ((oldTail->data[0]<<2) & 0x30) | ((oldTail->data[0]>>4) & 0xF);
			for (pTab=typeTable; pTab->code!=0xFF; pTab++) {
				if (pTab->code == tCode)
					break;
			}

			DBGPRINT(RT_DEBUG_OFF, ("%cxH  %c%c%c%c [%02X%02X %02X%02X]       \n",
					oldTail->type==0x72? 'T': 'R',
					pTab->str[0], pTab->str[1], pTab->str[2], pTab->str[3],
					oldTail->data[3], oldTail->data[2], oldTail->data[1], oldTail->data[0]) );
			}
			break;

		case 0x73:	/* TX STAT FIFO*/
			showTimestamp = true;

			/* origMCS is limited to 4 bits. Check for case of MCS16 to 23*/
			origMCS = (oldTail->data[0]>>1) & 0xF;
			succMCS = (oldTail->data[2] & 0x7F);
			if (succMCS>origMCS && origMCS<8)
				origMCS += 16;
			phyRate = (oldTail->data[3]<<8) + oldTail->data[2];

			DBGPRINT(RT_DEBUG_OFF, ("TxFI %02X%02X%02X%02X=%c%c%c%c%c M%02d/%02d%c%c",
					oldTail->data[3], oldTail->data[2],
					oldTail->data[1], oldTail->data[0],
					(phyRate & 0x0100)? 'S': 'L',				/* Guard Int:    S or L */
					(phyRate & 0x0080)? '4': '2',				/* BW: 			 4 or 2 */
					(phyRate & 0x0200)? 'S': 's',				/* STBC:         S or s */
					(phyRate & 0x2000)? 'I': ((phyRate & 0x0800)? 'E': '_'), /* Beamforming:  E or I or _ */
					(oldTail->data[0] & 0x40)? 'A': '_',		/* Aggregated:   A or _ */
					succMCS, origMCS,							/* MCS:          <Final>/<orig> */
					succMCS==origMCS? ' ': '*',					/* Retry:        '*' if MCS doesn't match */
					(oldTail->data[0] & 0x20)? ' ': 'F') );		/* Success/Fail  _ or F */
			break;
		case 0x7E:	/* RA Log info */
			{
				struct {unsigned short phy; unsigned short per; unsigned short tp; unsigned short bfPer;} *p = (void*)(oldTail->data);
				DBGPRINT(RT_DEBUG_OFF, ("RALog %02X%02X %d %d %d    ",
											(p->phy>>8) & 0xFF, p->phy & 0xFF, p->per, p->tp, p->bfPer) );
			}
			break;

		default:
			DBGPRINT(RT_DEBUG_OFF, ("%02X   %02X%02X %02X%02X %02X%02X %02X%02X   ", oldTail->type,
					oldTail->data[0], oldTail->data[1], oldTail->data[2], oldTail->data[3],
					oldTail->data[4], oldTail->data[5], oldTail->data[6], oldTail->data[7]) );
			break;
		}

		if (showTimestamp)
		{
			ULONG t = oldTail->timestamp;
			ULONG dt = oldTail->timestamp-lastTimestamp;

			DBGPRINT(RT_DEBUG_OFF, ("%lu.%06lu ", t/1000000L, t % 1000000L) );

			if (dt>999999L)
			DBGPRINT(RT_DEBUG_OFF, ("+%lu.%06lu s\n", dt/1000000L, dt % 1000000L) );
			else
			DBGPRINT(RT_DEBUG_OFF, ("+%lu us\n", dt) );
			lastTimestamp = oldTail->timestamp;
		}
	}
}

/*
	Set_DebugQueue_Proc - Control DBQueue
		iwpriv ra0 set DBQueue=dd.
			dd: 0=>disable, 1=>enable, 2=>dump, 3=>clear, 4=>dump & decode
*/
INT Set_DebugQueue_Proc(
    IN  struct rtmp_adapter *  pAd,
    IN  char *        arg)
{
    ULONG argValue = simple_strtol(arg, 0, 10);

	switch (argValue) {
	case 0:
		dbqEnable = 0;
		break;
	case 1:
		dbqEnable = DBQ_ENA_SIG;
		break;
	case 2:
		dbQueueDump(pAd, false);
		break;
	case 3:
		dbQueueInit();
		break;
	case 4:
		dbQueueDump(pAd, true);
		break;
	default:
		return false;
	}

	return true;
}
#endif /* INCLUDE_DEBUG_QUEUE */
#endif /* DBG_CTRL_SUPPORT */

#ifdef PRE_ANT_SWITCH
/*
	Set_PreAntSwitch_Proc - enable/disable Preamble Antenna Switch
		usage: iwpriv ra0 set PreAntSwitch=[0 | 1]
*/
INT Set_PreAntSwitch_Proc(
    IN  struct rtmp_adapter *  pAd,
    IN  char *        arg)
{
    pAd->CommonCfg.PreAntSwitch = simple_strtol(arg, 0, 10)!=0;
    DBGPRINT(RT_DEBUG_TRACE, ("%s():(PreAntSwitch=%d)\n",
				__FUNCTION__, pAd->CommonCfg.PreAntSwitch));
	return true;
}


/*
	Set_PreAntSwitchRSSI_Proc - set Preamble Antenna Switch RSSI threshold
		usage: iwpriv ra0 set PreAntSwitchRSSI=<RSSI threshold in dBm>
*/
INT Set_PreAntSwitchRSSI_Proc(
    IN  struct rtmp_adapter *  pAd,
    IN  char *        arg)
{
    pAd->CommonCfg.PreAntSwitchRSSI = simple_strtol(arg, 0, 10);
    DBGPRINT(RT_DEBUG_TRACE, ("%s():(PreAntSwitchRSSI=%d)\n",
				__FUNCTION__, pAd->CommonCfg.PreAntSwitchRSSI));
	return true;
}

/*
	Set_PreAntSwitchTimeout_Proc - set Preamble Antenna Switch Timeout threshold
		usage: iwpriv ra0 set PreAntSwitchTimeout=<timeout in seconds, 0=>disabled>
*/
INT Set_PreAntSwitchTimeout_Proc(
    IN  struct rtmp_adapter *  pAd,
    IN  char *        arg)
{
    pAd->CommonCfg.PreAntSwitchTimeout = simple_strtol(arg, 0, 10);
    DBGPRINT(RT_DEBUG_TRACE, ("%s():(PreAntSwitchTimeout=%d)\n",
				__FUNCTION__, pAd->CommonCfg.PreAntSwitchTimeout));
	return true;
}
#endif /* PRE_ANT_SWITCH */




#ifdef CFO_TRACK
/*
	Set_CFOTrack_Proc - enable/disable CFOTrack
		usage: iwpriv ra0 set CFOTrack=[0..8]
*/
INT Set_CFOTrack_Proc(
    IN  struct rtmp_adapter *  pAd,
    IN  char *        arg)
{
    pAd->CommonCfg.CFOTrack = simple_strtol(arg, 0, 10);
    DBGPRINT(RT_DEBUG_TRACE, ("%s():(CFOTrack=%d)\n",
				__FUNCTION__, pAd->CommonCfg.CFOTrack));
	return true;
}
#endif /* CFO_TRACK */


#ifdef DBG_CTRL_SUPPORT
INT Set_DebugFlags_Proc(
    IN  struct rtmp_adapter *  pAd,
    IN  char *        arg)
{
    pAd->CommonCfg.DebugFlags = simple_strtol(arg, 0, 16);

    DBGPRINT(RT_DEBUG_TRACE, ("Set_DebugFlags_Proc::(DebugFlags=%02lX)\n", pAd->CommonCfg.DebugFlags));
	return true;
}
#endif /* DBG_CTRL_SUPPORT */






INT Set_LongRetryLimit_Proc(struct rtmp_adapter *pAd, char *arg)
{
	TX_RTY_CFG_STRUC	tx_rty_cfg;
	u8 LongRetryLimit = (u8)simple_strtol(arg, 0, 10);

	tx_rty_cfg.word = mt7612u_read32(pAd, TX_RTY_CFG);
	tx_rty_cfg.field.LongRtyLimit = LongRetryLimit;
	mt7612u_write32(pAd, TX_RTY_CFG, tx_rty_cfg.word);
	DBGPRINT(RT_DEBUG_TRACE, ("IF Set_LongRetryLimit_Proc::(tx_rty_cfg=0x%x)\n", tx_rty_cfg.word));
	return true;
}

INT Set_ShortRetryLimit_Proc(struct rtmp_adapter *pAd, char *arg)
{
	TX_RTY_CFG_STRUC	tx_rty_cfg;
	u8 ShortRetryLimit = (u8)simple_strtol(arg, 0, 10);

	tx_rty_cfg.word = mt7612u_read32(pAd, TX_RTY_CFG);
	tx_rty_cfg.field.ShortRtyLimit = ShortRetryLimit;
	mt7612u_write32(pAd, TX_RTY_CFG, tx_rty_cfg.word);
	DBGPRINT(RT_DEBUG_TRACE, ("IF Set_ShortRetryLimit_Proc::(tx_rty_cfg=0x%x)\n", tx_rty_cfg.word));
	return true;
}

INT Set_AutoFallBack_Proc(
	IN	struct rtmp_adapter *pAdapter,
	IN	char *		arg)
{
	return RT_CfgSetAutoFallBack(pAdapter, arg);
}



char *RTMPGetRalinkAuthModeStr(
    IN  NDIS_802_11_AUTHENTICATION_MODE authMode)
{
	switch(authMode)
	{
		case Ndis802_11AuthModeOpen:
			return "OPEN";
		case Ndis802_11AuthModeWPAPSK:
			return "WPAPSK";
		case Ndis802_11AuthModeShared:
			return "SHARED";
		case Ndis802_11AuthModeAutoSwitch:
			return "WEPAUTO";
		case Ndis802_11AuthModeWPA:
			return "WPA";
		case Ndis802_11AuthModeWPA2:
			return "WPA2";
		case Ndis802_11AuthModeWPA2PSK:
			return "WPA2PSK";
        case Ndis802_11AuthModeWPA1PSKWPA2PSK:
			return "WPAPSKWPA2PSK";
        case Ndis802_11AuthModeWPA1WPA2:
			return "WPA1WPA2";
		case Ndis802_11AuthModeWPANone:
			return "WPANONE";
		default:
			return "UNKNOW";
	}
}

char *RTMPGetRalinkEncryModeStr(
    IN  unsigned short encryMode)
{
	switch(encryMode)
	{
		case Ndis802_11WEPDisabled:
			return "NONE";
		case Ndis802_11WEPEnabled:
			return "WEP";
		case Ndis802_11TKIPEnable:
			return "TKIP";
		case Ndis802_11AESEnable:
			return "AES";
        case Ndis802_11TKIPAESMix:
			return "TKIPAES";
		default:
			return "UNKNOW";
	}
}


INT	Show_SSID_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	u8 ssid_str[33];


	memset(&ssid_str[0], 0, 33);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		struct os_cookie *pObj = pAd->OS_Cookie;
		memmove(&ssid_str[0],
						pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid,
						pAd->ApCfg.MBSSID[pObj->ioctl_if].SsidLen);
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		memmove(&ssid_str[0],
						pAd->CommonCfg.Ssid,
						pAd->CommonCfg.SsidLen);
	}
#endif /* CONFIG_STA_SUPPORT */

	snprintf(pBuf, BufLen, "\t%s", ssid_str);
	return 0;
}

INT	Show_WirelessMode_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{

	switch(pAd->CommonCfg.PhyMode)
	{
		case (WMODE_B | WMODE_G):
			snprintf(pBuf, BufLen, "\t11B/G");
			break;
		case (WMODE_B):
			snprintf(pBuf, BufLen, "\t11B");
			break;
		case (WMODE_A):
			snprintf(pBuf, BufLen, "\t11A");
			break;
		case (WMODE_A | WMODE_B | WMODE_G):
			snprintf(pBuf, BufLen, "\t11A/B/G");
			break;
		case (WMODE_G):
			snprintf(pBuf, BufLen, "\t11G");
			break;
		case (WMODE_A | WMODE_B | WMODE_G | WMODE_GN | WMODE_AN):
			snprintf(pBuf, BufLen, "\t11A/B/G/N");
			break;
		case (WMODE_GN):
			snprintf(pBuf, BufLen, "\t11N only with 2.4G");
			break;
		case (WMODE_G | WMODE_GN):
			snprintf(pBuf, BufLen, "\t11G/N");
			break;
		case (WMODE_A | WMODE_AN):
			snprintf(pBuf, BufLen, "\t11A/N");
			break;
		case (WMODE_B | WMODE_G | WMODE_GN):
			snprintf(pBuf, BufLen, "\t11B/G/N");
			break;
		case (WMODE_A | WMODE_G | WMODE_GN | WMODE_AN):
			snprintf(pBuf, BufLen, "\t11A/G/N");
			break;
		case (WMODE_AN):
			snprintf(pBuf, BufLen, "\t11N only with 5G");
			break;
		default:
			snprintf(pBuf, BufLen, "\tUnknow Value(%d)", pAd->CommonCfg.PhyMode);
			break;
	}

	return 0;
}


INT	Show_TxBurst_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bEnableTxBurst ? "true":"false");
	return 0;
}

INT	Show_TxPreamble_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	switch(pAd->CommonCfg.TxPreamble)
	{
		case Rt802_11PreambleShort:
			snprintf(pBuf, BufLen, "\tShort");
			break;
		case Rt802_11PreambleLong:
			snprintf(pBuf, BufLen, "\tLong");
			break;
		case Rt802_11PreambleAuto:
			snprintf(pBuf, BufLen, "\tAuto");
			break;
		default:
			snprintf(pBuf, BufLen, "\tUnknown Value(%lu)", pAd->CommonCfg.TxPreamble);
			break;
	}

	return 0;
}

INT	Show_TxPower_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%lu", pAd->CommonCfg.TxPowerPercentage);
	return 0;
}

INT	Show_Channel_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%d", pAd->CommonCfg.Channel);
	return 0;
}

INT	Show_BGProtection_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	switch(pAd->CommonCfg.UseBGProtection)
	{
		case 1: /*Always On*/
			snprintf(pBuf, BufLen, "\tON");
			break;
		case 2: /*Always OFF*/
			snprintf(pBuf, BufLen, "\tOFF");
			break;
		case 0: /*AUTO*/
			snprintf(pBuf, BufLen, "\tAuto");
			break;
		default:
			snprintf(pBuf, BufLen, "\tUnknow Value(%lu)", pAd->CommonCfg.UseBGProtection);
			break;
	}
	return 0;
}

INT	Show_RTSThreshold_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%u", pAd->CommonCfg.RtsThreshold);
	return 0;
}

INT	Show_FragThreshold_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%u", pAd->CommonCfg.FragmentThreshold);
	return 0;
}

INT	Show_HtBw_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	if (pAd->CommonCfg.RegTransmitSetting.field.BW == BW_40)
	{
		snprintf(pBuf, BufLen, "\t40 MHz");
	}
	else
	{
        snprintf(pBuf, BufLen, "\t20 MHz");
	}
	return 0;
}

INT	Show_HtMcs_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	struct rtmp_wifi_dev *wdev = NULL;
#ifdef CONFIG_AP_SUPPORT
	struct os_cookie *pObj = pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;
#endif /* CONFIG_STA_SUPPORT */
	if (wdev)
		snprintf(pBuf, BufLen, "\t%u", wdev->DesiredTransmitSetting.field.MCS);
	return 0;
}

INT	Show_HtGi_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	switch(pAd->CommonCfg.RegTransmitSetting.field.ShortGI)
	{
		case GI_400:
			snprintf(pBuf, BufLen, "\tGI_400");
			break;
		case GI_800:
			snprintf(pBuf, BufLen, "\tGI_800");
			break;
		default:
			snprintf(pBuf, BufLen, "\tUnknow Value(%u)", pAd->CommonCfg.RegTransmitSetting.field.ShortGI);
			break;
	}
	return 0;
}

INT	Show_HtOpMode_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	switch(pAd->CommonCfg.RegTransmitSetting.field.HTMODE)
	{
		case HTMODE_GF:
			snprintf(pBuf, BufLen, "\tGF");
			break;
		case HTMODE_MM:
			snprintf(pBuf, BufLen, "\tMM");
			break;
		default:
			snprintf(pBuf, BufLen, "\tUnknow Value(%u)", pAd->CommonCfg.RegTransmitSetting.field.HTMODE);
			break;
	}
	return 0;
}

INT	Show_HtExtcha_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	switch(pAd->CommonCfg.RegTransmitSetting.field.EXTCHA)
	{
		case EXTCHA_BELOW:
			snprintf(pBuf, BufLen, "\tBelow");
			break;
		case EXTCHA_ABOVE:
			snprintf(pBuf, BufLen, "\tAbove");
			break;
		default:
			snprintf(pBuf, BufLen, "\tUnknow Value(%u)", pAd->CommonCfg.RegTransmitSetting.field.EXTCHA);
			break;
	}
	return 0;
}


INT	Show_HtMpduDensity_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%u", pAd->CommonCfg.BACapability.field.MpduDensity);
	return 0;
}

INT	Show_HtBaWinSize_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%u", pAd->CommonCfg.BACapability.field.RxBAWinLimit);
	return 0;
}

INT	Show_HtRdg_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bRdg ? "true":"false");
	return 0;
}

INT	Show_HtAmsdu_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.BACapability.field.AmsduEnable ? "true":"false");
	return 0;
}

INT	Show_HtAutoBa_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.BACapability.field.AutoBA ? "true":"false");
	return 0;
}

INT	Show_CountryRegion_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%d", pAd->CommonCfg.CountryRegion);
	return 0;
}

INT	Show_CountryRegionABand_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%d", pAd->CommonCfg.CountryRegionForABand);
	return 0;
}

INT	Show_CountryCode_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.CountryCode);
	return 0;
}

#ifdef AGGREGATION_SUPPORT
INT	Show_PktAggregate_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bAggregationCapable ? "true":"false");
	return 0;
}
#endif /* AGGREGATION_SUPPORT */

INT	Show_WmmCapable_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
#ifdef CONFIG_AP_SUPPORT
    struct os_cookie *pObj = pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		snprintf(pBuf, BufLen, "\t%s", pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev.bWmmCapable ? "true":"false");
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bWmmCapable ? "true":"false");
#endif /* CONFIG_STA_SUPPORT */

	return 0;
}


INT	Show_IEEE80211H_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\t%s", pAd->CommonCfg.bIEEE80211H ? "true":"false");
	return 0;
}

#ifdef CONFIG_STA_SUPPORT
INT	Show_NetworkType_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	switch(pAd->StaCfg.BssType)
	{
		case BSS_ADHOC:
			snprintf(pBuf, BufLen, "\tAdhoc");
			break;
		case BSS_INFRA:
			snprintf(pBuf, BufLen, "\tInfra");
			break;
		case BSS_ANY:
			snprintf(pBuf, BufLen, "\tAny");
			break;
		case BSS_MONITOR:
			snprintf(pBuf, BufLen, "\tMonitor");
			break;
		default:
			sprintf(pBuf, "\tUnknow Value(%d)", pAd->StaCfg.BssType);
			break;
	}
	return 0;
}


INT	Show_WPAPSK_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	if ((pAd->StaCfg.WpaPassPhraseLen >= 8) &&
		(pAd->StaCfg.WpaPassPhraseLen < 64))
		snprintf(pBuf, BufLen, "\tWPAPSK = %s", pAd->StaCfg.WpaPassPhrase);
	else
	{
		INT idx;
		snprintf(pBuf, BufLen, "\tWPAPSK = ");
		for (idx = 0; idx < 32; idx++)
			snprintf(pBuf+strlen(pBuf), BufLen-strlen(pBuf), "%02X", pAd->StaCfg.WpaPassPhrase[idx]);
	}

	return 0;
}

INT	Show_AutoReconnect_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	snprintf(pBuf, BufLen, "\tAutoReconnect = %d", pAd->StaCfg.bAutoReconnect);
	return 0;
}

#endif /* CONFIG_STA_SUPPORT */

INT	Show_AuthMode_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	NDIS_802_11_AUTHENTICATION_MODE	AuthMode = Ndis802_11AuthModeOpen;
	struct rtmp_wifi_dev *wdev = NULL;
#ifdef CONFIG_AP_SUPPORT
    struct os_cookie *pObj = pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev  = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;

#endif /* CONFIG_STA_SUPPORT */

	if (!wdev)
		return -1;

	AuthMode = wdev->AuthMode;
	if ((AuthMode >= Ndis802_11AuthModeOpen) &&
		(AuthMode <= Ndis802_11AuthModeWPA1PSKWPA2PSK))
		snprintf(pBuf, BufLen, "\t%s", RTMPGetRalinkAuthModeStr(AuthMode));
	else
		snprintf(pBuf, BufLen, "\tUnknow Value(%d)", AuthMode);

	return 0;
}

INT	Show_EncrypType_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	struct rtmp_wifi_dev *wdev = NULL;
	NDIS_802_11_WEP_STATUS	WepStatus = Ndis802_11WEPDisabled;
#ifdef CONFIG_AP_SUPPORT
    struct os_cookie *pObj = pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;
#endif /* CONFIG_STA_SUPPORT */

	if (!wdev)
		return -1;

	WepStatus = wdev->WepStatus;
	if ((WepStatus >= Ndis802_11WEPEnabled) &&
		(WepStatus <= Ndis802_11Encryption4KeyAbsent))
		snprintf(pBuf, BufLen, "\t%s", RTMPGetRalinkEncryModeStr(WepStatus));
	else
		snprintf(pBuf, BufLen, "\tUnknow Value(%d)", WepStatus);

	return 0;
}

INT	Show_DefaultKeyID_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	struct rtmp_wifi_dev *wdev = NULL;
#ifdef CONFIG_AP_SUPPORT
    struct os_cookie *pObj = pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		wdev = &pAd->ApCfg.MBSSID[pObj->ioctl_if].wdev;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		wdev = &pAd->StaCfg.wdev;
#endif /* CONFIG_STA_SUPPORT */

	if (!wdev)
		return -1;

	snprintf(pBuf, BufLen, "\t%d", wdev->DefaultKeyId);

	return 0;
}


INT	Show_WepKey_Proc(
	IN	struct rtmp_adapter *pAd,
	IN  INT				KeyIdx,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	u8   Key[16] = {0}, KeyLength = 0;
	INT		index = BSS0;
#ifdef CONFIG_AP_SUPPORT
    struct os_cookie *pObj = pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		index = pObj->ioctl_if;
#endif /* CONFIG_AP_SUPPORT */

	KeyLength = pAd->SharedKey[index][KeyIdx].KeyLen;
	memmove(Key, pAd->SharedKey[index][KeyIdx].Key, KeyLength);

	/*check key string is ASCII or not*/
    if (RTMPCheckStrPrintAble((PCHAR)Key, KeyLength))
        sprintf(pBuf, "\t%s", Key);
    else
    {
        int idx;
        sprintf(pBuf, "\t");
        for (idx = 0; idx < KeyLength; idx++)
            sprintf(pBuf+strlen(pBuf), "%02X", Key[idx]);
    }
	return 0;
}

INT	Show_Key1_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	Show_WepKey_Proc(pAd, 0, pBuf, BufLen);
	return 0;
}

INT	Show_Key2_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	Show_WepKey_Proc(pAd, 1, pBuf, BufLen);
	return 0;
}

INT	Show_Key3_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	Show_WepKey_Proc(pAd, 2, pBuf, BufLen);
	return 0;
}

INT	Show_Key4_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	Show_WepKey_Proc(pAd, 3, pBuf, BufLen);
	return 0;
}

INT	Show_PMK_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	INT 	idx;
	u8 PMK[32] = {0};

#ifdef CONFIG_AP_SUPPORT
    struct os_cookie *pObj = pAd->OS_Cookie;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		memmove(PMK, pAd->ApCfg.MBSSID[pObj->ioctl_if].PMK, 32);
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		memmove(PMK, pAd->StaCfg.PMK, 32);
#endif /* CONFIG_STA_SUPPORT */

	sprintf(pBuf, "\tPMK = ");
	for (idx = 0; idx < 32; idx++)
		sprintf(pBuf+strlen(pBuf), "%02X", PMK[idx]);

	return 0;
}


INT	Show_STA_RAInfo_Proc(
	IN	struct rtmp_adapter *pAd,
	OUT	char *		pBuf,
	IN	ULONG			BufLen)
{
	sprintf(pBuf, "\n");
#ifdef PRE_ANT_SWITCH
	sprintf(pBuf+strlen(pBuf), "PreAntSwitch: %d\n", pAd->CommonCfg.PreAntSwitch);
	sprintf(pBuf+strlen(pBuf), "PreAntSwitchRSSI: %d\n", pAd->CommonCfg.PreAntSwitchRSSI);
#endif /* PRE_ANT_SWITCH */


#ifdef NEW_RATE_ADAPT_SUPPORT
	sprintf(pBuf+strlen(pBuf), "LowTrafficThrd: %d\n", pAd->CommonCfg.lowTrafficThrd);
	sprintf(pBuf+strlen(pBuf), "TrainUpRule: %d\n", pAd->CommonCfg.TrainUpRule);
	sprintf(pBuf+strlen(pBuf), "TrainUpRuleRSSI: %d\n", pAd->CommonCfg.TrainUpRuleRSSI);
	sprintf(pBuf+strlen(pBuf), "TrainUpLowThrd: %d\n", pAd->CommonCfg.TrainUpLowThrd);
	sprintf(pBuf+strlen(pBuf), "TrainUpHighThrd: %d\n", pAd->CommonCfg.TrainUpHighThrd);
#endif // NEW_RATE_ADAPT_SUPPORT //

	sprintf(pBuf+strlen(pBuf), "ITxBfEn: %d\n", pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn);
	sprintf(pBuf+strlen(pBuf), "ITxBfTimeout: %ld\n", pAd->CommonCfg.ITxBfTimeout);
	sprintf(pBuf+strlen(pBuf), "ETxBfTimeout: %ld\n", pAd->CommonCfg.ETxBfTimeout);
	sprintf(pBuf+strlen(pBuf), "ETxBfEnCond: %ld\n", pAd->CommonCfg.ETxBfEnCond);
	sprintf(pBuf+strlen(pBuf), "ETxBfNoncompress: %d\n", pAd->CommonCfg.ETxBfNoncompress);
	sprintf(pBuf+strlen(pBuf), "ETxBfIncapable: %d\n", pAd->CommonCfg.ETxBfIncapable);

#ifdef DBG_CTRL_SUPPORT
	sprintf(pBuf+strlen(pBuf), "DebugFlags: 0x%lx\n", pAd->CommonCfg.DebugFlags);
#endif /* DBG_CTRL_SUPPORT */
	return 0;
}

INT Show_MacTable_Proc(struct rtmp_adapter *pAd, char *arg)
{
	INT i;
    	uint32_t RegValue;
	uint32_t DataRate=0;


	printk("\n");
	RegValue = mt7612u_read32(pAd, BKOFF_SLOT_CFG);
	printk("BackOff Slot      : %s slot time, BKOFF_SLOT_CFG(0x1104) = 0x%08x\n",
			OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED) ? "short" : "long",
 			RegValue);

	printk("HT Operating Mode : %d\n", pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode);
	printk("\n");

	printk("\n%-19s%-4s%-4s%-4s%-4s%-8s",
		   "MAC", "AID", "BSS", "PSM", "WMM", "MIMOPS");

        if (pAd->CommonCfg.RxStream == 3)
                printk("%-7s%-7s%-7s","RSSI0", "RSSI1","RSSI2");
        else if (pAd->CommonCfg.RxStream == 2)
                printk("%-7s%-7s", "RSSI0", "RSSI1");
        else
                printk("%-7s", "RSSI0");

	printk("%-10s%-6s%-6s%-6s%-6s%-7s%-7s%-7s\n", "PhMd", "BW", "MCS", "SGI", "STBC", "Idle", "Rate", "TIME");

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if ((IS_ENTRY_CLIENT(pEntry) || (IS_ENTRY_APCLI(pEntry)
			))
			&& (pEntry->Sst == SST_ASSOC))
		{
			DataRate=0;
			//getRate(pEntry->HTPhyMode, &DataRate);
			RtmpDrvRateGet(pAd, pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.ShortGI,
		                          pEntry->HTPhyMode.field.BW, pEntry->HTPhyMode.field.MCS,
		                          newRateGetAntenna(pEntry->MaxHTPhyMode.field.MCS), &DataRate);
			DataRate /= 500000;
			DataRate /= 2;

			printk("%02X:%02X:%02X:%02X:%02X:%02X  ", PRINT_MAC(pEntry->Addr));
			printk("%-4d", (int)pEntry->Aid);
			printk("%-4d", (int)pEntry->apidx);
			printk("%-4d", (int)pEntry->PsMode);
			printk("%-4d", (int)CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE));
			printk("%-8d", (int)pEntry->MmpsMode);

		        if (pAd->CommonCfg.RxStream == 3)
                		printk("%-7d%-7d%-7d", pEntry->RssiSample.AvgRssi0, pEntry->RssiSample.AvgRssi1, pEntry->RssiSample.AvgRssi2);
		        else if (pAd->CommonCfg.RxStream == 2)
                		printk("%-7d%-7d", pEntry->RssiSample.AvgRssi0, pEntry->RssiSample.AvgRssi1);
        		else
                		printk("%-7d", pEntry->RssiSample.AvgRssi0);

			printk("%-10s", get_phymode_str(pEntry->HTPhyMode.field.MODE));
			printk("%-6s", get_bw_str(pEntry->HTPhyMode.field.BW));
			if (pEntry->HTPhyMode.field.MODE == MODE_VHT)
				printk("%dS-M%-2d", ((pEntry->HTPhyMode.field.MCS>>4) + 1), (pEntry->HTPhyMode.field.MCS & 0xf));
			else
			printk("%-6d", pEntry->HTPhyMode.field.MCS);
			printk("%-6d", pEntry->HTPhyMode.field.ShortGI);
			printk("%-6d", pEntry->HTPhyMode.field.STBC);
			printk("%-7d", (int)(pEntry->StaIdleTimeout - pEntry->NoDataIdleCount));
			printk("%-7d", (int)DataRate);
#ifdef CONFIG_AP_SUPPORT
			printk("%-7d", (int)pEntry->StaConnectTime);
#endif /* CONFIG_AP_SUPPORT */
			printk("\t\t\t\t\t\t\t%-10d, %d, %d%%\n", pEntry->DebugFIFOCount, pEntry->DebugTxCount,
						(pEntry->DebugTxCount) ? ((pEntry->DebugTxCount-pEntry->DebugFIFOCount)*100/pEntry->DebugTxCount) : 0);
//+++Add by shiang for debug
			printk(" MaxCap:%-10s", get_phymode_str(pEntry->MaxHTPhyMode.field.MODE));
			printk("%-6s", get_bw_str(pEntry->MaxHTPhyMode.field.BW));
			if (pEntry->MaxHTPhyMode.field.MODE == MODE_VHT)
				printk("%dS-M%d", ((pEntry->MaxHTPhyMode.field.MCS>>4) + 1), (pEntry->MaxHTPhyMode.field.MCS & 0xf));
			else
			printk("%-6d", pEntry->MaxHTPhyMode.field.MCS);
			printk("%-6d", pEntry->MaxHTPhyMode.field.ShortGI);
			printk("%-6d\n", pEntry->MaxHTPhyMode.field.STBC);
//---Add by shiang for debug
			printk("\n");
		}
	}

	return true;
}


INT show_stainfo_proc(struct rtmp_adapter *pAd, char *arg)
{
	INT i;
	uint32_t DataRate=0;
	u8 mac_addr[MAC_ADDR_LEN];
	char *token;
	CHAR sep[1] = {':'};
	MAC_TABLE_ENTRY *pEntry;


	DBGPRINT(RT_DEBUG_OFF, ("%s(): Input string=%s\n",
				__FUNCTION__, arg));
	for (i = 0, token = rstrtok(arg, &sep[0]); token; token = rstrtok(NULL, &sep[0]), i++)
	{
		DBGPRINT(RT_DEBUG_OFF, ("%s(): token(len=%d) =%s\n",
					__FUNCTION__, (int) strlen(token), token));
		if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
			return false;
		AtoH(token, (&mac_addr[i]), 1);
	}

	DBGPRINT(RT_DEBUG_OFF, ("%s(): i= %d\n",
				__FUNCTION__, i));
	if(i != 6)
		return false;

	DBGPRINT(RT_DEBUG_OFF, ("\nAddr %02x:%02x:%02x:%02x:%02x:%02x\n",
				PRINT_MAC(mac_addr)));

	pEntry = MacTableLookup(pAd, (u8 *)mac_addr);
    	if (!pEntry)
		return false;

	if (IS_ENTRY_NONE(pEntry)) {
		DBGPRINT(RT_DEBUG_OFF, ("Invalid MAC address!\n"));
		return false;
	}

	printk("\n");

	printk("EntryType : %d\n", pEntry->EntryType);
	printk("Entry Capability:\n");
	printk("\tPhyMode:%-10s\n", get_phymode_str(pEntry->MaxHTPhyMode.field.MODE));
	printk("\tBW:%-6s\n", get_bw_str(pEntry->MaxHTPhyMode.field.BW));
	printk("\tDataRate: \n");
	if (pEntry->MaxHTPhyMode.field.MODE == MODE_VHT)
		printk("%dS-M%d", ((pEntry->MaxHTPhyMode.field.MCS>>4) + 1), (pEntry->MaxHTPhyMode.field.MCS & 0xf));
	else
	printk(" %-6d", pEntry->MaxHTPhyMode.field.MCS);
	printk(" %-6d", pEntry->MaxHTPhyMode.field.ShortGI);
	printk(" %-6d\n", pEntry->MaxHTPhyMode.field.STBC);

	printk("Entry Operation Features\n");
	printk("\t%-4s%-4s%-4s%-4s%-8s%-7s%-7s%-7s%-10s%-6s%-6s%-6s%-6s%-7s%-7s\n",
		   "AID", "BSS", "PSM", "WMM", "MIMOPS", "RSSI0", "RSSI1",
		   "RSSI2", "PhMd", "BW", "MCS", "SGI", "STBC", "Idle", "Rate");

	DataRate=0;
	//getRate(pEntry->HTPhyMode, &DataRate);
    RtmpDrvRateGet(pAd, pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.ShortGI,
                      pEntry->HTPhyMode.field.BW, pEntry->HTPhyMode.field.MCS,
                      newRateGetAntenna(pEntry->MaxHTPhyMode.field.MCS), &DataRate);
	DataRate /= 500000;
	DataRate /= 2;

	printk("\t%-4d", (int)pEntry->Aid);
	printk("%-4d", (int)pEntry->apidx);
	printk("%-4d", (int)pEntry->PsMode);
	printk("%-4d", (int)CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE));
	printk("%-8d", (int)pEntry->MmpsMode);
	printk("%-7d", pEntry->RssiSample.AvgRssi0);
	printk("%-7d", pEntry->RssiSample.AvgRssi1);
	printk("%-7d", pEntry->RssiSample.AvgRssi2);
	printk("%-10s", get_phymode_str(pEntry->HTPhyMode.field.MODE));
	printk("%-6s", get_bw_str(pEntry->HTPhyMode.field.BW));
	if (pEntry->HTPhyMode.field.MODE == MODE_VHT)
		printk("%dS-M%d", ((pEntry->HTPhyMode.field.MCS>>4) + 1), (pEntry->HTPhyMode.field.MCS & 0xf));
	else
		printk("%-6d", pEntry->HTPhyMode.field.MCS);
	printk("%-6d", pEntry->HTPhyMode.field.ShortGI);
	printk("%-6d", pEntry->HTPhyMode.field.STBC);
	printk("%-7d", (int)(pEntry->StaIdleTimeout - pEntry->NoDataIdleCount));
	printk("%-7d", (int)DataRate);
	printk("%-10d, %d, %d%%\n", pEntry->DebugFIFOCount, pEntry->DebugTxCount,
				(pEntry->DebugTxCount) ? ((pEntry->DebugTxCount-pEntry->DebugFIFOCount)*100/pEntry->DebugTxCount) : 0);

	printk("\n");

	return true;
}

CHAR *wdev_type_str[]={"AP", "STA", "ADHOC", "WDS", "MESH", "Unknown"};

char *wdev_type2str(int type)
{
	switch (type)
	{
		case WDEV_TYPE_AP:
			return wdev_type_str[0];
		case WDEV_TYPE_STA:
			return wdev_type_str[1];
		case WDEV_TYPE_ADHOC:
			return wdev_type_str[2];
		case WDEV_TYPE_WDS	:
			return wdev_type_str[3];
		case WDEV_TYPE_MESH:
			return wdev_type_str[4];
		default:
			return wdev_type_str[5];
	}
}


INT show_trinfo_proc(struct rtmp_adapter *pAd, char *arg)
{

	DBGPRINT(RT_DEBUG_OFF, ("TxRing Configuration\n"));

	DBGPRINT(RT_DEBUG_OFF, ("\nRxRing Configuration\n"));

	DBGPRINT(RT_DEBUG_OFF, ("\nPBF Configuration\n"));

	return true;
}


void  getRate(HTTRANSMIT_SETTING HTSetting, uint32_t *fLastTxRxRate)

{
	VOID *pAd = NULL;
	RtmpDrvRateGet(pAd, HTSetting.field.MODE,HTSetting.field.ShortGI,
		HTSetting.field.BW,HTSetting.field.MCS,newRateGetAntenna(HTSetting.field.MCS),fLastTxRxRate);
	*(fLastTxRxRate) /= 500000;
	*(fLastTxRxRate) /= 2;
	return;
}

INT	Set_NoSndgCntThrd_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	u8 i;
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++){
		pAd->MacTab.Content[i].noSndgCntThrd = simple_strtol(arg, 0, 10);
	}
	return true;
}

INT	Set_NdpSndgStreams_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	u8 i;
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++){
		pAd->MacTab.Content[i].ndpSndgStreams = simple_strtol(arg, 0, 10);
	}
	return true;
}


INT	Set_Trigger_Sounding_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	u8 		macAddr[MAC_ADDR_LEN];
	CHAR			*value;
	INT				i;
	MAC_TABLE_ENTRY *pEntry = NULL;

	/* Mac address acceptable format 01:02:03:04:05:06 length 17 */
	if(strlen(arg) != 17)
		return false;

	for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":"))
	{
		if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) )
			return false;  /*Invalid*/

		AtoH(value, &macAddr[i++], 1);
	}

	/*DBGPRINT(RT_DEBUG_TRACE, ("TriggerSounding=%02x:%02x:%02x:%02x:%02x:%02x\n",*/
	/*		macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5], macAddr[6]) );*/
	pEntry = MacTableLookup(pAd, macAddr);
	if (pEntry==NULL)
		return false;

	Trigger_Sounding_Packet(pAd, SNDG_TYPE_SOUNDING, 0, pEntry->sndgMcs, pEntry);

	return true;
}

unsigned short  PFMU_TimeOut;
u8 MatrixForm[5];
u8 StsSnr[2];
u8 TxScale[4];
u8 macAddr[MAC_ADDR_LEN];
u8   FlgStatus[4];
unsigned short  CMDInIdx = 0, dCMDInIdx = 0;
u8   psi21;
u8   phill;



static SC_TABLE_ENTRY impSCTable[3] = { {224, 255, 1, 31}, {198, 254, 2, 58}, {134, 254, 2, 122} };
static SC_TABLE_ENTRY expSCTable[3] = { {224, 255, 1, 31}, {198, 254, 2, 58}, {134, 254, 2, 122} };

void assoc_ht_info_debugshow(
	IN struct rtmp_adapter *pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN u8 ht_cap_len,
	IN HT_CAPABILITY_IE *pHTCapability)
{
	HT_CAP_INFO			*pHTCap;
	HT_CAP_PARM		*pHTCapParm;
	EXT_HT_CAP_INFO		*pExtHT;
	HT_BF_CAP			*pBFCap;


	if (pHTCapability && (ht_cap_len > 0))
	{
		pHTCap = &pHTCapability->HtCapInfo;
		pHTCapParm = &pHTCapability->HtCapParm;
		pExtHT = &pHTCapability->ExtHtCapInfo;
		pBFCap = &pHTCapability->TxBFCap;

		DBGPRINT(RT_DEBUG_TRACE, ("Peer - 11n HT Info\n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\tHT Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t HT_RX_LDPC(%d), BW(%d), MIMOPS(%d), GF(%d), ShortGI_20(%d), ShortGI_40(%d)\n",
			pHTCap->ht_rx_ldpc, pHTCap->ChannelWidth, pHTCap->MimoPs, pHTCap->GF,
			pHTCap->ShortGIfor20, pHTCap->ShortGIfor40));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t TxSTBC(%d), RxSTBC(%d), DelayedBA(%d), A-MSDU(%d), CCK_40(%d)\n",
			pHTCap->TxSTBC, pHTCap->RxSTBC, pHTCap->DelayedBA, pHTCap->AMsduSize, pHTCap->CCKmodein40));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t PSMP(%d), Forty_Mhz_Intolerant(%d), L-SIG(%d)\n",
			pHTCap->PSMP, pHTCap->Forty_Mhz_Intolerant, pHTCap->LSIGTxopProSup));

		DBGPRINT(RT_DEBUG_TRACE, ("\tHT Parm Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t MaxRx A-MPDU Factor(%d), MPDU Density(%d)\n",
			pHTCapParm->MaxRAmpduFactor, pHTCapParm->MpduDensity));

		DBGPRINT(RT_DEBUG_TRACE, ("\tHT MCS set: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t RxMCS(%02x %02x %02x %02x %02x) MaxRxMbps(%d) TxMCSSetDef(%02x)\n",
			pHTCapability->MCSSet[0], pHTCapability->MCSSet[1], pHTCapability->MCSSet[2],
			pHTCapability->MCSSet[3], pHTCapability->MCSSet[4],
			(pHTCapability->MCSSet[11]<<8) + pHTCapability->MCSSet[10],
			pHTCapability->MCSSet[12]));

		DBGPRINT(RT_DEBUG_TRACE, ("\tExt HT Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t PCO(%d), TransTime(%d), MCSFeedback(%d), +HTC(%d), RDG(%d)\n",
			pExtHT->Pco, pExtHT->TranTime, pExtHT->MCSFeedback, pExtHT->PlusHTC, pExtHT->RDGSupport));

        	DBGPRINT(RT_DEBUG_TRACE, ("\tTX BF Cap: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t ImpRxCap(%d), RXStagSnd(%d), TXStagSnd(%d), RxNDP(%d), TxNDP(%d) ImpTxCap(%d)\n",
			pBFCap->TxBFRecCapable, pBFCap->RxSoundCapable, pBFCap->TxSoundCapable,
			pBFCap->RxNDPCapable, pBFCap->TxNDPCapable, pBFCap->ImpTxBFCapable));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t Calibration(%d), ExpCSICapable(%d), ExpComSteerCapable(%d), ExpCSIFbk(%d), ExpNoComBF(%d) ExpComBF(%d)\n",
			pBFCap->Calibration, pBFCap->ExpCSICapable, pBFCap->ExpComSteerCapable,
			pBFCap->ExpCSIFbk, pBFCap->ExpNoComBF, pBFCap->ExpComBF));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\t MinGrouping(%d), CSIBFAntSup(%d), NoComSteerBFAntSup(%d), ComSteerBFAntSup(%d), CSIRowBFSup(%d) ChanEstimation(%d)\n",
			pBFCap->MinGrouping, pBFCap->CSIBFAntSup, pBFCap->NoComSteerBFAntSup,
			pBFCap->ComSteerBFAntSup, pBFCap->CSIRowBFSup, pBFCap->ChanEstimation));

		DBGPRINT(RT_DEBUG_TRACE, ("\nPeer - MODE=%d, BW=%d, MCS=%d, ShortGI=%d, MaxRxFactor=%d, MpduDensity=%d, MIMOPS=%d, AMSDU=%d\n",
			pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW,
			pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.ShortGI,
			pEntry->MaxRAmpduFactor, pEntry->MpduDensity,
			pEntry->MmpsMode, pEntry->AMsduSize));

		DBGPRINT(RT_DEBUG_TRACE, ("\tExt Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tBss2040CoexistMgmt=%d\n", pEntry->BSS2040CoexistenceMgmtSupport));
	}
}


INT	Set_BurstMode_Proc(
	IN	struct rtmp_adapter *pAd,
	IN	char *		arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	pAd->CommonCfg.bRalinkBurstMode = ((Value == 1) ? true : false);

	AsicSetRalinkBurstMode(pAd, pAd->CommonCfg.bRalinkBurstMode);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_BurstMode_Proc ::%s\n",
				(pAd->CommonCfg.bRalinkBurstMode == true) ? "enabled" : "disabled"));

	return true;
}


VOID assoc_vht_info_debugshow(
	IN struct rtmp_adapter *pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN VHT_CAP_IE *vht_cap,
	IN VHT_OP_IE *vht_op)
{
	VHT_CAP_INFO *cap_info;
	VHT_MCS_SET *mcs_set;
	VHT_OP_INFO *op_info;
	VHT_MCS_MAP *mcs_map;


	if (!WMODE_CAP_AC(pAd->CommonCfg.PhyMode))
		return;

	DBGPRINT(RT_DEBUG_TRACE, ("Peer - 11AC VHT Info\n"));
	if (vht_cap)
	{
		cap_info = &vht_cap->vht_cap;
		mcs_set = &vht_cap->mcs_set;

		hex_dump("peer vht_cap raw data", (u8 *)cap_info, sizeof(VHT_CAP_INFO));
		hex_dump("peer vht_mcs raw data", (u8 *)mcs_set, sizeof(VHT_MCS_SET));

		DBGPRINT(RT_DEBUG_TRACE, ("\tVHT Cap Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tMaxMpduLen(%d), BW(%d), SGI_80M(%d), RxLDPC(%d), TxSTBC(%d), RxSTBC(%d), +HTC-VHT(%d)\n",
				cap_info->max_mpdu_len, cap_info->ch_width, cap_info->sgi_80M, cap_info->rx_ldpc, cap_info->tx_stbc,
				cap_info->rx_stbc, cap_info->htc_vht_cap));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tMaxAmpduExp(%d), VhtLinkAdapt(%d), RxAntConsist(%d), TxAntConsist(%d)\n",
				cap_info->max_ampdu_exp, cap_info->vht_link_adapt, cap_info->rx_ant_consistency, cap_info->tx_ant_consistency));
		mcs_map = &mcs_set->rx_mcs_map;
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tRxMcsSet: HighRate(%d), RxMCSMap(%d,%d,%d,%d,%d,%d,%d)\n",
			mcs_set->rx_high_rate, mcs_map->mcs_ss1, mcs_map->mcs_ss2, mcs_map->mcs_ss3,
			mcs_map->mcs_ss4, mcs_map->mcs_ss5, mcs_map->mcs_ss6, mcs_map->mcs_ss7));
		mcs_map = &mcs_set->tx_mcs_map;
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tTxMcsSet: HighRate(%d), TxMcsMap(%d,%d,%d,%d,%d,%d,%d)\n",
			mcs_set->tx_high_rate, mcs_map->mcs_ss1, mcs_map->mcs_ss2, mcs_map->mcs_ss3,
			mcs_map->mcs_ss4, mcs_map->mcs_ss5, mcs_map->mcs_ss6, mcs_map->mcs_ss7));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tETxBfCap: Bfer(%d), Bfee(%d), SndDim(%d)\n",
			cap_info->bfer_cap_su, cap_info->bfee_cap_su, cap_info->num_snd_dimension));
	}

	if (vht_op)
	{
		op_info = &vht_op->vht_op_info;
		mcs_map = &vht_op->basic_mcs_set;
		DBGPRINT(RT_DEBUG_TRACE, ("\tVHT OP Info: \n"));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tChannel Width(%d), CenteralFreq1(%d), CenteralFreq2(%d)\n",
			op_info->ch_width, op_info->center_freq_1, op_info->center_freq_2));
		DBGPRINT(RT_DEBUG_TRACE, ("\t\tBasicMCSSet(SS1:%d, SS2:%d, SS3:%d, SS4:%d, SS5:%d, SS6:%d, SS7:%d)\n",
			mcs_map->mcs_ss1, mcs_map->mcs_ss2, mcs_map->mcs_ss3,
			mcs_map->mcs_ss4, mcs_map->mcs_ss5, mcs_map->mcs_ss6,
			mcs_map->mcs_ss7));
	}

	DBGPRINT(RT_DEBUG_TRACE, ("\n"));

}


INT Set_RateAdaptInterval(
	IN struct rtmp_adapter *pAd,
	IN char *arg)
{
	uint32_t ra_time, ra_qtime;
	char *token;
	char sep = ':';
	ULONG irqFlags;

/*
	The ra_interval inupt string format should be d:d, in units of ms.
		=>The first decimal number indicates the rate adaptation checking period,
		=>The second decimal number indicates the rate adaptation quick response checking period.
*/
	DBGPRINT(RT_DEBUG_TRACE,("%s():%s\n", __FUNCTION__, arg));

	token = strchr(arg, sep);
	if (token != NULL)
	{
		*token = '\0';

		if (strlen(arg) && strlen(token+1))
		{
			ra_time = simple_strtol(arg, 0, 10);
			ra_qtime = simple_strtol(token+1, 0, 10);
			DBGPRINT(RT_DEBUG_OFF, ("%s():Set RateAdaptation TimeInterval as(%d:%d) ms\n",
						__FUNCTION__, ra_time, ra_qtime));

			RTMP_IRQ_LOCK(&pAd->irq_lock, irqFlags);
			pAd->ra_interval = ra_time;
			pAd->ra_fast_interval = ra_qtime;
#ifdef CONFIG_AP_SUPPORT
			if (pAd->ApCfg.ApQuickResponeForRateUpTimerRunning == true)
			{
				bool Cancelled;

				RTMPCancelTimer(&pAd->ApCfg.ApQuickResponeForRateUpTimer, &Cancelled);
				pAd->ApCfg.ApQuickResponeForRateUpTimerRunning = false;
			}
#endif /* CONFIG_AP_SUPPORT  */
			RTMP_IRQ_UNLOCK(&pAd->irq_lock, irqFlags);
			return true;
		}
	}

	return false;

}


INT Set_VcoPeriod_Proc(
	IN struct rtmp_adapter 	*pAd,
	IN char *		arg)
{
	pAd->chipCap.VcoPeriod = simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE,
			("VCO Period = %d seconds\n", pAd->chipCap.VcoPeriod));
	return true;
}

#ifdef WFA_VHT_PF
INT set_vht_nss_mcs_cap(struct rtmp_adapter *pAd, char *arg)
{
	CHAR *token, sep[2] = {':', '-'};
	u8 val[3] = {0}, ss, mcs_l, mcs_h, mcs_cap, status = false;
	INT idx = 0;

	DBGPRINT(RT_DEBUG_TRACE, ("%s():intput string=%s\n", __FUNCTION__, arg));
	ss = mcs_l = mcs_h = 0;

	while (arg)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s():intput string[len=%d]=%s\n", __FUNCTION__, strlen(arg), arg));
		if (idx < 2) {
			token = rtstrchr(arg, sep[idx]);
			if (!token) {
				DBGPRINT(RT_DEBUG_TRACE, ("cannot found token '%c' in string \"%s\"!\n", sep[idx], arg));
				return false;
			}
			*token++ = 0;
		} else
			token = NULL;

		if (strlen(arg)) {
			val[idx] = (u8)simple_strtoul(arg, NULL, 10);
			DBGPRINT(RT_DEBUG_TRACE, ("%s():token string[len=%d]=%s, val[%d]=%d\n",
						__FUNCTION__, strlen(arg), arg, idx, val[idx]));
			idx++;
		}
		arg = token;
		if (idx == 3)
			break;
	}

	if (idx <3)
		return false;

	ss = val[0];
	mcs_l = val[1];
	mcs_h = val[2];
	DBGPRINT(RT_DEBUG_TRACE, ("ss=%d, mcs_l=%d, mcs_h=%d\n", ss, mcs_l, mcs_h));
	if (ss && mcs_h)
	{
		if (ss <= pAd->chipCap.max_nss)
			pAd->CommonCfg.vht_nss_cap = ss;
		else
			pAd->CommonCfg.vht_nss_cap = pAd->chipCap.max_nss;

		switch (mcs_h)
		{
			case 7:
				mcs_cap = VHT_MCS_CAP_7;
				break;
			case 8:
				mcs_cap = VHT_MCS_CAP_8;
				break;
			case 9:
				mcs_cap = VHT_MCS_CAP_9;
				break;
			default:
				mcs_cap = VHT_MCS_CAP_9;
				break;
		}

		if (mcs_h <= pAd->chipCap.max_vht_mcs)
			pAd->CommonCfg.vht_mcs_cap = mcs_cap;
		else
			pAd->CommonCfg.vht_mcs_cap = pAd->chipCap.max_vht_mcs;

		DBGPRINT(RT_DEBUG_TRACE, ("%s():ss=%d, mcs_cap=%d, vht_nss_cap=%d, vht_mcs_cap=%d\n",
					__FUNCTION__, ss, mcs_cap,
					pAd->CommonCfg.vht_nss_cap,
					pAd->CommonCfg.vht_mcs_cap));
		status = true;
	}

	return status;
}


INT set_vht_nss_mcs_opt(struct rtmp_adapter *pAd, char *arg)
{


	DBGPRINT(RT_DEBUG_TRACE, ("%s():intput string=%s\n", __FUNCTION__, arg));

	return Set_HtMcs_Proc(pAd, arg);
}


INT set_vht_opmode_notify_ie(struct rtmp_adapter *pAd, char *arg)
{
	CHAR *token;
	UINT ss, bw;
	bool status = false;

	DBGPRINT(RT_DEBUG_TRACE, ("%s():intput string=%s\n", __FUNCTION__, arg));
	token = rtstrchr(arg, ':');
	if (!token)
		return false;

	*token = 0;
	token++;
	if (strlen(arg) && strlen(token))
	{
		ss = simple_strtoul(arg, NULL, 10);
		bw = simple_strtoul(token, NULL, 10);

		DBGPRINT(RT_DEBUG_TRACE, ("%s():ss=%d, bw=%d\n", __FUNCTION__, ss, bw));
		if (ss > 0 && ss <= 2)
			pAd->vht_pf_op_ss = ss;
		else
			pAd->vht_pf_op_ss = pAd->Antenna.field.RxPath;

		switch (bw) {
			case 20:
				pAd->vht_pf_op_bw = BAND_WIDTH_20;
				break;
			case 40:
				pAd->vht_pf_op_bw = BAND_WIDTH_40;
				break;
			case 80:
			default:
				pAd->vht_pf_op_bw = BAND_WIDTH_80;
				break;
		}
		status = true;
	}

	pAd->force_vht_op_mode = status;

	DBGPRINT(RT_DEBUG_TRACE, ("%s():force_vht_op_mode=%d, vht_pf_op_ss=%d, vht_pf_op_bw=%d\n",
				__FUNCTION__, pAd->force_vht_op_mode, pAd->vht_pf_op_ss, pAd->vht_pf_op_bw));

	return status;
}


INT set_force_operating_mode(struct rtmp_adapter *pAd, char *arg)
{
	pAd->force_vht_op_mode = (simple_strtol(arg, 0, 10) > 0 ? true : false);

	if (pAd->force_vht_op_mode == true) {
		pAd->vht_pf_op_ss = 1; // 1SS
		pAd->vht_pf_op_bw = BAND_WIDTH_20; // 20M
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s(): force_operating_mode=%d\n",
				__FUNCTION__, pAd->force_vht_op_mode));
	if (pAd->force_vht_op_mode == true)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("\tforce_operating_mode as %dSS in 20MHz BW\n",
					pAd->vht_pf_op_ss));
	}

	return true;
}


INT set_force_noack(struct rtmp_adapter *pAd, char *arg)
{
	pAd->force_noack = (simple_strtol(arg, 0, 10) > 0 ? true : false);
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): force_noack=%d\n",
				__FUNCTION__, pAd->force_noack));

	return true;
}


INT set_force_amsdu(struct rtmp_adapter *pAd, char *arg)
{
	pAd->force_amsdu = (simple_strtol(arg, 0, 10) > 0 ? true : false);
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): force_amsdu=%d\n",
				__FUNCTION__, pAd->force_amsdu));
	return true;
}


INT set_force_vht_sgi(struct rtmp_adapter *pAd, char *arg)
{
	pAd->vht_force_sgi = (simple_strtol(arg, 0, 10) > 0 ? true : false);
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): vht_force_sgi=%d\n",
				__FUNCTION__, pAd->vht_force_sgi));

	return true;
}


INT set_force_vht_tx_stbc(struct rtmp_adapter *pAd, char *arg)
{
	pAd->vht_force_tx_stbc = (simple_strtol(arg, 0, 10) > 0 ? true : false);
	if (pAd->CommonCfg.TxStream < 2)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s(): TxStream=%d is not enough for TxSTBC!\n",
				__FUNCTION__, pAd->CommonCfg.TxStream));
		pAd->vht_force_tx_stbc = 0;
	}
	DBGPRINT(RT_DEBUG_TRACE, ("%s(): vht_force_tx_stbc=%d\n",
				__FUNCTION__, pAd->vht_force_tx_stbc));

	return true;
}


INT set_force_ext_cca(struct rtmp_adapter *pAd, char *arg)
{
	ULONG cca_cfg;
	uint32_t mac_val;

	cca_cfg = (simple_strtol(arg, 0, 10) > 0 ? true : false);
	if (cca_cfg)
		mac_val = 0x04101b3f;
	else
		mac_val = 0x583f;
	mt7612u_write32(pAd, TXOP_CTRL_CFG, mac_val);

	return true;
}


INT set_rx_rts_cts(struct rtmp_adapter *pAd, char *arg)
{
	uint32_t do_rx, i;
	uint32_t mac_val, mask;
	char *cmds[4]={"no", "rts", "cts", "all"};

	for (i =0 ; i < 4; i++) {
		if (strcmp(arg, cmds[i]) ==0)
			break;
	}

	if (i >= 4)
		i = 0;

	mac_val = mt7612u_read32(pAd, RX_FILTR_CFG);
	switch (i)
	{
		case 1: // receive rts
			mac_val &= (~0x1000);
			break;
		case 2: // receive cts
			mac_val &= (~0x0800);
			break;
		case 3: // receive both
			mac_val &= (~0x1800);
			break;
		case 0: // filter out both rts/cts
		default:
			mac_val |= 0x1800;
			break;
	}
	mt7612u_write32(pAd, RX_FILTR_CFG, mac_val);

	mac_val = mt7612u_read32(pAd, RX_FILTR_CFG);
	DBGPRINT(RT_DEBUG_TRACE, ("%s():Configure the RTS/CTS filter as receive %s(0x%x)\n",
				__FUNCTION__, cmds[i], mac_val));

	return true;
}
#endif /* WFA_VHT_PF */

#define MAX_AGG_CNT	48

/* DisplayTxAgg - display Aggregation statistics from MAC */
void DisplayTxAgg (struct rtmp_adapter *pAd)
{
	ULONG totalCount;
	ULONG aggCnt[MAX_AGG_CNT + 2];
	int i;

	AsicReadAggCnt(pAd, aggCnt, sizeof(aggCnt) / sizeof(ULONG));
	totalCount = aggCnt[0] + aggCnt[1];
	if (totalCount > 0)
		for (i=0; i<MAX_AGG_CNT; i++) {
			DBGPRINT(RT_DEBUG_OFF, ("\t%d MPDU=%ld (%ld%%)\n", i+1, aggCnt[i+2], aggCnt[i+2]*100/totalCount));
		}
	printk("====================\n");

}

static struct {
	char *name;
	INT (*show_proc)(struct rtmp_adapter *pAd, char *arg, ULONG BufLen);
} *PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC, RTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC[] = {
#ifdef DBG
	{"SSID",					Show_SSID_Proc},
	{"WirelessMode",			Show_WirelessMode_Proc},
	{"TxBurst",					Show_TxBurst_Proc},
	{"TxPreamble",				Show_TxPreamble_Proc},
	{"TxPower",					Show_TxPower_Proc},
	{"Channel",					Show_Channel_Proc},
	{"BGProtection",			Show_BGProtection_Proc},
	{"RTSThreshold",			Show_RTSThreshold_Proc},
	{"FragThreshold",			Show_FragThreshold_Proc},
	{"HtBw",					Show_HtBw_Proc},
	{"HtMcs",					Show_HtMcs_Proc},
	{"HtGi",					Show_HtGi_Proc},
	{"HtOpMode",				Show_HtOpMode_Proc},
	{"HtExtcha",				Show_HtExtcha_Proc},
	{"HtMpduDensity",			Show_HtMpduDensity_Proc},
	{"HtBaWinSize",		        Show_HtBaWinSize_Proc},
	{"HtRdg",		        	Show_HtRdg_Proc},
	{"HtAmsdu",		        	Show_HtAmsdu_Proc},
	{"HtAutoBa",		        Show_HtAutoBa_Proc},
	{"CountryRegion",			Show_CountryRegion_Proc},
	{"CountryRegionABand",		Show_CountryRegionABand_Proc},
	{"CountryCode",				Show_CountryCode_Proc},
#ifdef AGGREGATION_SUPPORT
	{"PktAggregate",			Show_PktAggregate_Proc},
#endif

	{"WmmCapable",				Show_WmmCapable_Proc},

	{"IEEE80211H",				Show_IEEE80211H_Proc},
#ifdef CONFIG_STA_SUPPORT
    {"NetworkType",				Show_NetworkType_Proc},
	{"WPAPSK",					Show_WPAPSK_Proc},
	{"AutoReconnect", 			Show_AutoReconnect_Proc},
#endif /* CONFIG_STA_SUPPORT */
	{"AuthMode",				Show_AuthMode_Proc},
	{"EncrypType",				Show_EncrypType_Proc},
	{"DefaultKeyID",			Show_DefaultKeyID_Proc},
	{"Key1",					Show_Key1_Proc},
	{"Key2",					Show_Key2_Proc},
	{"Key3",					Show_Key3_Proc},
	{"Key4",					Show_Key4_Proc},
	{"PMK",						Show_PMK_Proc},
#endif /* DBG */
	{"rainfo",					Show_STA_RAInfo_Proc},
	{NULL, NULL}
};


INT RTMPShowCfgValue(
	IN	struct rtmp_adapter *pAd,
	IN	char *		pName,
	IN	char *		pBuf,
	IN	uint32_t 		MaxLen)
{
	INT	Status = 0;

	for (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC = RTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC; PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name; PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC++)
	{
		if (!strcmp(pName, PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name))
		{
			if(PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->show_proc(pAd, pBuf, MaxLen))
				Status = -EINVAL;
			break;  /*Exit for loop.*/
		}
	}

	if(PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name == NULL)
	{
		snprintf(pBuf, MaxLen, "\n");
		for (PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC = RTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC; PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name; PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC++)
		{
			if ((strlen(pBuf) + strlen(PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name)) >= MaxLen)
				break;
			sprintf(pBuf, "%s%s\n", pBuf, PRTMP_PRIVATE_STA_SHOW_CFG_VALUE_PROC->name);
		}
	}

	return Status;
}


#ifdef DBG_DIAGNOSE
INT Show_Diag_Proc(struct rtmp_adapter *pAd, char *arg)
{
	RtmpDiagStruct *pDiag = NULL;
	u8 i, start, stop, McsIdx, SwQNumLevel, TxDescNumLevel;
	unsigned long irqFlags;
	u8 McsMaxIdx = MAX_MCS_SET;
	u8 vht_mcs_max_idx = MAX_VHT_MCS_SET;

	pDiag = kmalloc(sizeof(RtmpDiagStruct), GFP_ATOMIC);
	if (!pDiag) {
		DBGPRINT(RT_DEBUG_ERROR, ("%s():AllocMem failed!\n", __FUNCTION__));
		return false;
	}

	memmove(pDiag, &pAd->DiagStruct, sizeof(RtmpDiagStruct));



	if (pDiag->inited == false)
		goto done;

	start = pDiag->ArrayStartIdx;
	stop = pDiag->ArrayCurIdx;
	printk("Start=%d, stop=%d!\n\n", start, stop);
	printk("    %-12s", "Time(Sec)");
	for(i=1; i< DIAGNOSE_TIME; i++)
	{
		printk("%-7d", i);
	}
	printk("\n    -------------------------------------------------------------------------------\n");
	printk("Tx Info:\n");
	printk("    %-12s", "TxDataCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->diag_info[i].TxDataCnt);
	}
	printk("\n    %-12s", "TxFailCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->diag_info[i].TxFailCnt);
	}

#ifdef DBG_TX_AGG_CNT
	printk("\n    %-12s", "TxAggCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->diag_info[i].TxAggCnt);
	}
	printk("\n");
#endif /* DBG_TX_AGG_CNT */

#ifdef DBG_TXQ_DEPTH
	printk("\n    %-12s\n", "Sw-Queued TxSwQCnt");
	for (SwQNumLevel = 0 ; SwQNumLevel < 9; SwQNumLevel++)
	{
		if (SwQNumLevel == 8)
			printk("\t>%-5d",  SwQNumLevel);
		else
			printk("\t%-6d", SwQNumLevel);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			printk("%-7d", pDiag->diag_info[i].TxSWQueCnt[SwQNumLevel]);

		printk("\n");
	}
#endif /* DBG_TXQ_DEPTH */

#ifdef DBG_TX_RING_DEPTH
	printk("\n    %-12s\n", "DMA-Queued TxDescCnt");
	for(TxDescNumLevel = 0; TxDescNumLevel < 16; TxDescNumLevel++)
	{
		if (TxDescNumLevel == 15)
			printk("\t>%-5d",  TxDescNumLevel);
		else
			printk("\t%-6d",  TxDescNumLevel);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			printk("%-7d", pDiag->diag_info[i].TxDescCnt[TxDescNumLevel]);

		printk("\n");
	}
#endif /* DBG_TX_RING_DEPTH */

#ifdef DBG_TX_AGG_CNT
	printk("\n    %-12s\n", "Tx-Agged AMPDUCnt");
	for (McsIdx =0 ; McsIdx < 16; McsIdx++)
	{
		printk("\t%-6d", (McsIdx+1));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			printk("%d(%d%%)  ", pDiag->diag_info[i].TxAMPDUCnt[McsIdx],
								pDiag->diag_info[i].TxAMPDUCnt[McsIdx] ? (pDiag->diag_info[i].TxAMPDUCnt[McsIdx] * 100 / pDiag->diag_info[i].TxAggCnt) : 0);
		}
		printk("\n");
	}
#endif /* DBG_TX_AGG_CNT */

#ifdef DBG_TX_MCS
	printk("\n    %-12s\n", "TxMcsCnt_HT");
	for (McsIdx =0 ; McsIdx < McsMaxIdx; McsIdx++)
	{
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			printk("%-7d", pDiag->diag_info[i].TxMcsCnt_HT[McsIdx]);
		printk("\n");
		}

	printk("\n    %-12s\n", "TxMcsCnt_VHT");
	for (McsIdx =0 ; McsIdx < vht_mcs_max_idx; McsIdx++)
		{
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
			printk("%-7d", pDiag->diag_info[i].TxMcsCnt_VHT[McsIdx]);
		printk("\n");
	}
#endif /* DBG_TX_MCS */



	printk("Rx Info\n");
	printk("    %-12s", "RxDataCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->diag_info[i].RxDataCnt);
	}
	printk("\n    %-12s", "RxCrcErrCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->diag_info[i].RxCrcErrCnt);
	}

#ifdef DBG_RX_MCS
	printk("\n    %-12s\n", "RxMcsCnt_HT");
	for (McsIdx =0 ; McsIdx < McsMaxIdx; McsIdx++)
	{
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			printk("(%d,%d)\t", pDiag->diag_info[i].RxCrcErrCnt_HT[McsIdx], pDiag->diag_info[i].RxMcsCnt_HT[McsIdx] + pDiag->diag_info[i].RxCrcErrCnt_HT[McsIdx]);
		}
		printk("\n");
	}

	printk("\n    %-12s\n", "RxMcsCnt_VHT");
	for (McsIdx =0 ; McsIdx < vht_mcs_max_idx; McsIdx++)
	{
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			printk("(%d,%d)\t", pDiag->diag_info[i].RxCrcErrCnt_VHT[McsIdx], pDiag->diag_info[i].RxMcsCnt_VHT[McsIdx] + pDiag->diag_info[i].RxCrcErrCnt_VHT[McsIdx]);
		}
		printk("\n");
	}

#endif /* DBG_RX_MCS */

	printk("\n-------------\n");

done:
	kfree(pDiag);
	return true;
}
#endif /* DBG_DIAGNOSE */

