/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2013, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:

	Abstract:

	Revision History:
	Who 		When			What
	--------	----------		----------------------------------------------
*/

#ifdef RT_CFG80211_SUPPORT

#include "rt_config.h"

VOID CFG80211_Convert802_3Packet(struct rtmp_adapter *pAd, RX_BLK *pRxBlk, u8 *pHeader802_3)
{
#ifdef CONFIG_AP_SUPPORT
	if (IS_PKT_OPMODE_AP(pRxBlk))
	{
    	RTMP_AP_802_11_REMOVE_LLC_AND_CONVERT_TO_802_3(pRxBlk, pHeader802_3);
	}
	else
#endif /* CONFIG_AP_SUPPORT */
	{
#ifdef CONFIG_STA_SUPPORT
		RTMP_802_11_REMOVE_LLC_AND_CONVERT_TO_802_3(pRxBlk, pHeader802_3);
#endif /*CONFIG_STA_SUPPORT*/
	}
}

VOID CFG80211_Announce802_3Packet(struct rtmp_adapter *pAd, RX_BLK *pRxBlk, u8 FromWhichBSSID)
{
#ifdef CONFIG_AP_SUPPORT
	if (IS_PKT_OPMODE_AP(pRxBlk))
	{
		AP_ANNOUNCE_OR_FORWARD_802_3_PACKET(pAd, pRxBlk->pRxPacket, FromWhichBSSID);
	}
	else
#endif /* CONFIG_AP_SUPPORT */
	{
#ifdef CONFIG_STA_SUPPORT
		ANNOUNCE_OR_FORWARD_802_3_PACKET(pAd, pRxBlk->pRxPacket, FromWhichBSSID);
#endif /*CONFIG_STA_SUPPORT*/
	}

}

#endif /* RT_CFG80211_SUPPORT */

