/******************************************************************************
Ootake

	[AudioOutX.c]
		オーディオインタフェイスを XAudio2 を利用して実装します。

Copyright(C)2006-2013 Kitao Nakamura.
	改造版・後継版を公開なさるときは必ずソースコードを添付してください。
	その際に事後でかまいませんので、ひとことお知らせいただけると幸いです。
	商的な利用は禁じます。
	あとは「GNU General Public License(一般公衆利用許諾契約書)」に準じます。

*******************************************************************************/
#define _WIN32_DCOM //v2.19追加

#include <stdio.h>
#include <xaudio2.h>
#include "AudioOutX.h"
#include "AudioOut.h"
#include "WinMain.h"
#include "Printf.h"
#include "App.h"

//Kitao更新。ソフトでPSG,ADPCM,CDDAをミックスして音を鳴らした場合、ダイナミックレンジを1/3に下げなくてはいけないので音質が大きく落ちてしまう。そのため各Ch専用のバッファで鳴らすようにした。
static	IXAudio2*				_pXA	= NULL;
static	IXAudio2MasteringVoice*	_pMV	= NULL; //マスタリングボイス
static	IXAudio2SourceVoice*	_pSV1	= NULL; //PSG再生用
static	IXAudio2SourceVoice*	_pSV2	= NULL; //ADPCM再生用
static	IXAudio2SourceVoice*	_pSV3	= NULL; //CDDA再生用
static	XAUDIO2_BUFFER			_SV1BUF = {0};
static	XAUDIO2_BUFFER			_SV2BUF = {0};
static	XAUDIO2_BUFFER			_SV3BUF = {0};
static	BYTE*					_pBuf1[AOUTX_BUFFERRATE]; //PSG再生用。バッファは途切れを起こさないために３つぶんに分割。
static	Sint32					_BufN1;
static	BYTE*					_pBuf2[AOUTX_BUFFERRATE]; //ADPCM再生用
static	Sint32					_BufN2;
static	BYTE*					_pBuf3[AOUTX_BUFFERRATE]; //CDDA再生用
static	Sint32					_BufN3;

class VoiceCallback: public IXAudio2VoiceCallback
{
	public:
		HANDLE hBufferEndEvent;
		VoiceCallback(): hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)){}
		~VoiceCallback(){CloseHandle(hBufferEndEvent);}
		STDMETHOD_(void, OnBufferEnd(void *)){SetEvent(hBufferEndEvent);}
		STDMETHOD_(void, OnStreamEnd)(){}
		STDMETHOD_(void, OnVoiceProcessingPassEnd)(){}
		STDMETHOD_(void, OnVoiceProcessingPassStart(UINT32)){}
		STDMETHOD_(void, OnBufferStart(void *)){}
		STDMETHOD_(void, OnLoopEnd(void *)){}
		STDMETHOD_(void, OnVoiceError(void *, HRESULT)){}
};
static	VoiceCallback		_VoiceCallback1;
static	VoiceCallback		_VoiceCallback2;
static	VoiceCallback		_VoiceCallback3;

static UINT32 OperationSetCounter = 0;
static UINT32 _OperationID = UINT32(InterlockedIncrement(LPLONG(&OperationSetCounter)));

static	DWORD				_dwBufSize;
static	HANDLE				_hThread;
static	DWORD				_dwThreadID;

static	volatile BOOL		_bPlay; //v2.05更新。volatileに。
static	volatile BOOL		_bThreadEnd; //Kitao追加。スレッドを終了したいときTRUEにして知らせる。
static	volatile BOOL		_bThreadStarted; //v2.71追加。スレッドが確実に動き出した際TRUEに。

static 	void				(*_pCallBack)(int ch, Sint16* pBuf, Sint32 nSamples) = NULL; //Kitao更新。ch(チャンネルナンバー)を追加

static	BOOL				_bAudioInit = FALSE;


/*-----------------------------------------------------------------------------
	[write_streaming_buffer]
		XAudio2のバッファ に出力する音声データを書き込みます。
-----------------------------------------------------------------------------*/

