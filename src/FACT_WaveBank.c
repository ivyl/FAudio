/* FACT - XACT Reimplementation for FNA
 * Copyright 2009-2017 Ethan Lee and the MonoGame Team
 *
 * Released under the Microsoft Public License.
 * See LICENSE for details.
 */

#include "FACT_internal.h"

uint32_t FACTWaveBank_Destroy(FACTWaveBank *pWaveBank)
{
	FACTWaveBank *wb, *prev;

	/* Remove this WaveBank from the Engine list */
	wb = pWaveBank->parentEngine->wbList;
	prev = wb;
	while (wb != NULL)
	{
		if (wb == pWaveBank)
		{
			if (wb == prev) /* First in list */
			{
				pWaveBank->parentEngine->wbList = wb->next;
			}
			else
			{
				prev->next = wb->next;
			}
			break;
		}
		prev = wb;
		wb = wb->next;
	}
	FACT_assert(wb != NULL && "Could not find WaveBank reference!");

	FACT_free(pWaveBank->name);
	FACT_free(pWaveBank->entries);
	FACT_free(pWaveBank->entryRefs);
	FACT_close(pWaveBank->io);
	FACT_free(pWaveBank);
	return 0;
}

uint32_t FACTWaveBank_GetState(
	FACTWaveBank *pWaveBank,
	uint32_t *pdwState
) {
	/* FIXME: Is there more to this than just checking INUSE? */
	uint32_t i;
	for (i = 0; i < pWaveBank->entryCount; i += 1)
	{
		if (pWaveBank->entryRefs[i] > 0)
		{
			return FACT_STATE_INUSE;
		}
	}
	return 0;
}

uint32_t FACTWaveBank_GetNumWaves(
	FACTWaveBank *pWaveBank,
	uint16_t *pnNumWaves
) {
	*pnNumWaves = pWaveBank->entryCount;
	return 0;
}

uint16_t FACTWaveBank_GetWaveIndex(
	FACTWaveBank *pWaveBank,
	const char *szFriendlyName
) {
	FACT_assert(0 && "WaveBank name tables are not supported!");
	return 0;
}

uint32_t FACTWaveBank_GetWaveProperties(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	FACTWaveProperties *pWaveProperties
) {
	FACTWaveBankEntry *entry = &pWaveBank->entries[nWaveIndex];

	/* FIXME: Name tables! -flibit */
	FACT_zero(
		pWaveProperties->friendlyName,
		sizeof(pWaveProperties->friendlyName)
	);

	pWaveProperties->format = entry->Format;
	pWaveProperties->durationInSamples = (
		entry->LoopRegion.dwStartSample +
		entry->LoopRegion.dwTotalSamples
	); /* FIXME: Do we just want the full wave block? -flibit */
	pWaveProperties->loopRegion = entry->LoopRegion;
	pWaveProperties->streaming = pWaveBank->streaming;
	return 0;
}

uint32_t FACTWaveBank_Prepare(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	uint32_t dwFlags,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	FACTWave *latest;
	FACTWaveBankMiniWaveFormat *fmt = &pWaveBank->entries[nWaveIndex].Format;

	*ppWave = (FACTWave*) FACT_malloc(sizeof(FACTWave));
	(*ppWave)->next = NULL;

	/* Engine references */
	(*ppWave)->parentBank = pWaveBank;
	(*ppWave)->index = nWaveIndex;

	/* Playback */
	(*ppWave)->state = FACT_STATE_PREPARED;
	(*ppWave)->volume = 1.0f;
	(*ppWave)->pitch = 0;
	if (dwFlags & FACT_FLAG_UNITS_MS)
	{
		(*ppWave)->initialPosition = (uint32_t) (
			( /* Samples per millisecond... */
				(float) fmt->nSamplesPerSec /
				1000.0f
			) * (float) dwPlayOffset
		);
	}
	else
	{
		(*ppWave)->initialPosition = dwPlayOffset;
	}
	(*ppWave)->position = (*ppWave)->initialPosition;
	(*ppWave)->loopCount = nLoopCount;

	/* Decoding */
	if (fmt->wFormatTag == 0x0) /* PCM */
	{
		if (fmt->wBitsPerSample == 1)
		{
			(*ppWave)->decode = (fmt->nChannels == 2) ?
				FACT_INTERNAL_DecodeStereoPCM16 :
				FACT_INTERNAL_DecodeMonoPCM16;
		}
		else
		{
			(*ppWave)->decode = (fmt->nChannels == 2) ?
				FACT_INTERNAL_DecodeStereoPCM8 :
				FACT_INTERNAL_DecodeMonoPCM8;
		}
	}
	else if (fmt->wFormatTag == 0x2) /* ADPCM */
	{
		if (fmt->wBlockAlign == 0)
		{
			(*ppWave)->decode = (fmt->nChannels == 2) ?
				FACT_INTERNAL_DecodeStereoMSADPCM32 :
				FACT_INTERNAL_DecodeMonoMSADPCM32;
		}
		else if (fmt->wBlockAlign == 16)
		{
			(*ppWave)->decode = (fmt->nChannels == 2) ?
				FACT_INTERNAL_DecodeStereoMSADPCM64 :
				FACT_INTERNAL_DecodeMonoMSADPCM64;
		}
		else if (fmt->wBlockAlign == 48)
		{
			(*ppWave)->decode = (fmt->nChannels == 2) ?
				FACT_INTERNAL_DecodeStereoMSADPCM128 :
				FACT_INTERNAL_DecodeMonoMSADPCM128;
		}
		else if (fmt->wBlockAlign == 112)
		{
			(*ppWave)->decode = (fmt->nChannels == 2) ?
				FACT_INTERNAL_DecodeStereoMSADPCM256 :
				FACT_INTERNAL_DecodeMonoMSADPCM256;
		}
		else if (fmt->wBlockAlign == 240)
		{
			(*ppWave)->decode = (fmt->nChannels == 2) ?
				FACT_INTERNAL_DecodeStereoMSADPCM512 :
				FACT_INTERNAL_DecodeMonoMSADPCM512;
		}
		else
		{
			FACT_assert(0 && "Unrecognized wBlockAlign!");
		}
	}
	else /* Includes 0x1 - XMA, 0x3 - WMA */
	{
		FACT_assert(0 && "Rebuild your WaveBanks with ADPCM!");
	}
	(*ppWave)->msadpcmExtra = 0;
	FACT_PlatformInitConverter(*ppWave);

	/* Add to the WaveBank Wave list */
	if (pWaveBank->waveList == NULL)
	{
		pWaveBank->waveList = *ppWave;
	}
	else
	{
		latest = pWaveBank->waveList;
		while (latest->next != NULL)
		{
			latest = latest->next;
		}
		latest->next = *ppWave;
	}

	return 0;
}

uint32_t FACTWaveBank_Play(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	uint32_t dwFlags,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
) {
	FACTWaveBank_Prepare(
		pWaveBank,
		nWaveIndex,
		dwFlags,
		dwPlayOffset,
		nLoopCount,
		ppWave
	);
	FACTWave_Play(*ppWave);
	return 0;
}

uint32_t FACTWaveBank_Stop(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	uint32_t dwFlags
) {
	FACTWave *wave = pWaveBank->waveList;
	while (wave != NULL)
	{
		if (wave->index == nWaveIndex)
		{
			FACTWave_Stop(wave, dwFlags);
		}
		wave = wave->next;
	}
	return 0;
}
