/******************************************************************************
Ootake

	[AudioOut9.h]
		DirectSound9によるオーディオインタフェイスを定義します．

Copyright(C)2006-2013 Kitao Nakamura.
	改造版・後継版を公開なさるときは必ずソースコードを添付してください。
	その際に事後でかまいませんので、ひとことお知らせいただけると幸いです。
	商的な利用は禁じます。
	あとは「GNU General Public License(一般公衆利用許諾契約書)」に準じます。

******************************************************************************/
#ifndef AUDIO_OUT9_H_INCLUDED
#define AUDIO_OUT9_H_INCLUDED

#include "TypeDefs.h"

#define AOUT9_BUFFERRATE 4	//バッファを4つに分けて持つ。数が少ないほうが音の遅延は少なく済むが、数が多いほうが音が安定し音質も上がる。v2.70更新


BOOL
AOUT9_Init(
	Sint32		soundType,
	Uint32		bufSize,
	Uint32		sampleRate,
	void		(*pCallBack)(int ch, Sint16* pBuf, Sint32 nSamples));

void
AOUT9_Play(
	BOOL		bPlay);

void
AOUT9_Deinit();

//Kitao追加
void
AOUT9_SetPlayStart();

//Kitao追加
BOOL
AOUT9_GetPlay();

//Kitao追加
BOOL
AOUT9_GetThreadStarted();


#endif // AUDIO_OUT9_H_INCLUDED