//Kitao更新。ここはPSG専用。
static inline void
write_streaming_buffer_1()
{
	if (_bPlay) //再生中のみバッファへキュー
	{
		_pCallBack(1, (Sint16*)(_pBuf1[_BufN1]), _dwBufSize/4); //1ch目を処理 ※/4=「1サンプルが4バイト」の意味
		_SV1BUF.pAudioData = _pBuf1[_BufN1];
		_pSV1->SubmitSourceBuffer(&_SV1BUF, NULL); //※キューした後も、バッファ内のデータは再生が終わるまで残さなければならない。そのため_pBuf1も配列として3つ持つ必要がある。
		_BufN1++;
		_BufN1 %= AOUTX_BUFFERRATE;
	}
}

//Kitao追加。ここはADPCM専用。
static inline void
write_streaming_buffer_2()
{
	if (_bPlay)
	{
		_pCallBack(2, (Sint16*)(_pBuf2[_BufN2]), _dwBufSize/4); //1ch目を処理 ※/4=「1サンプルが4バイト」の意味
		_SV2BUF.pAudioData = _pBuf2[_BufN2];
		_pSV2->SubmitSourceBuffer(&_SV2BUF);
		_BufN2++;
		_BufN2 %= AOUTX_BUFFERRATE;
	}
}

//Kitao更新。ここはCDDA専用。
static inline void
write_streaming_buffer_3()
{
	if (_bPlay)
	{
		_pCallBack(3, (Sint16*)(_pBuf3[_BufN3]), _dwBufSize/4); //1ch目を処理 ※/4=「1サンプルが4バイト」の意味
		_SV3BUF.pAudioData = _pBuf3[_BufN3];
		_pSV3->SubmitSourceBuffer(&_SV3BUF);
		_BufN3++;
		_BufN3 %= AOUTX_BUFFERRATE;
	}
}


/*-----------------------------------------------------------------------------
	[playback_thread]
		サウンドバッファの更新を行なうスレッドです。 
-----------------------------------------------------------------------------*/
static DWORD WINAPI
playback_thread(
	LPVOID	param)
{
	DWORD					dwOffset = 0;
	XAUDIO2_VOICE_STATE		state;
	Sint32					bn1, bn2, bn3;						

	CoInitializeEx(NULL, COINIT_MULTITHREADED); //v2.19追加。それぞれのスレッドでCoInitializeExしないと安定しない環境もあるかもしれない。(COINIT_MULTITHREADEDの場合、COMを使うスレッドそれぞれでCoInitializeExすることをMSが推奨)

	while (!_bThreadEnd)
	{
		//PSGチャンネルのバッファが１つぶん以上空になるまで待つ
		while (_bPlay)
		{
			_pSV1->GetState(&state);
			if (state.BuffersQueued <= AOUTX_BUFFERRATE-1) break;
			WaitForSingleObject(_VoiceCallback1.hBufferEndEvent, INFINITE);
		}
		//バッファにデータを送る
		write_streaming_buffer_1(); //PSGチャンネル

		if (APP_GetCDGame()) //Huカードゲームの処理を軽くする。v2.36追加
		{
			//ADPCMチャンネルのバッファが１つぶん以上空になるまで待つ。ここで再び同期をとらないとPSGと噛み合わないことがあった。
			while (_bPlay)
			{
				_pSV2->GetState(&state);
				if (state.BuffersQueued <= AOUTX_BUFFERRATE-1) break;
				WaitForSingleObject(_VoiceCallback2.hBufferEndEvent, INFINITE);
			}
			//バッファにデータを送る
			write_streaming_buffer_2(); //ADPCMチャンネル

			//CDDAチャンネルのバッファが１つぶん以上空になるまで待つ
			while (_bPlay)
			{
				_pSV3->GetState(&state);
				if (state.BuffersQueued <= AOUTX_BUFFERRATE-1) break;
				WaitForSingleObject(_VoiceCallback3.hBufferEndEvent, INFINITE);
			}
			//バッファにデータを送る
			write_streaming_buffer_3(); //CDDAチャンネル
		}

		//Kitao追加。WAVファイルへ出力処理
		if (AOUT_GetfpOutputWAV() != NULL)
		{
			if (_bPlay)
			{
				if (_BufN1==0) bn1=AOUTX_BUFFERRATE-1; else bn1=_BufN1-1;
				if (_BufN2==0) bn2=AOUTX_BUFFERRATE-1; else bn2=_BufN2-1;
				if (_BufN3==0) bn3=AOUTX_BUFFERRATE-1; else bn3=_BufN3-1;
				AOUT_OutputWavExecute(_dwBufSize, (Sint16*)_pBuf1[bn1], (Sint16*)_pBuf2[bn2], (Sint16*)_pBuf3[bn3]);
			}
		}

		_bThreadStarted = TRUE; //v2.71追加。AudioOut用スレッドが確かに動き出した合図。
	}

	CoUninitialize(); //v2.19追加

	ExitThread(TRUE);
}


