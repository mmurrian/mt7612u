/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2009, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	cmm_txbf.c

	Abstract:
	Tx Beamforming related functions

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
	Shiang     2009/11/04
*/

#include	"rt_config.h"


#define ETXBF_PROBE_TIME (RA_INTERVAL-100)	/* Wait for Sounding Response will time out 100msec before end of RA interval */





VOID rtmp_asic_set_bf(
	IN struct rtmp_adapter *pAd)
{
	UINT Value32;



	Value32 = mt76u_reg_read(pAd, PFMU_R1);
	Value32 &= ~0x330;

	if (pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn)
		Value32 |= 0x120;
	else
		Value32 &= (~0x120);

	if (pAd->CommonCfg.ETxBfEnCond > 0)
		Value32 |= 0x210;
	else
		Value32 &= ~0x210;

	mt76u_reg_write(pAd, PFMU_R1, Value32);

	Value32 = mt76u_reg_read(pAd, PFMU_R0);
	Value32 &= ~((0x1 << 6) | 0x3);

	if (pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn)
		Value32 |= ((0x1 << 6) | 0x1);
	else
		Value32 &= ~((0x1 << 6) | 0x1);

	if (pAd->CommonCfg.ETxBfEnCond > 0)
	{
		Value32 |= (0x1 << 6) | 0x2;
		mt76u_reg_write(pAd, TX_TXBF_CFG_2, 0xFFFFFFFF);
	}
	else
		Value32 &= ~((0x1 << 6) | 0x2);

	mt76u_reg_write(pAd, PFMU_R0, Value32);
}

/*
	TxBFInit - Intialize TxBF fields in pEntry
		supportsETxBF - true if client supports ETxBF
*/
VOID TxBFInit(
	IN struct rtmp_adapter *	pAd,
	IN MAC_TABLE_ENTRY	*pEntry,
	IN bool			supportsETxBF)
{
	pEntry->bfState = READY_FOR_SNDG0;
	pEntry->sndgMcs = 0;
	pEntry->sndg0Snr0 = 0;
	pEntry->sndg0Snr1 = 0;
	pEntry->sndg0Snr2 = 0;
	pEntry->sndg0Mcs = 0;
	pEntry->noSndgCnt = 0;
	pEntry->eTxBfEnCond = supportsETxBF? pAd->CommonCfg.ETxBfEnCond: 0;
	pEntry->noSndgCntThrd = NO_SNDG_CNT_THRD;
	pEntry->ndpSndgStreams = pAd->Antenna.field.TxPath;

	/* If client supports ETxBf and ITxBF then give ETxBF priority over ITxBF */
	pEntry->iTxBfEn = pEntry->eTxBfEnCond> 0 ? 0 : pAd->CommonCfg.RegTransmitSetting.field.ITxBfEn;

}

bool rtmp_chk_itxbf_calibration(
	IN struct rtmp_adapter *pAd)
{
	INT calIdx, calCnt;
	unsigned short offset, eeVal, *calptr;
	unsigned short g_caladdr[] = {0xc0, 0xc2, 0xd4, 0xd6, 0xd8};
	unsigned short a_caladdr[] = {0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0};
	uint32_t ee_sum;
	bool bCalibrated = true;


	if (pAd->CommonCfg.Channel <= 14)
	{
		calCnt = sizeof(g_caladdr) / sizeof(unsigned short);
		calptr = &g_caladdr[0] ;
	}
	else
	{
		calCnt = sizeof(a_caladdr) / sizeof(unsigned short);
		calptr = &a_caladdr[0];
	}

	ee_sum = 0;
	for (calIdx = 0; calIdx < calCnt; calIdx++)
	{
		offset = *(calptr + calIdx);
		eeVal = mt76u_read_eeprom(pAd, offset);
		ee_sum += eeVal;
		DBGPRINT(RT_DEBUG_INFO, ("Check EEPROM(offset=0x%x, eeVal=0x%x, ee_sum=0x%x)!\n",
					offset, eeVal, ee_sum));
		if (eeVal!=0xffff && eeVal!=0)
			return true;
	}

	if ((ee_sum == (0xffff * calCnt)) || (ee_sum == 0x0))
	{
		bCalibrated = false;
		DBGPRINT(RT_DEBUG_TRACE, ("EEPROM all 0xffff(cnt =%d, sum=0x%x), not valid calibration value!\n",
					calCnt, ee_sum));
	}

	return bCalibrated;
}


