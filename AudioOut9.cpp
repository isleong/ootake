/******************************************************************************
Ootake
・PSG,ADPCM,CDDAそれぞれにDirectSoundバッファを用意することで、各サウンドのダイ
  ナミックレンジを最大限に取り、音質を向上させた。
・バッファのブロックを４つに分けることで、音の遅延とパソコンへの負荷を軽減させ
  た。
・初回の再生時にはウェイトを入れることで、初回再生に起こるノイズを解消した。
・現状は再生サンプルレートは44.1KHz固定とした。(CD-DA再生時の速度アップのため)
・バッファ書き込み時のメモリロックは起動時と終了時の１回のみ行うようにして高速
  化した。v1.02
・CoInitializeはDirectSoundCreate()を使った場合は不要のようなのでカット。これ
  がS_FALSEを返すことで強制終了していた環境があれば、動くようになるかもしれな
  い。v1.04。COINIT_MULTITHREADEDをメインスレッドで行っていることを考慮して、
  Audioスレッドでも念のためCoInitializeExを行うようにした。v2.19
・クリティカルセクションは必要ない(書き込みが同時に行われるわけではない)ような
  ので、省略し高速化した。v1.09

Copyright(C)2006-2013 Kitao Nakamura.
	改造版・後継版を公開なさるときは必ずソースコードを添付してください。
	その際に事後でかまいませんので、ひとことお知らせいただけると幸いです。
	商的な利用は禁じます。
	あとは「GNU General Public License(一般公衆利用許諾契約書)」に準じます。

*******************************************************************************
	[AudioOut.c]
		オーディオインタフェイスを DirectSound を利用して実装します。

		Implement audio interface using DirectSound.

	Copyright (C) 2004 Ki

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/
#define DIRECTSOUND_VERSION	0x0900	//DirectSound9のライブラリで処理を行う。9はパンチと品のバランスよし(SOFTバッファだとどのゲームも無難以上。HARDバッファにするとパンチ良しだがゲームによっては響きに品がなくなるケースも)＆CDDA最良(グラIIなどはSOFTバッファのほうが響きが綺麗)。									//			 8はパンチが強い(ソルジャーブレイドは最適だった。HARDバッファだと響きに品がない)。7でSOFTバッファだと音にややパンチが欠ける(特にソルジャーブレイド)がHARDバッファだとパンチと品のバランス最良。

#define _WIN32_DCOM //v2.19追加

#include <stdio.h>
#include <dsound.h>
#include "AudioOut9.h"
#include "AudioOut.h"
#include "WinMain.h"
#include "Printf.h"
#include "App.h"

//Kitao更新。ソフトでPSG,ADPCM,CDDAをミックスして音を鳴らした場合、ダイナミックレンジを1/3に下げなくてはいけないので音質が大きく落ちてしまう。そのため各Ch専用のバッファで鳴らすようにした。
static	LPDIRECTSOUND			_pDS	= NULL;
static	LPDIRECTSOUNDBUFFER		_pDSBP	= NULL; //Kitao追加。プライマリ用
static	LPDIRECTSOUNDBUFFER		_pDSB1	= NULL; //Kitao追加。PSG再生用
static	LPDIRECTSOUNDBUFFER		_pDSB2	= NULL; //Kitao追加。ADPCM再生用
static	LPDIRECTSOUNDBUFFER		_pDSB3	= NULL; //Kitao追加。CDDA再生用
static	LPVOID					_LpvPtr1; //Kitao追加。PSG再生用。少しでも速度アップするため配列にせずPSG,ADPCM,CDDA用それぞれの変数を用意。
static	DWORD					_DwBytes1; 
static	LPVOID					_LpvPtr2; //Kitao追加。ADPCM再生用
static	DWORD					_DwBytes2; 
static	LPVOID					_LpvPtr3; //Kitao追加。CDDA再生用
static	DWORD					_DwBytes3; 

static	LPDIRECTSOUNDNOTIFY		_LpDSN1; //Kitao更新。この構造体もリリースはDeinit時にした。3chぶん用意。
static	LPDIRECTSOUNDNOTIFY		_LpDSN2; //			  v2.36記：3ch独立させてバッファを監視することでそれぞれの音源の発生タイミングがしっかりと合う。
static	LPDIRECTSOUNDNOTIFY		_LpDSN3; //					   1chぶんのイベントだけで監視を済ましたら監視以外のチャンネルではタイミングがうまく取れなくて音質が落ちた。ドラゴンスレイヤー英雄伝説のADPCMドラムがわかりやすい。
static	DSBPOSITIONNOTIFY		_PosNotify1[AOUT9_BUFFERRATE+1]; //Kitao追加。PSG再生用
static	DSBPOSITIONNOTIFY		_PosNotify2[AOUT9_BUFFERRATE+1]; //Kitao追加。ADPCM再生用
static	DSBPOSITIONNOTIFY		_PosNotify3[AOUT9_BUFFERRATE+1]; //Kitao追加。CDDA再生用
static	HANDLE					_hEvent1[AOUT9_BUFFERRATE+1]; //Kitao追加。PSG再生用
static	HANDLE					_hEvent2[AOUT9_BUFFERRATE+1]; //Kitao追加。ADPCM再生用
static	HANDLE					_hEvent3[AOUT9_BUFFERRATE+1]; //Kitao追加。CDDA再生用

static	DWORD				_dwBufSize;
static	HANDLE				_hThread;
static	DWORD				_dwThreadID;

static	volatile BOOL		_bPlay; //v2.05更新。volatileに。
static	volatile BOOL		_bThreadEnd; //Kitao追加。スレッドを終了したいときTRUEにして知らせる。
static	volatile BOOL		_bThreadStarted; //v2.71追加。スレッドが確実に動き出した際TRUEに。

static	Sint16*				_pAudioBuf1 = NULL; //Kitao追加。Output時のPSG用
static	Sint16*				_pAudioBuf2 = NULL; //Kitao追加。Output時のADPCM用
static	Sint16*				_pAudioBuf3 = NULL; //Kitao追加。Output時のCDDA用
static 	void				(*_pCallBack)(int ch, Sint16* pBuf, Sint32 nSamples) = NULL; //Kitao更新。ch(チャンネルナンバー)を追加

static	BOOL				_bAudioInit = FALSE;


//v2.36更新
static void
buffer_lock_1()
{
	HRESULT		hr;

	hr = _pDSB1->Lock(0, _dwBufSize, &_LpvPtr1, &_DwBytes1, NULL, NULL, 0); //書き込み先のポインタを得る。v2.36更新。第2ブロックの指定をNULLにすることで、第1ブロックのみでロックできる。
	if (hr ==DSERR_BUFFERLOST) //失敗した場合リストアしてからもう一度試みる
	{
		_pDSB1->Restore();
		_pDSB1->Lock(0, _dwBufSize, &_LpvPtr1, &_DwBytes1, NULL, NULL, 0);
	}
}
static void
buffer_lock_2()
{
	HRESULT		hr;

	hr = _pDSB2->Lock(0, _dwBufSize, &_LpvPtr2, &_DwBytes2, NULL, NULL, 0);
	if (hr ==DSERR_BUFFERLOST)
	{
		_pDSB2->Restore();
		_pDSB2->Lock(0, _dwBufSize, &_LpvPtr2, &_DwBytes2, NULL, NULL, 0);
	}
}
static void
buffer_lock_3()
{
	HRESULT		hr;

	hr = _pDSB3->Lock(0, _dwBufSize, &_LpvPtr3, &_DwBytes3, NULL, NULL, 0);
	if (hr ==DSERR_BUFFERLOST)
	{
		_pDSB3->Restore();
		_pDSB3->Lock(0, _dwBufSize, &_LpvPtr3, &_DwBytes3, NULL, NULL, 0);
	}
}
static void
buffer_unlock_1()
{
	_pDSB1->Unlock(_LpvPtr1, _DwBytes1, NULL, NULL);
}
static void
buffer_unlock_2()
{
	_pDSB2->Unlock(_LpvPtr2, _DwBytes2, NULL, NULL);
}
static void
buffer_unlock_3()
{
	_pDSB3->Unlock(_LpvPtr3, _DwBytes3, NULL, NULL);
}

/*-----------------------------------------------------------------------------
	[write_streaming_buffer]
		DirectSoundBuffer に出力する音声データを書き込みます。
-----------------------------------------------------------------------------*/
//Kitao更新。v1.02でロックとアンロックをここでしないようにしたが、PC環境によってはすメモリのアンロックをしない音が再生されないドライバがあるらしい(MSDN談。ユーザーの方からも報告があった)ので、ここでロックとアンロックをする。v2.36
//			 _pAudioBufを廃止し、直接DirectSoundのバッファへ書き込むようにして高速化。v2.36