/*-----------------------------------------------------------------------------
	Deinit XAudio2

-----------------------------------------------------------------------------*/
static BOOL
x_deinit() 	//Kitao更新。3chぶんのリソースを開放
{
	int		i;

	if (!_bAudioInit)
		return FALSE;

	_bPlay = FALSE;

	if (_hThread)
	{
		//スレッドの終了を待つ 
		_bThreadEnd = TRUE;
		SetEvent(_VoiceCallback1.hBufferEndEvent);//バッファチェックのループからすぐ抜ける
		SetEvent(_VoiceCallback2.hBufferEndEvent);//
		SetEvent(_VoiceCallback3.hBufferEndEvent);//
		WaitForSingleObject(_hThread, INFINITE);

		CloseHandle(_hThread);
		_hThread = NULL;
	}

	_pSV1->DestroyVoice();
	_pSV2->DestroyVoice();
	_pSV3->DestroyVoice();
	_pSV1 = NULL;
	_pSV2 = NULL;
	_pSV3 = NULL;

	_pMV->DestroyVoice();
	_pMV = NULL;

	_pXA->Release();
	_pXA = NULL;

	for (i = 0; i<AOUTX_BUFFERRATE; i++)
	{
		GlobalFree(_pBuf1[i]);
		GlobalFree(_pBuf2[i]);
		GlobalFree(_pBuf3[i]);
		_pBuf1[i] = NULL;
		_pBuf2[i] = NULL;
		_pBuf3[i] = NULL;
	}
	AOUT_DeinitWavMixBuf(); //WAV出力用のバッファを開放
	_dwBufSize = 0;

	return TRUE;
}