VOID Trigger_Sounding_Packet(
	IN	struct rtmp_adapter *pAd,
	IN	u8 		SndgType,
	IN	u8 		SndgBW,
	IN	u8 		SndgMcs,
	IN  MAC_TABLE_ENTRY *pEntry)
{
	/*
		SngType
			0: disable
			1 : sounding
			2: NDP sounding
	*/
	spin_lock_bh(&pEntry->TxSndgLock);
	pEntry->TxSndgType = SndgType;
	spin_unlock_bh(&pEntry->TxSndgLock);

	RTMPSetTimer(&pEntry->eTxBfProbeTimer, ETXBF_PROBE_TIME);
	DBGPRINT(RT_DEBUG_TRACE, ("ETxBF in Trigger_Sounding_Packet(): sndgType=%d, bw=%d, mcs=%d\n", SndgType, SndgBW, SndgMcs));
}


/*
	eTxBFProbing - called by Rate Adaptation routine each interval.
		Initiates a sounding packet if enabled.
*/
VOID eTxBFProbing(
 	IN struct rtmp_adapter *pAd,
	IN MAC_TABLE_ENTRY	*pEntry)
{
	if (pEntry->eTxBfEnCond == 0)
	{
		pEntry->bfState = READY_FOR_SNDG0;
	}
	else if (pEntry->bfState==READY_FOR_SNDG0 && pEntry->noSndgCnt>=pEntry->noSndgCntThrd)
	{
		/* Select NDP sounding, maximum streams */
		pEntry->sndgMcs = (pEntry->ndpSndgStreams==3) ? 16 : 8;
		Trigger_Sounding_Packet(pAd, SNDG_TYPE_NDP, 0, pEntry->sndgMcs, pEntry);

		pEntry->bfState = WAIT_SNDG_FB0;
		pEntry->noSndgCnt = 0;
	}
	else if (pEntry->bfState == READY_FOR_SNDG0)
	{
		pEntry->noSndgCnt++;
	}
	else
		pEntry->noSndgCnt = 0;
}


/*
	clientSupportsETxBF - returns true if client supports compatible Sounding
*/
bool clientSupportsETxBF(
	IN	struct rtmp_adapter * pAd,
	IN	HT_BF_CAP *pTxBFCap)
{
	bool compCompat, noncompCompat;

	compCompat = (pTxBFCap->ExpComBF > 0) &&
			 /*(pTxBFCap->ComSteerBFAntSup+1 >= pAd->Antenna.field.TxPath) && */
			 (pAd->CommonCfg.ETxBfNoncompress == 0);

	noncompCompat = (pTxBFCap->ExpNoComBF > 0)
			/* && (pTxBFCap->NoComSteerBFAntSup+1 >= pAd->Antenna.field.TxPath)*/;

	return pTxBFCap->RxNDPCapable==1 && (compCompat || noncompCompat);
}


/*
	clientSupportsETxBF - returns true if client supports compatible Sounding
*/
bool clientSupportsVHTETxBF(
	IN	struct rtmp_adapter * pAd,
	IN	VHT_CAP_INFO    *pTxBFCap)
{
	return pTxBFCap->bfee_cap_su;
}


/*
	setETxBFCap - sets our ETxBF capabilities
*/
void setETxBFCap(struct rtmp_adapter *pAd, HT_BF_CAP *pTxBFCap)
{
	if (pAd->CommonCfg.ETxBfIncapable) {
		memset(pTxBFCap, 0, sizeof(*pTxBFCap));
	}
	else
	{
		pTxBFCap->RxNDPCapable =  true;
		pTxBFCap->TxNDPCapable =  true;
		pTxBFCap->ExpNoComSteerCapable =  true;
		pTxBFCap->ExpComSteerCapable = !pAd->CommonCfg.ETxBfNoncompress;
		pTxBFCap->ExpNoComBF = HT_ExBF_FB_CAP_IMMEDIATE;
		pTxBFCap->ExpComBF = pAd->CommonCfg.ETxBfNoncompress? HT_ExBF_FB_CAP_NONE: HT_ExBF_FB_CAP_IMMEDIATE;
		pTxBFCap->MinGrouping = 3;
		pTxBFCap->NoComSteerBFAntSup = 1; // 2 Tx antenna sounding
		pTxBFCap->ComSteerBFAntSup = 1;   // 2 Tx antenna sounding

		pTxBFCap->TxSoundCapable = true;  // Support staggered sounding frames
		pTxBFCap->ChanEstimation = pAd->Antenna.field.RxPath-1;
	}
}