//Kitao更新。ここはPSG専用。
static inline void
write_streaming_buffer_1(
	DWORD	dwOffset)
{
	buffer_lock_1();
	if (_bPlay) //Kitao追加。初回再生時はバッファ全体ぶんの時間をウェイトする（PSGキューに値が用意される時間を待つ）ようにした。
		_pCallBack(1, (Sint16*)((DWORD)_LpvPtr1 + dwOffset), _DwBytes1/4); //Kitao更新。1ch目を処理 ※/4=「1サンプルが4バイト」の意味
	else
		ZeroMemory((LPVOID)((DWORD)_LpvPtr1 + dwOffset), _DwBytes1);
	buffer_unlock_1();
}

//Kitao追加。ここはADPCM専用。
static inline void
write_streaming_buffer_2(
	DWORD	dwOffset)
{
	buffer_lock_2();
	if (_bPlay)
		_pCallBack(2, (Sint16*)((DWORD)_LpvPtr2 + dwOffset), _DwBytes2/4);
	else
		ZeroMemory((LPVOID)((DWORD)_LpvPtr2 + dwOffset), _DwBytes2);
	buffer_unlock_2();
}

//Kitao更新。ここはCDDA専用。
static inline void
write_streaming_buffer_3(
	DWORD	dwOffset)
{
	buffer_lock_3();
	if (_bPlay)
		_pCallBack(3, (Sint16*)((DWORD)_LpvPtr3 + dwOffset), _DwBytes3/4);
	else
		ZeroMemory((LPVOID)((DWORD)_LpvPtr3 + dwOffset), _DwBytes3);
	buffer_unlock_3();
}


