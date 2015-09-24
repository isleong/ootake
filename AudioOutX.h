/******************************************************************************
Ootake

	[AudioOutX.h]
		XAudio2によるオーディオインタフェイスを定義します．

Copyright(C)2006-2013 Kitao Nakamura.
	改造版・後継版を公開なさるときは必ずソースコードを添付してください。
	その際に事後でかまいませんので、ひとことお知らせいただけると幸いです。
	商的な利用は禁じます。
	あとは「GNU General Public License(一般公衆利用許諾契約書)」に準じます。

******************************************************************************/
#ifndef AUDIO_OUTX_H_INCLUDED
#define AUDIO_OUTX_H_INCLUDED

#include "TypeDefs.h"

#define AOUTX_BUFFERRATE 2	//２つぶんのバッファを用意。XAudio2に任せる部分が多いので4つでなく2つで音質もOK。v2.70


BOOL
AOUTX_Init(
	Sint32		soundType,
	Uint32		bufSize,
	Uint32		sampleRate,
	void		(*pCallBack)(int ch, Sint16* pBuf, Sint32 nSamples));

void
AOUTX_Play(
	BOOL		bPlay);

void
AOUTX_Deinit();

void
AOUTX_SetPlayStart();

BOOL
AOUTX_GetPlay();

//Kitao追加
BOOL
AOUTX_GetThreadStarted();


#endif // AUDIO_OUTX_H_INCLUDED