void setVHTETxBFCap(struct rtmp_adapter *pAd, VHT_CAP_INFO *pTxBFCap)
{
	if (pAd->CommonCfg.ETxBfIncapable) {
		pTxBFCap->num_snd_dimension = 0;
		pTxBFCap->bfee_cap_mu = false;
		pTxBFCap->bfee_cap_su = false;
		pTxBFCap->bfer_cap_mu = false;
		pTxBFCap->bfer_cap_su = false;
		pTxBFCap->cmp_st_num_bfer = 0;
	}
	else
	{
		pTxBFCap->bfee_cap_su = true;
		pTxBFCap->bfer_cap_su = true;
		pTxBFCap->num_snd_dimension = 1;
		pTxBFCap->cmp_st_num_bfer = 1;
	}
}

VOID handleBfFb(
	IN	struct rtmp_adapter *pAd,
	IN	RX_BLK			*pRxBlk)
{
	MAC_TABLE_ENTRY *pEntry = NULL;

	if (pRxBlk->wcid >= MAX_LEN_OF_MAC_TABLE)
	{
		return;
	}
	pEntry = &(pAd->MacTab.Content[pRxBlk->wcid]);

	/*
		DBGPRINT(RT_DEBUG_TRACE, ("ETxBF :(%02x:%02x:%02x:%02x:%02x:%02x)\n",
							pEntry->Addr[0],pEntry->Addr[1],pEntry->Addr[2],
							pEntry->Addr[3],pEntry->Addr[4], pEntry->Addr[5]));
	*/

	if (pEntry->bfState == WAIT_SNDG_FB0)
	{
		int Nc = ((pRxBlk ->pData)[2] & 0x3) + 1;
		/*record the snr comb*/
		pEntry->sndg0Snr0 = 88+(CHAR)(pRxBlk ->pData[8]);
		pEntry->sndg0Snr1 = (Nc<2)? 0: 88+(CHAR)(pRxBlk ->pData[9]);
		pEntry->sndg0Snr2 = (Nc<3)? 0: 88+(CHAR)(pRxBlk ->pData[10]);
		pEntry->sndg0Mcs = pEntry->sndgMcs;

		DBGPRINT(RT_DEBUG_INFO,("   ETxBF: aid=%d  snr %d.%02d %d.%02d %d.%02d\n",
					pRxBlk->wcid,
					pEntry->sndg0Snr0/4,  25*(pEntry->sndg0Snr0 & 0x3),
					pEntry->sndg0Snr1/4,  25*(pEntry->sndg0Snr1 & 0x3),
					pEntry->sndg0Snr2/4,  25*(pEntry->sndg0Snr2 & 0x3)) );
		pEntry->bfState = READY_FOR_SNDG0;
	}
}

VOID handleHtcField(struct rtmp_adapter *pAd, RX_BLK *pRxBlk)
{
}

void eTxBfProbeTimerExec(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3)
{
	MAC_TABLE_ENTRY     *pEntry = (PMAC_TABLE_ENTRY) FunctionContext;

	if (pEntry->bfState == WAIT_SNDG_FB0)
	{
		/*record the snr comb*/
		pEntry->sndg0Snr0 = -128;
		pEntry->sndg0Snr1 = -128;
		pEntry->sndg0Snr2 = -128;
		pEntry->sndg0Mcs = pEntry->sndgMcs;
		//DBGPRINT(RT_DEBUG_TRACE,("   ETxBF: timer of WAIT_SNDG_FB0 expires\n" ));
		pEntry->bfState =READY_FOR_SNDG0;
	}
}

/* MlmeTxBfAllowed - returns true if ETxBF or ITxBF is supported and pTxRate is a valid BF mode */
bool MlmeTxBfAllowed(
	IN struct rtmp_adapter *pAd,
	IN MAC_TABLE_ENTRY *pEntry,
	IN RTMP_RA_LEGACY_TB *pTxRate)
{
	/* ETxBF */
	if ((pEntry->eTxBfEnCond > 0) &&
		(pTxRate->Mode == MODE_HTMIX || pTxRate->Mode == MODE_HTGREENFIELD)
#ifdef DBG_CTRL_SUPPORT
		&& (!((pAd->CommonCfg.DebugFlags & DBF_NO_TXBF_3SS) && pTxRate->CurrMCS>20))
#endif /* DBG_CTRL_SUPPORT */
	)
		return true;

	/* ITxBF */
	if (pEntry->iTxBfEn && pTxRate->CurrMCS<16 && pTxRate->Mode!=MODE_CCK)
		return true;

	return false;
}