/*-----------------------------------------------------------------------------
	[playback_thread]
		サウンドバッファの更新を行なうスレッドです。 
-----------------------------------------------------------------------------*/
static DWORD WINAPI
playback_thread(
	LPVOID	param)
{
	DWORD		n;
	DWORD		dwOffset = 0;

	CoInitializeEx(NULL, COINIT_MULTITHREADED); //v2.19追加。それぞれのスレッドでCoInitializeExしないと安定しない環境もあるかもしれない。(COINIT_MULTITHREADEDの場合、COMを使うスレッドそれぞれでCoInitializeExすることをMSが推奨)

	while (!_bThreadEnd)
	{
		//PSGチャンネルの notification を待つ 
		n = WaitForMultipleObjects(AOUT9_BUFFERRATE+1, _hEvent1, FALSE, INFINITE);
		if (n != WAIT_OBJECT_0 + 0) //stopじゃなければ
		{
			//バッファの分岐点（先頭含む）を通知するイベントが発生した場合の処理 
			if (n == AOUT9_BUFFERRATE)
				dwOffset = 0;
			else
				dwOffset = n * _dwBufSize;
			if (AOUT_GetfpOutputWAV() != NULL) //Kiao追加。WAVファイルへ出力時
			{
				if (_bPlay)
					_pCallBack(1, _pAudioBuf1, _dwBufSize/4);
				else
					ZeroMemory(_pAudioBuf1, _dwBufSize);
				buffer_lock_1();
				CopyMemory((LPVOID)((DWORD)_LpvPtr1 + dwOffset), (LPBYTE)_pAudioBuf1, _DwBytes1);
				buffer_unlock_1();
			}
			else //通常
				write_streaming_buffer_1(dwOffset); //PSGチャンネル
		}

		if (APP_GetCDGame()) //Huカードゲームの処理を軽くする。v2.36追加
		{
			//ADPCMチャンネルの notification を待つ 
			n = WaitForMultipleObjects(AOUT9_BUFFERRATE+1, _hEvent2, FALSE, INFINITE);
			if (n != WAIT_OBJECT_0 + 0) //stopじゃなければ
			{
				//バッファの分岐点（先頭含む）を通知するイベントが発生した場合の処理 
				if (n == AOUT9_BUFFERRATE)
					dwOffset = 0;
				else
					dwOffset = n * _dwBufSize;
				if (AOUT_GetfpOutputWAV() != NULL) //Kiao追加。WAVファイルへ出力時
				{
					if (_bPlay)
						_pCallBack(2, _pAudioBuf2, _dwBufSize/4);
					else
						ZeroMemory(_pAudioBuf2, _dwBufSize);
					buffer_lock_2();
					CopyMemory((LPVOID)((DWORD)_LpvPtr2 + dwOffset), (LPBYTE)_pAudioBuf2, _DwBytes2);
					buffer_unlock_2();
				}
				else //通常
					write_streaming_buffer_2(dwOffset); //ADPCMチャンネル
			}

			//CDDAチャンネルの notification を待つ 
			n = WaitForMultipleObjects(AOUT9_BUFFERRATE+1, _hEvent3, FALSE, INFINITE);
			if (n != WAIT_OBJECT_0 + 0) //stopじゃなければ
			{
				//バッファの分岐点（先頭含む）を通知するイベントが発生した場合の処理 
				if (n == AOUT9_BUFFERRATE)
					dwOffset = 0;
				else
					dwOffset = n * _dwBufSize;
				if (AOUT_GetfpOutputWAV() != NULL) //Kiao追加。WAVファイルへ出力時
				{
					if (_bPlay)
						_pCallBack(3, _pAudioBuf3, _dwBufSize/4);
					else
						ZeroMemory(_pAudioBuf3, _dwBufSize);
					buffer_lock_3();
					CopyMemory((LPVOID)((DWORD)_LpvPtr3 + dwOffset), (LPBYTE)_pAudioBuf3, _DwBytes3);
					buffer_unlock_3();
				}
				else //通常
					write_streaming_buffer_3(dwOffset); //CDDAチャンネル
			}
		}

		//Kitao追加。WAVファイルへ出力処理
		if (AOUT_GetfpOutputWAV() != NULL)
			AOUT_OutputWavExecute(_dwBufSize, _pAudioBuf1, _pAudioBuf2, _pAudioBuf3);

		_bThreadStarted = TRUE; //v2.71追加。AudioOut用スレッドが確かに動き出した合図。
	}

	CoUninitialize(); //v2.19追加

	ExitThread(TRUE);
}