/*-----------------------------------------------------------------------------
	Initialize XAudio2

-----------------------------------------------------------------------------*/
static BOOL
x_init(
	Sint32	soundType, //DirectSound用。Audio2では無効。
	HWND	hWnd,
	WORD	nChannels,
	WORD	nSamplesPerSec,
	WORD	wBitsPerSample,
	DWORD	dwBufSize)			// in bytes
{
	int				i;
	WAVEFORMATEX	waveFormat;//Kitao更新

	//XAudio2を初期化
	if (FAILED(XAudio2Create(&_pXA, 0, XAUDIO2_DEFAULT_PROCESSOR)))
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: XAUDIO2::XAudio2Create() failed.    ", "Ootake", MB_OK);
		return FALSE;
	}

	//マスタリングボイスを作成
	if (FAILED(_pXA->CreateMasteringVoice(&_pMV, XAUDIO2_DEFAULT_CHANNELS, 44100, 0, 0, NULL)))
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: XAUDIO2::CreateMasteringVoice() failed.    ", "Ootake", MB_OK);
		return FALSE;
	}

	//Waveフォーマットを設定
	ZeroMemory(&waveFormat, sizeof(WAVEFORMATEX));
	waveFormat.wFormatTag		= WAVE_FORMAT_PCM;
	waveFormat.nChannels		= nChannels; 
	waveFormat.wBitsPerSample	= wBitsPerSample;
	waveFormat.nBlockAlign		= waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
	waveFormat.nSamplesPerSec	= nSamplesPerSec;
	waveFormat.nAvgBytesPerSec	= waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

	//ソースボイスを作成。3chぶん作成。
	if (FAILED(_pXA->CreateSourceVoice(&_pSV1, &waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &_VoiceCallback1, NULL, NULL)))
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: XAUDIO2::CreateSourceVoice() failed.    ", "Ootake", MB_OK);
		return FALSE;
	}
	if (FAILED(_pXA->CreateSourceVoice(&_pSV2, &waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &_VoiceCallback2, NULL, NULL)))
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: XAUDIO2::CreateSourceVoice() failed.    ", "Ootake", MB_OK);
		return FALSE;
	}
	if (FAILED(_pXA->CreateSourceVoice(&_pSV3, &waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &_VoiceCallback3, NULL, NULL)))
	{
		MessageBox(WINMAIN_GetHwnd(), "ERROR: XAUDIO2::CreateSourceVoice() failed.    ", "Ootake", MB_OK);
		return FALSE;
	}

	//バッファを設定。3chぶん作成。
	_dwBufSize = dwBufSize;
	_SV1BUF.AudioBytes	= _dwBufSize;
	_SV1BUF.Flags		= XAUDIO2_END_OF_STREAM;
	_SV2BUF.AudioBytes	= _dwBufSize;
	_SV2BUF.Flags		= XAUDIO2_END_OF_STREAM;
	_SV3BUF.AudioBytes	= _dwBufSize;
	_SV3BUF.Flags		= XAUDIO2_END_OF_STREAM;

	//オーディオバッファを確保する 
	for (i = 0; i<AOUTX_BUFFERRATE; i++)
	{
		_pBuf1[i] = (BYTE*)GlobalAlloc(GMEM_FIXED, _dwBufSize);
		_pBuf2[i] = (BYTE*)GlobalAlloc(GMEM_FIXED, _dwBufSize);
		_pBuf3[i] = (BYTE*)GlobalAlloc(GMEM_FIXED, _dwBufSize);
	}
	_BufN1 = 0;
	_BufN2 = 0;
	_BufN3 = 0;

	//WAV出力用のバッファを確保する
	AOUT_InitWavMixBuf(_dwBufSize);

	_bPlay = FALSE;

	// スレッドを開始する前にオーディオ初期化完了フラグをたてる。
	// [2004.04.28] fixed
	_bAudioInit = TRUE;

	// スレッドを作成し実行する 
	_bThreadEnd = FALSE;
	_bThreadStarted = FALSE; //v2.71
	_hThread = CreateThread(NULL, 0, playback_thread, NULL, 0, &_dwThreadID);
	if (_hThread == NULL)
	{
		x_deinit();
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
AOUTX_Init(
	Sint32		soundType,	//Kitao追加
	Uint32		bufSize,	// in samples 
	Uint32		sampleRate,
	void		(*pCallBack)(int ch, Sint16* pBuf, Sint32 nSamples)) //Kitao更新。ch(チャンネルナンバー)を追加
{
	if (x_init(soundType, WINMAIN_GetHwnd(), 2, (WORD)sampleRate, 16, (DWORD)bufSize*2*2))
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
AOUTX_Play(
	BOOL	bPlay)
{
	if (!_bAudioInit)
		return;

	_bPlay = bPlay;
	if (_bPlay)
	{
		_pSV1->Start(0, _OperationID);
		_pSV2->Start(0, _OperationID);
		_pSV3->Start(0, _OperationID);
		_pXA->CommitChanges(_OperationID);
	}
	else
	{
		_pSV1->Stop(0, _OperationID);
		_pSV2->Stop(0, _OperationID);
		_pSV3->Stop(0, _OperationID);
		_pXA->CommitChanges(_OperationID);
	}
}


/*-----------------------------------------------------------------------------
	[Deinit]
		
-----------------------------------------------------------------------------*/
void
AOUTX_Deinit()
{
	if (!_bAudioInit)
		return;

	x_deinit();

	_bAudioInit = FALSE;
}


//Kitao追加
void
AOUTX_SetPlayStart()
{
	if (!_bAudioInit)
		return;

	// 再生中バッファの内容を削除
	_pSV1->FlushSourceBuffers();
	_pSV2->FlushSourceBuffers();
	_pSV3->FlushSourceBuffers();

	_BufN1 = 0;
	_BufN2 = 0;
	_BufN3 = 0;
}


//Kitao追加
BOOL
AOUTX_GetPlay()
{
	return _bPlay;
}

//Kitao追加
BOOL
AOUTX_GetThreadStarted()
{
	if (!_bAudioInit)
		return TRUE;
	else
		return _bThreadStarted;
}