/*-----------------------------------------------------------------------------
	Deinit DirectSound

-----------------------------------------------------------------------------*/
#define SAFE_RELEASE(p)		{ if (p) {(p)->Release(); (p) = NULL;} }
static BOOL
d_deinit() 	//Kitao更新。3chぶんのリソースを開放
{
	int		i; //Kitao追加

	if (!_bAudioInit)
		return FALSE;

	_bPlay = FALSE;

	if (_hThread)
	{
		// スレッドの終了を待つ 
		for (i = 0; i<AOUT9_BUFFERRATE; i++) //ストリーミングバッファを無音で埋める
		{
			write_streaming_buffer_1(i*_dwBufSize);
			write_streaming_buffer_2(i*_dwBufSize);
			write_streaming_buffer_3(i*_dwBufSize);
		}
		_pDSB1->Play(0, 0, DSBPLAY_LOOPING);//もし再生していなかった場合は、_bThreadEndを受け付けないのでここで再生。
		_pDSB2->Play(0, 0, DSBPLAY_LOOPING);
		_pDSB3->Play(0, 0, DSBPLAY_LOOPING);
		_bThreadEnd = TRUE;
		WaitForSingleObject(_hThread, INFINITE);
		_pDSB1->Stop();
		_pDSB2->Stop();
		_pDSB3->Stop();

		CloseHandle(_hThread);
		_hThread = NULL;
	}

	for (i = 0; i<=AOUT9_BUFFERRATE; i++)
	{
		if (_hEvent1[i]) CloseHandle(_hEvent1[i]);
		if (_hEvent2[i]) CloseHandle(_hEvent2[i]);
		if (_hEvent3[i]) CloseHandle(_hEvent3[i]);
		_hEvent1[i] = NULL;
		_hEvent2[i] = NULL;
		_hEvent3[i] = NULL;
	}

	SAFE_RELEASE(_LpDSN1);//Kitao更新
	SAFE_RELEASE(_LpDSN2);
	SAFE_RELEASE(_LpDSN3);

	SAFE_RELEASE(_pDSB1);
	SAFE_RELEASE(_pDSB2);
	SAFE_RELEASE(_pDSB3);
	SAFE_RELEASE(_pDSBP);
	SAFE_RELEASE(_pDS);

	GlobalFree(_pAudioBuf1);
	GlobalFree(_pAudioBuf2);
	GlobalFree(_pAudioBuf3);
	_pAudioBuf1 = NULL;
	_pAudioBuf2 = NULL;
	_pAudioBuf3 = NULL;
	AOUT_DeinitWavMixBuf(); //WAV出力用のバッファを開放
	_dwBufSize = 0;

	return TRUE;
}


/*-----------------------------------------------------------------------------
	Initialize DirectSound

-----------------------------------------------------------------------------*/
static BOOL
d_init(
	Sint32	soundType, //Kitao追加。1…通常(ソフトバッファ＆ミキシング。CPUによるミキシングになり優しい感じの音質になる)。2…静的バッファ＆ソフトミキシング(おそらく古い処理だが、レンジが高く高音質。DirectSoundのバージョンによってはパンチが強すぎる場合も)。
	HWND	hWnd,
	WORD	nChannels,
	WORD	nSamplesPerSec,
	WORD	wBitsPerSample,
	DWORD	dwBufSize)			// in bytes
{
	int 					i;//Kitao追加
	DSBUFFERDESC			dsbd;
	WAVEFORMATEX			waveFormat;//Kitao更新

	// Create IDirectSound 
	if (FAILED(DirectSoundCreate(NULL, &_pDS, NULL)))
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: DIRECTSOUND::DirectSoundCreate() failed.    ", "Ootake", MB_OK); //Kitao追加
		return FALSE;
	}

	/*
	** Set coop level to DSSCL_PRIORITY
	**
	** プライマリバッファのフォーマットを設定できるよう、プライマリ協調レベル
	** を設定する。デフォルトのフォーマットに変更を加えない場合、入力の
	** フォーマットにかかわりなく、出力は 8 ビット、22 kHz フォーマットになる。
	** IDirectSoundBuffer::SetFormat の呼び出しが失敗しても問題はない点に
	** 注意する。DirectSound は単純に、利用できる中で最も近いフォーマットに
	** 設定する。
	*/
	if (FAILED(_pDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY)))
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: DIRECTSOUND::SetCooperativeLevel() failed.    ", "Ootake", MB_OK); //Kitao更新
		return FALSE;
	}

	/*
	** Get the primary buffer.
	**
	** プライマリバッファのフォーマットを設定するには、最初に
	** DSBUFFERDESC 構造体でそのフォーマットを記述し、次にその記述を
	** IDirectSound::CreateSoundBuffer メソッドに渡す。 
	*/
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize			= sizeof(DSBUFFERDESC);
	dsbd.dwFlags		= DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes	= 0;
	dsbd.lpwfxFormat	= NULL;

	if (FAILED(_pDS->CreateSoundBuffer(&dsbd, &_pDSBP, NULL))) //Kitao更新。v1.03。プライマリは作成した後すぐには開放しないようにした。
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: DIRECTSOUND::CreateSoundBuffer() failed.    ", "Ootake", MB_OK); //Kitao更新
		return FALSE;
	}

	/*
	** Set primary buffer to desired format.
	**
	** プライマリバッファオブジェクトを取得した後で、希望のウェーブ
	** フォーマットを記述し、その記述を IDirectSoundBuffer::SetFormat
	** メソッドに渡す。
	*/
	ZeroMemory(&waveFormat, sizeof(WAVEFORMATEX));
	waveFormat.wFormatTag		= WAVE_FORMAT_PCM;
	waveFormat.nChannels		= nChannels; 
	waveFormat.wBitsPerSample	= wBitsPerSample;
	waveFormat.nBlockAlign		= waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
	waveFormat.nSamplesPerSec	= nSamplesPerSec;
	waveFormat.nAvgBytesPerSec	= waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

	if (FAILED(_pDSBP->SetFormat(&waveFormat)))
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: DIRECTSOUNDBUFFER::SetFormat() failed.    ", "Ootake", MB_OK); //Kitao更新
//		return FALSE;
	}

	// DSBUFFERDESC 構造体を設定する。
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
	dsbd.dwSize	= sizeof(DSBUFFERDESC);
	switch (soundType) //Kitao更新。バッファ設定を選択できるようにした。v1.03。v1.31から２択に絞った。
	{
		case 2:
			//Kitao更新。バッファをDSBCAPS_STATICでハードメモリ上扱いに。音の解像度は確実に上がる。v2.70からこちらをデフォルトに。DirectSound7と相性がいい。
			dsbd.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS | DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE;
			break;
		default: //=1
			//Kitao更新。グローバルフォーカスにした。ソフトミキシング。優しい音質。
			dsbd.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE;
			break;
	}
	dsbd.dwBufferBytes	= dwBufSize * AOUT9_BUFFERRATE; //Kitao更新。AOUT9_BUFFERRATE倍ぶん用意する。
	dsbd.lpwfxFormat	= &waveFormat;

	// セカンダリバッファを作成する Kitao更新。3chぶん作成。
	if (FAILED(_pDS->CreateSoundBuffer(&dsbd, &_pDSB1, NULL)))	
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: AudioOut: Failed creating secondary buffer1.    ", "Ootake", MB_OK); //Kitao更新
		return FALSE;
	}
	if (FAILED(_pDS->CreateSoundBuffer(&dsbd, &_pDSB2, NULL)))	
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: AudioOut: Failed creating secondary buffer2.    ", "Ootake", MB_OK); //Kitao更新
		return FALSE;
	}
	if (FAILED(_pDS->CreateSoundBuffer(&dsbd, &_pDSB3, NULL)))	
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: AudioOut: Failed creating secondary buffer3.    ", "Ootake", MB_OK); //Kitao更新
		return FALSE;
	}

	//Kitao更新。3chぶん用意。[0]をstop notificationとし、[1]～[AOUT9_BUFFERRATE]までを分岐点イベントとした。
	for (i =0; i<=AOUT9_BUFFERRATE; i++)
	{
		_hEvent1[i] = CreateEvent(NULL, FALSE, FALSE, NULL); //v2.36更新。自動リセットするようにした。
		_hEvent2[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		_hEvent3[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	// DIRECTSOUNDNOTIFY のインタフェイスを得る 
	if (FAILED(_pDSB1->QueryInterface(IID_IDirectSoundNotify, (void**)&_LpDSN1)))	return FALSE;
	if (FAILED(_pDSB2->QueryInterface(IID_IDirectSoundNotify, (void**)&_LpDSN2)))	return FALSE;
	if (FAILED(_pDSB3->QueryInterface(IID_IDirectSoundNotify, (void**)&_LpDSN3)))	return FALSE;

	// Kitao更新。再生が停止されたときの[0]で処理することにして、[1]～[AOUT9_BUFFERRATE]ぶんまでを音の分岐点通知用とした。
	// 再生が停止されたときの notification 用 
	_PosNotify1[0].dwOffset = DSBPN_OFFSETSTOP;
	_PosNotify1[0].hEventNotify = _hEvent1[0];
	_PosNotify2[0].dwOffset = DSBPN_OFFSETSTOP;
	_PosNotify2[0].hEventNotify = _hEvent2[0];
	_PosNotify3[0].dwOffset = DSBPN_OFFSETSTOP;
	_PosNotify3[0].hEventNotify = _hEvent3[0];
	// Kitao更新。バッファの分岐点(先頭含む)通知用 
	for (i = 1; i<=AOUT9_BUFFERRATE; i++)
	{
		//_PosNotify1[i].dwOffset = (i-1)*dwBufSize;
		//v2.36更新。各バッファの切れ目よりも手前(半分の位置)でイベントを起こすようにした。反応が遅いドライバでもこれなら確実で、半分の位置なら早すぎる心配もないようだ。
		if (i == 1)
		{
			_PosNotify1[i].dwOffset = AOUT9_BUFFERRATE*dwBufSize - dwBufSize/2;
			_PosNotify2[i].dwOffset = AOUT9_BUFFERRATE*dwBufSize - dwBufSize/2;
			_PosNotify3[i].dwOffset = AOUT9_BUFFERRATE*dwBufSize - dwBufSize/2;
		}
		else
		{
			_PosNotify1[i].dwOffset = (i-1)*dwBufSize - dwBufSize/2;
			_PosNotify2[i].dwOffset = (i-1)*dwBufSize - dwBufSize/2;
			_PosNotify3[i].dwOffset = (i-1)*dwBufSize - dwBufSize/2;
		}
		_PosNotify1[i].hEventNotify = _hEvent1[i];
		_PosNotify2[i].hEventNotify = _hEvent2[i];
		_PosNotify3[i].hEventNotify = _hEvent3[i];
	}

	// notification を設定する 
	if (FAILED(_LpDSN1->SetNotificationPositions(AOUT9_BUFFERRATE+1, _PosNotify1)))	return FALSE;
	if (FAILED(_LpDSN2->SetNotificationPositions(AOUT9_BUFFERRATE+1, _PosNotify2)))	return FALSE;
	if (FAILED(_LpDSN3->SetNotificationPositions(AOUT9_BUFFERRATE+1, _PosNotify3)))	return FALSE;

	// オーディオバッファを確保する 
	_pAudioBuf1 = (Sint16*)GlobalAlloc(GMEM_FIXED, dwBufSize);
	_pAudioBuf2 = (Sint16*)GlobalAlloc(GMEM_FIXED, dwBufSize);
	_pAudioBuf3 = (Sint16*)GlobalAlloc(GMEM_FIXED, dwBufSize);
	if (_pAudioBuf1 == NULL)
	{
		d_deinit();
		return FALSE;
	}

	//WAV出力用のバッファを確保する
	AOUT_InitWavMixBuf(dwBufSize);

	_dwBufSize = dwBufSize;
	_bPlay = FALSE;

	//スレッドを開始する前にオーディオ初期化完了フラグをたてる。
	//[2004.04.28] fixed
	_bAudioInit = TRUE;

	//スレッドを作成し実行する 
	_bThreadEnd = FALSE;
	_bThreadStarted = FALSE; //v2.71
	_hThread = CreateThread(NULL, 0, playback_thread, NULL, 0, &_dwThreadID);
	if (_hThread == NULL)
	{
		d_deinit();
		_bAudioInit = FALSE;
		return FALSE;
	}

	//スレッドの優先順位を上げる。v2.36追加。効果が無かったためカット
	//OpenThread(THREAD_SET_INFORMATION, TRUE, _dwThreadID);
	//if (SetThreadPriority(_hThread, THREAD_PRIORITY_HIGHEST) == NULL)
	//	return FALSE;
		
	return TRUE;
}


/*-----------------------------------------------------------------------------
	[Init]
		
-----------------------------------------------------------------------------*/
BOOL
AOUT9_Init(
	Sint32		soundType,	//Kitao追加
	Uint32		bufSize,	// in samples 
	Uint32		sampleRate,
	void		(*pCallBack)(int ch, Sint16* pBuf, Sint32 nSamples)) //Kitao更新。ch(チャンネルナンバー)を追加
{
	if (d_init(soundType, WINMAIN_GetHwnd(), 2, (WORD)sampleRate, 16, (DWORD)bufSize*2*2))
	{
		_pCallBack = pCallBack;
		return TRUE;
	}

	return FALSE;
}


/*-----------------------------------------------------------------------------
	[Play]
		
-----------------------------------------------------------------------------*/
void
AOUT9_Play(
	BOOL	bPlay)
{
	if (!_bAudioInit)
		return;

	_bPlay = bPlay;
	if (_bPlay)
	{
		_pDSB1->Play(0, 0, DSBPLAY_LOOPING);
		_pDSB2->Play(0, 0, DSBPLAY_LOOPING);
		_pDSB3->Play(0, 0, DSBPLAY_LOOPING);
	}
/*  //ここで音をStopするとバッファに残った音が次のゲームを開くときなどに出てしまうのでStopせずに放置で行く。※XAudio2の場合、逆に止めないと負荷高しで環境により誤動作。
	else
	{	//v2.70追加
		_pDSB1->Stop();
		_pDSB2->Stop();
		_pDSB3->Stop();
	}
*/
}


/*-----------------------------------------------------------------------------
	[Deinit]
		
-----------------------------------------------------------------------------*/
void
AOUT9_Deinit()
{
	if (!_bAudioInit)
		return;

	d_deinit();

	_bAudioInit = FALSE;
}


//Kitao追加
void
AOUT9_SetPlayStart()
{
	int i;

	if (!_bAudioInit)
		return;

	// WAVファイル出力用Bufferに無音を書き込む 
	ZeroMemory(_pAudioBuf1, _dwBufSize);
	ZeroMemory(_pAudioBuf2, _dwBufSize);
	ZeroMemory(_pAudioBuf3, _dwBufSize);

	// ストリーミングバッファを無音で埋める
	for (i = 0; i<AOUT9_BUFFERRATE; i++) //Kitao追加
	{
		write_streaming_buffer_1(i*_dwBufSize);
		write_streaming_buffer_2(i*_dwBufSize);
		write_streaming_buffer_3(i*_dwBufSize);
	}

	// 再生位置を0にする
	_pDSB1->SetCurrentPosition(0);
	_pDSB2->SetCurrentPosition(0);
	_pDSB3->SetCurrentPosition(0);
}

//Kitao追加
BOOL
AOUT9_GetPlay()
{
	return _bPlay;
}

//Kitao追加
BOOL
AOUT9_GetThreadStarted()
{
	if (!_bAudioInit)
		return TRUE;
	else
		return _bThreadStarted;
}

