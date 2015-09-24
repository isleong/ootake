/******************************************************************************
Ootake
・Direct3Dでの描画にも対応した。Vista対応。
・Window表示にもDirectDrawを使うようにした。
・早回し機能を付けた。

Copyright(C)2006-2013 Kitao Nakamura.
	改造版・後継版を公開なさるときは必ずソースコードを添付してください。
	その際に事後でかまいませんので、ひとことお知らせいただけると幸いです。
	商的な利用は禁じます。
	あとは「GNU General Public License(一般公衆利用許諾契約書)」に準じます。

*******************************************************************************
	[Screen.c]

	Implement ScreenInterface.

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
#include <stdio.h>
#include <math.h> //pow()関数で必要
#include "Screen.h"
#include "ScreenDD.h"
#include "ScreenD3D.h"
//#include "GDIScreen.h"
#include "TIMER.h"
#include "VDC.h"
#include "CDROM.h"
#include "APU.h"
#include "App.h"
#include "MainBoard.h"
#include "WinMain.h"
#include "Printf.h"

Uint32 _Gamma[8]; //Kitao追加。ガンマを計算した数値を入れておく。v1.14高速化。Uint32にしたほうが処理速かった。
Uint32 _GammaS80[8]; //Kitao追加。スキャンライン80%用
Uint32 _GammaS90[8]; //Kitao追加。スキャンライン90%用
Uint32 _MonoTableR[256]; //モノクロ変換用テーブル。高速化のため必要。v2.28
Uint32 _MonoTableG[256]; //
Uint32 _MonoTableB[256]; //

static Sint32	_Width;
static Sint32	_Height;
static Sint32	_Magnification;	//※ここ(Screen.cpp)での_Magnificationはスクリーンショット用表示処理のときは、それと一致しないことがあるので注意。v2.28記
static Sint32	_BitsPerPixel;  //※ここ(Screen.cpp)での_BitsPerPixelは、"DirectDrawフルスクリーンカラーの設定"の値であって、現在表示中のBitsPerPixelと一致しているとは限らないので注意。v2.28記
static Uint32	_Flags;

//Kitao追加。v2.70
static Uint32	_SBSwapBuf[40*16]; //40(横ドット)*16(縦ドット)*sizeof(Uint32)
static Uint32	_SBWorldStadium91[]   = {0x00030303,0x00070707,0x00000000,0x00000000,0x00070700,0x00070700};
static Uint32	_SBWorldStadium[]     = {0x00000006,0x00000002,0x00000002,0x00000002,0x00070700,0x00070700};
static Uint32	_SBWorldStadium2[]    = {0x00000006,0x00000002,0x00000002,0x00000500,0x00000500,0x00000500};
static Uint32	_SBPowerLeague34593[] = {0x00070701,0x00070701,0x00000000,0x00000000,0x00000000,0x00070701};
static Uint32	_SBPowerLeague2[]     = {0x00070701,0x00070701,0x00000000,0x00000000,0x00000000,0x00070701};
static Uint32	_SBPowerLeague[]      = {0x00010101,0x00060605,0x00060605,0x00060605,0x00060605,0x00010101};
static Uint32	_SBEikanKimi1[]       = {0x00070707,0x00000301,0x00000301,0x00000301,0x00070707};
static Uint32	_SBEikanKimi2[]       = {0x00060606,0x00000200,0x00000200,0x00000200,0x00060606};
static Uint32	_SBEikanKimi3[]       = {0x00050505,0x00000100,0x00000100,0x00000100,0x00050505};
static Uint32	_SBEikanKimi4[]       = {0x00040404,0x00000000,0x00000000,0x00000000,0x00040404};
static Uint32	_SBEikanKimi5[]       = {0x00030303,0x00000000,0x00000000,0x00000000,0x00030303};
static Uint32	_SBEikanKimi6[]       = {0x00020202,0x00000000,0x00000000,0x00000000,0x00020202};
static Uint32	_SBEikanKimi7[]       = {0x00010101,0x00000000,0x00000000,0x00000000,0x00010101};
static Uint32	_SBEikanKimi8[]       = {0x00000000,0x00010101,0x00010101,0x00000000,0x00000000};
static Uint32	_SBNaxatStadium[]     = {0x00050507,0x00050507,0x00050507,0x00050507,0x00050507,0x00000000};
static Uint32	_SBRomRomStadium[]    = {0x00000000,0x00000000,0x00070707,0x00070707,0x00070707,0x00000000};
static Uint32	_SBBaseballer1[]      = {0x00070707,0x00040507,0x00000202,0x00000202,0x00000202,0x00070707};
static Uint32	_SBBaseballer2[]      = {0x00040404,0x00020304,0x00000101,0x00000101,0x00000101,0x00040404};
static Uint32	_SBBaseballer3[]      = {0x00030303,0x00020303,0x00000101,0x00000101,0x00000101,0x00030303};
static Uint32	_SBKoregaPro1[]       = {0x00010204,0x00000000,0x00000000,0x00000000,0x00000000};
static Uint32	_SBKoregaPro2[]       = {0x00010203,0x00000000,0x00000000,0x00000000,0x00000000};
static Uint32	_SBKoregaPro3[]       = {0x00010202,0x00000000,0x00000000,0x00000000,0x00000000};
static Uint32	_SBKoregaPro4[]       = {0x00010101,0x00000000,0x00000000,0x00000000,0x00000000};
static Uint32	_SBKoregaPro5[]       = {0x00000103,0x00060606,0x00060606,0x00060606,0x00060606};
static Uint32	_SBKoregaPro6[]       = {0x00000002,0x00050505,0x00050505,0x00050505,0x00050505};
static Uint32	_SBKoregaPro7[]       = {0x00000001,0x00040404,0x00040404,0x00040404,0x00040404};
static Uint32	_SBKoregaPro8[]       = {0x00000000,0x00030303,0x00030303,0x00030303,0x00030303};
static Uint32	_SBKoregaPro9[]       = {0x00000000,0x00020202,0x00020202,0x00020202,0x00020202};
static Uint32	_SBKoregaProA[]       = {0x00000000,0x00010101,0x00010101,0x00010101,0x00010101};
static Uint32	_SBTheProYakyuu1[]    = {0x00000000,0x00070707,0x00070707,0x00070707,0x00070707};
static Uint32	_SBTheProYakyuu2[]    = {0x00000000,0x00060606,0x00060606,0x00060606,0x00060606};
static Uint32	_SBTheProYakyuu3[]    = {0x00000000,0x00050505,0x00050505,0x00050505,0x00050505};
static Uint32	_SBTheProYakyuu4[]    = {0x00000000,0x00040404,0x00040404,0x00040404,0x00040404};
static Uint32	_SBTheProYakyuu11[]   = {0x00000000,0x00070707,0x00070707,0x00000000,0x00070707};
static Uint32	_SBTheProYakyuu12[]   = {0x00000000,0x00060606,0x00060606,0x00000000,0x00060606};
static Uint32	_SBTheProYakyuu13[]   = {0x00000000,0x00050505,0x00050505,0x00000000,0x00050505};
static Uint32	_SBTheProYakyuu14[]   = {0x00000000,0x00040404,0x00040404,0x00000000,0x00040404};
static Uint32	_SBTheProYakyuu15[]   = {0x00000000,0x00030303,0x00030303,0x00000000,0x00030303};
static Uint32	_SBTheProYakyuu16[]   = {0x00000000,0x00020202,0x00020202,0x00000000,0x00020202};
static Uint32	_SBTheProYakyuu17[]   = {0x00000000,0x00010101,0x00010101,0x00000000,0x00010101};
static Uint32	_SBTheProYakyuu21[]   = {0x00000000,0x00000000,0x00070707,0x00070707,0x00000000};
static Uint32	_SBTheProYakyuu22[]   = {0x00000000,0x00000000,0x00060606,0x00060606,0x00000000};
static Uint32	_SBTheProYakyuu23[]   = {0x00000000,0x00000000,0x00050505,0x00050505,0x00000000};
static Uint32	_SBTheProYakyuu24[]   = {0x00000000,0x00000000,0x00040404,0x00040404,0x00000000};
static Uint32	_SBTheProYakyuu25[]   = {0x00000000,0x00000000,0x00030303,0x00030303,0x00000000};
static Uint32	_SBTheProYakyuu26[]   = {0x00000000,0x00000000,0x00020202,0x00020202,0x00000000};
static Uint32	_SBTheProYakyuu27[]   = {0x00000000,0x00000000,0x00010101,0x00010101,0x00000000};

//Kitao追加。他アプリのウィンドウの状態を保存しておくための変数。v2.24
static HWND				_OtherAppWindowHWnd[512];
static WINDOWPLACEMENT	_OtherAppWindowPlacement[512];
static Sint32			_OtherAppWindowN;


//Kitao追加。ウィンドウ位置保存のためのコールバック。v2.24
static BOOL
CALLBACK EnumWindowsSaveProc(HWND hWnd, LPARAM lParam)
{
	if ((IsWindowVisible(hWnd))&&(hWnd != WINMAIN_GetHwnd()))
	{
		if (_OtherAppWindowN < 512)
		{
			_OtherAppWindowHWnd[_OtherAppWindowN] = hWnd;
			_OtherAppWindowPlacement[_OtherAppWindowN].length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hWnd, &_OtherAppWindowPlacement[_OtherAppWindowN]);
			_OtherAppWindowN++;
		}
	}
	return TRUE;
}

//Kitao追加。ウィンドウ位置保存。v2.24
void
SCREEN_SaveWindowPosition()
{
	_OtherAppWindowN = 0;
	EnumWindows(EnumWindowsSaveProc, NULL);
	//PRINTF("WindowN %d",_OtherAppWindowN); //test
}

//Kitao追加。ウィンドウ位置を戻すためのコールバック。v2.24
static BOOL
CALLBACK EnumWindowsLoadProc(HWND hWnd, LPARAM lParam)
{
	int		i;	

	for (i=0; i<_OtherAppWindowN; i++)
		if (_OtherAppWindowHWnd[i] == hWnd)
		{
			SetWindowPlacement(hWnd, &_OtherAppWindowPlacement[i]);
			break;
		}
	return TRUE;
}

//Kitao追加。ウィンドウ位置を戻す。v2.24
void
SCREEN_LoadWindowPosition()
{
	EnumWindows(EnumWindowsLoadProc, NULL);
}


/*-----------------------------------------------------------------------------
	[Init]
		スクリーンモードを初期化(変更)します。 Kitao更新。v2.28
-----------------------------------------------------------------------------*/
BOOL
SCREEN_Init(
	Sint32		width,
	Sint32		height,
	Sint32		magnification, //Kitao追加
	Uint32		bitsPerPixel,
	Uint32		flags)
{
	BOOL	ret;
	Uint32	i;

	_Width = width;
	_Height = height;
	_Magnification = magnification; //Kitao追加
	_BitsPerPixel = bitsPerPixel;
	_Flags = flags;

	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_Deinit();
			ret = SCREEND3D_Init(_Width, _Height, _Magnification, _Flags);
			break;
		case 2: //DirectDraw
			SCREENDD_Deinit();
			ret = SCREENDD_Init(_Width, _Height, _Magnification, _BitsPerPixel, _Flags);
			break;
		default:
			ret = FALSE;
			break;
	}

	//モノクロ変換用テーブルを作成。v2.28追加
	if ((APP_GetDrawMethod() == 2)&&(SCREENDD_GetBitsPerPixel() == 16))
	{	//16bitカラー（DirectDrawのみ）
		for (i=0; i<32; i++)
		{
			//R,G,Bの輝度を平均化してモノクロ化
			_MonoTableR[i] = (Uint32)((pow((i * 0.298912) / 32.0, 1.0/1.076900) * 32.0)); //※画面の暗さを抑えるためガンマも上げる。
			_MonoTableG[i] = (Uint32)((pow((i * 0.586611) / 32.0, 1.0/1.076900) * 32.0)); //※RGB足したときに値オーバーしないために小数点以下は切り捨て。
			_MonoTableB[i] = (Uint32)((pow((i * 0.114478) / 32.0, 1.0/1.076900) * 32.0)); //※16bitは切り捨てられたぶんの暗さが大きいのでそこも考慮してガンマを決定。
		}
	}
	else
	{	//32bitカラー
		for (i=0; i<256; i++)
		{
			//R,G,Bの輝度を平均化してモノクロ化
			_MonoTableR[i] = (Uint32)((pow((i * 0.298912) / 256.0, 1.0/1.0752080) * 256.0)); //※画面の暗さを抑えるためガンマも上げる。
			_MonoTableG[i] = (Uint32)((pow((i * 0.586611) / 256.0, 1.0/1.0752080) * 256.0)); //※RGB足したときに値オーバーしないために小数点以下は切り捨て。
			_MonoTableB[i] = (Uint32)((pow((i * 0.114478) / 256.0, 1.0/1.0752080) * 256.0)); //
		}
	}

	return ret;
}


/*-----------------------------------------------------------------------------
	[Deinit]
		スクリーンの終了処理を行ないます。
-----------------------------------------------------------------------------*/
void
SCREEN_Deinit()
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_Deinit();
			break;
		case 2: //DirectDraw
			SCREENDD_Deinit();
			break;
	}
}


/*-----------------------------------------------------------------------------
	[ToggleFullScreen]
		スクリーンをウインドウ／フルスクリーンに切り替えます．
-----------------------------------------------------------------------------*/
BOOL
SCREEN_ToggleFullScreen()
{
	if (_Flags & SCREEN_FFULLSCREEN)
		_Flags &= ~SCREEN_FFULLSCREEN;
	else
		_Flags |= SCREEN_FFULLSCREEN;
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_Deinit();
			return SCREEND3D_Init(_Width, _Height, _Magnification, _Flags);
		case 2: //DirectDraw
			SCREENDD_Deinit();
			return SCREENDD_Init(_Width, _Height, _Magnification, _BitsPerPixel, _Flags);
		default:
			return FALSE;
	}
}


/*-----------------------------------------------------------------------------
	[WaitVBlank]
		垂直帰線期間を待ちます。 
-----------------------------------------------------------------------------*/
//Kitao更新
BOOL
SCREEN_WaitVBlank(
	BOOL	bDraw) //bDrawをTRUEにして呼ぶと描画も行う。FALSEの場合VSync待ちのみ。Direct3D利用時用。Kitao追加。
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			return SCREEND3D_WaitVBlank(bDraw);
		case 2: //DirectDraw
			return SCREENDD_WaitVBlank(FALSE); //DirectDrawのときは常に描画は行われない。
		default:
			return FALSE;
	}
}


void*
SCREEN_GetBuffer()
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			return SCREEND3D_GetBuffer();
		case 2: //DirectDraw
			return SCREENDD_GetBuffer();
		default:
			return NULL;
	}
}


const Sint32
SCREEN_GetBufferPitch()
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			return SCREEND3D_GetBufferPitch();
		case 2: //DirectDraw
			return SCREENDD_GetBufferPitch();
		default:
			return 0;
	}
}


/*-----------------------------------------------------------------------------
	[FillRect]
		バックバッファに指定の色の矩形を描きます。
	呼ぶ前に SCREEN_Lock() しましょう。
-----------------------------------------------------------------------------*/
void
SCREEN_FillRect(
	Sint32		x,
	Sint32		y,
	Sint32		width,
	Sint32		height,
	Uint32		color)
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_FillRect(x, y, width, height, color);
			break;
		case 2: //DirectDraw
			SCREENDD_FillRect(x, y, width, height, (Uint16)color);
			break;
	}
}


//Kitao追加。スクリーン全体をクリアする。v1.43
void
SCREEN_Clear(
	byte	r,
	byte	g,
	byte	b) //v2.73更新。任意の色でクリアできるようにした。※16ビットモードのときはモノクロ以外だと色化けするので現在はモノクロ固定でrのデータのみ使用。
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_Clear(r,g,b);
			break;
		case 2: //DirectDraw
			SCREENDD_Clear(r,g,b);
			break;
	}
}


/*-----------------------------------------------------------------------------
	[Blt]
		pSrc からバックバッファへ画像を書き込みます。拡大／縮小、
	呼ぶ前に SCREEN_Lock() しましょう。
-----------------------------------------------------------------------------*/
//Kitao更新。ラインごとに解像度を変えているゲーム(龍虎の拳，あすか120%など)に対応。
void
SCREEN_Blt(
	Uint32*		pSrc,
	Sint32		srcX,
	Sint32		srcY,
	Uint16*		pSrcW,	//Kitao更新。転送元の横ピクセル数。※srcHラインの数ぶん
	Sint32		srcH,	//Kitao更新。dstWとdstH はカットした。(ここで固定せず、様々な大きさでのペーストに対応するため)
	Sint32		executeCode)  //Kitao追加。実行コード。0…エンコードだけ行う。1…プライマリ画面へ転送も行う。
							  //					   3…左右に黒帯(オーバースキャン部)を配置しての転送(あとは1と同じ)
							  //					   5…左右のオーバースキャン部をカットしての転送(あとは1と同じ)
{
	int		a,i;

	if (APP_GetStrikeBallSwap())
	{
		//ワールドスタジアム'91でストライクとボールの表示をスワップする。v2.70
		if (MAINBOARD_GetWorldStadium91())
			if ((srcY <= 16+136)&&(srcY+srcH >= 16+144+7))
				if (memcmp(_SBWorldStadium91, pSrc+(16+136)*512+14, 4*6) == 0) //ストライクが表示されていたら
				{
					for (i = 0; i < 8; i++)
						memcpy(_SBSwapBuf+i*40, pSrc+(16+136+i)*512+16, 4*32);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+136+i)*512+16, pSrc+(16+144+i)*512+16, 4*32);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+144+i)*512+16, _SBSwapBuf+i*40, 4*32);
				}
		//ワールドスタジアム(初代)でストライクとボールの表示をスワップする。v2.70
		if (MAINBOARD_GetWorldStadium())
			if ((srcY <= 16+136)&&(srcY+srcH >= 16+144+7))
				if (memcmp(_SBWorldStadium, pSrc+(16+136)*512+14, 4*6) == 0) //ストライクが表示されていたら
				{
					for (i = 0; i < 8; i++)
						memcpy(_SBSwapBuf+i*40, pSrc+(16+136+i)*512+16, 4*32);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+136+i)*512+16, pSrc+(16+144+i)*512+16, 4*32);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+144+i)*512+16, _SBSwapBuf+i*40, 4*32);
				}
				//else if (memcmp(_SBWorldStadium2, pSrc+(16+144)*512+14, 4*6) == 0) //ボールカウントだけが表示されていた場合。代打が残り２人のときのタイム時に必要。
				//{
				//	for (i = 0; i < 8; i++)
				//		memcpy(pSrc+(16+144+i)*512+16, pSrc+(16+176+i)*512+16, 4*32); //ボールカウント表示を消す。現状は消さないほうが自然なのでボツ。
				//}
		//パワーリーグ３,４,５,'93でストライクとボールの表示をスワップする。v2.70
		if (MAINBOARD_GetPowerLeague34593())
			if ((srcY <= 16+136)&&(srcY+srcH >= 16+146+7))
				if (memcmp(_SBPowerLeague34593, pSrc+(16+137)*512+210, 4*6) == 0) //ストライクが表示されていたら
				{
					for (i = 0; i < 8; i++)
						memcpy(_SBSwapBuf+i*40, pSrc+(16+136+i)*512+210, 4*8);
					for (i = 0; i < 8; i++)
						memcpy(_SBSwapBuf+(8+i)*40, pSrc+(16+146+i)*512+210, 4*8);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+136+i)*512+210, pSrc+(16+136+i)*512+222, 4*8);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+146+i)*512+210, pSrc+(16+146+i)*512+222, 4*8);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+136+i)*512+222, _SBSwapBuf+i*40, 4*8);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+146+i)*512+222, _SBSwapBuf+(8+i)*40, 4*8);
				}
		//パワーリーグ２でストライクとボールの表示をスワップする。v2.70
		if (MAINBOARD_GetPowerLeague2())
			if ((srcY <= 16+136)&&(srcY+srcH >= 16+144+7))
				if (memcmp(_SBPowerLeague2, pSrc+(16+137)*512+216, 4*6) == 0) //ストライクが表示されていたら
				{
					for (i = 0; i < 8; i++)
						memcpy(_SBSwapBuf+i*40, pSrc+(16+136+i)*512+216, 4*32);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+136+i)*512+216, pSrc+(16+144+i)*512+216, 4*32);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+144+i)*512+216, _SBSwapBuf+i*40, 4*32);
				}
		//パワーリーグのスコアボード画面で、でストライクとボールの表示をスワップする。試合中のカウントはスプライトなのでVDC.cppにておこなう。v2.70
		if (VDC_GetPowerLeague())
			if ((srcY <= 16+121)&&(srcY+srcH >= 16+129+7))
				if (memcmp(_SBPowerLeague, pSrc+(16+121)*512+145, 4*6) == 0) //ストライクが表示されていたら
				{
					for (i = 0; i < 8; i++)
						memcpy(_SBSwapBuf+i*40, pSrc+(16+121+i)*512+144, 4*32);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+121+i)*512+144, pSrc+(16+129+i)*512+144, 4*32);
					for (i = 0; i < 8; i++)
						memcpy(pSrc+(16+129+i)*512+144, _SBSwapBuf+i*40, 4*32);
				}
		//栄冠は君にのスコアボード画面で、ストライクとボールの表示をスワップする。試合中のカウントはスプライトなのでVDP.cppにておこなう。v2.70
		if (VDC_GetEikanKimi())
		{
			if ((srcY <= 16+9)&&(srcY+srcH >= 16+17+8)&&
				(memcmp(_SBEikanKimi1, pSrc+(16+10)*512+287, 4*5) == 0)) //ストライクが表示されていたら。自チーム対戦のスコアボード表示時
			{
				for (i = 0; i < 8; i++)
					memcpy(_SBSwapBuf+i*40, pSrc+(16+9+i)*512+286, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+9+i)*512+286, pSrc+(16+17+i)*512+286, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+17+i)*512+286, _SBSwapBuf+i*40, 4*32);
			}
			else if ((srcY <= 16+9)&&(srcY+srcH >= 16+17+7)&&
					 ((memcmp(_SBEikanKimi8, pSrc+(16+21)*512+313, 4*5) == 0)||
					  (memcmp(_SBEikanKimi2, pSrc+(16+10)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi3, pSrc+(16+10)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi4, pSrc+(16+10)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi5, pSrc+(16+10)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi6, pSrc+(16+10)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi7, pSrc+(16+10)*512+287, 4*5) == 0))) //ストライクが表示されていたら。試合終了後
			{
				for (i = 0; i < 8; i++)
					memcpy(_SBSwapBuf+i*40, pSrc+(16+9+i)*512+286, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+9+i)*512+286, pSrc+(16+17+i)*512+286, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+17+i)*512+286, _SBSwapBuf+i*40, 4*32);
			}
			else if ((srcY <= 16+7)&&(srcY+srcH >= 16+15+7)&&
					 ((memcmp(_SBEikanKimi8, pSrc+(16+19)*512+313, 4*5) == 0)||
					  (memcmp(_SBEikanKimi2, pSrc+(16+ 8)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi3, pSrc+(16+ 8)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi4, pSrc+(16+ 8)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi5, pSrc+(16+ 8)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi6, pSrc+(16+ 8)*512+287, 4*5) == 0)||
					  (memcmp(_SBEikanKimi7, pSrc+(16+ 8)*512+287, 4*5) == 0))) //ストライクが表示されていたら。他チーム同士の対戦時
			{
				for (i = 0; i < 8; i++)
					memcpy(_SBSwapBuf+i*40, pSrc+(16+7+i)*512+286, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+7+i)*512+286, pSrc+(16+15+i)*512+286, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+15+i)*512+286, _SBSwapBuf+i*40, 4*32);
			}
		}
		//ナグザットスタジアム
		if (MAINBOARD_GetNaxatStadium())
		{
			if ((srcY <= 16+168)&&(srcY+srcH >= 16+176+7)&&
				(memcmp(_SBNaxatStadium, pSrc+(16+182)*512+17, 4*6) == 0)) //アウトカウントが表示されていたら
			{
				for (i = 0; i < 8; i++)
					memcpy(_SBSwapBuf+i*40, pSrc+(16+168+i)*512+16, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+168+i)*512+16, pSrc+(16+176+i)*512+16, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+176+i)*512+16, _SBSwapBuf+i*40, 4*32);
			}
		}
		//ROMROMスタジアム
		if (CDROM_GetRomRomStadium())
		{
			if ((srcY <= 16+185)&&(srcY+srcH >= 16+193+7)&&
				(memcmp(_SBRomRomStadium, pSrc+(16+185)*512+16, 4*6) == 0)) //ストライクが表示されていたら
			{
				if (*(pSrc+(16+185)*512+25) == 0x00000000) //代打ウィンドウ非表示時
					a = 40;
				else
					a = 8;
				for (i = 0; i < 8; i++)
					memcpy(_SBSwapBuf+i*40, pSrc+(16+185+i)*512+16, 4*a);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+185+i)*512+16, pSrc+(16+193+i)*512+16, 4*a);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+193+i)*512+16, _SBSwapBuf+i*40, 4*a);
			}
		}
		//熱血レジェンドベースボーラー
		if (CDROM_GetBaseballer())
		{
			if ((srcY <= 16+64)&&(srcY+srcH >= 16+72+7)&&
				((memcmp(_SBBaseballer1, pSrc+(16+65)*512+17, 4*6) == 0)||
				 (memcmp(_SBBaseballer2, pSrc+(16+65)*512+17, 4*6) == 0)||
				 (memcmp(_SBBaseballer3, pSrc+(16+65)*512+17, 4*6) == 0))) //ストライクが表示されていた
			{
				for (i = 0; i < 8; i++)
					memcpy(_SBSwapBuf+i*40, pSrc+(16+64+i)*512+16, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+64+i)*512+16, pSrc+(16+72+i)*512+16, 4*32);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+72+i)*512+16, _SBSwapBuf+i*40, 4*32);
			}
		}
		//これがプロ野球'89＆これがプロ野球'90
		if (MAINBOARD_GetKoregaPro())
		{
			if ((srcY <= 16+144)&&(srcY+srcH >= 16+152+7)&&
				((memcmp(_SBKoregaPro1, pSrc+(16+153)*512+224, 4*5) == 0)|| //ボールの３つ目のカウントが表示されていたら
				 (memcmp(_SBKoregaPro2, pSrc+(16+153)*512+224, 4*5) == 0)||
				 (memcmp(_SBKoregaPro3, pSrc+(16+153)*512+224, 4*5) == 0)||
				 (memcmp(_SBKoregaPro4, pSrc+(16+153)*512+224, 4*5) == 0)||
				 (memcmp(_SBKoregaPro5, pSrc+(16+145)*512+192, 4*5) == 0)|| //ストライクが表示されていたら
				 (memcmp(_SBKoregaPro6, pSrc+(16+145)*512+192, 4*5) == 0)||
				 (memcmp(_SBKoregaPro7, pSrc+(16+145)*512+192, 4*5) == 0)||
				 (memcmp(_SBKoregaPro8, pSrc+(16+145)*512+192, 4*5) == 0)||
				 (memcmp(_SBKoregaPro9, pSrc+(16+145)*512+192, 4*5) == 0)||
				 (memcmp(_SBKoregaProA, pSrc+(16+145)*512+192, 4*5) == 0)))
			{
				for (i = 0; i < 8; i++)
					memcpy(_SBSwapBuf+i*40, pSrc+(16+144+i)*512+192, 4*40);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+144+i)*512+192, pSrc+(16+152+i)*512+192, 4*40);
				for (i = 0; i < 8; i++)
					memcpy(pSrc+(16+152+i)*512+192, _SBSwapBuf+i*40, 4*40);
			}
		}
		if (VDC_GetTheProYakyuu()) //インテックCD野球３作（ザ・プロ野球，ザ・プロ野球SUPER，ザ・プロ野球SUPER'94）
		{
			if ((srcY <= 16+113)&&(srcY+srcH >= 16+128)&&
				((memcmp(_SBTheProYakyuu1, pSrc+(16+113)*512+212, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu2, pSrc+(16+113)*512+212, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu3, pSrc+(16+113)*512+212, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu4, pSrc+(16+113)*512+212, 4*5) == 0))) //打席画面でボールカウントが表示されていたら
			{
				if (*(pSrc+(16+120)*512+213) != 0x00000000) //まだ未修正の場合
				{
					memcpy(pSrc+(16+128)*512+212, pSrc+(16+120)*512+212, 4*2); //O(アウト)の左上背景を修正
					*(pSrc+(16+120)*512+213) = 0x00000000; //B(ボール)の左下背景を修正
				}
			}
			if ((srcY <= 16+165)&&(srcY+srcH >= 16+169)&&
				((memcmp(_SBTheProYakyuu11, pSrc+(16+169)*512+38, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu12, pSrc+(16+169)*512+38, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu13, pSrc+(16+169)*512+38, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu14, pSrc+(16+169)*512+38, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu15, pSrc+(16+169)*512+38, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu16, pSrc+(16+169)*512+38, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu17, pSrc+(16+169)*512+38, 4*5) == 0))) //フィールド画面でボールカウントが表示されていたら
			{
				if ((*(pSrc+(16+166)*512+62) != 0x00070003)&&
					((*(pSrc+(16+166)*512+62) != 0x00000000)||(*(pSrc+(16+169)*512+39) != 0x00070707))) //ピンク色(＆黒縁)のランナーがディスティネーションでなければorフェードアウト時
						*(pSrc+(16+166)*512+62) = *(pSrc+(16+169)*512+39); //VDC.cppで処理できなかった部分（一二塁間白線）をここで処理
			}
			if ((srcY <= 16+174)&&(srcY+srcH >= 16+175)&&
				((memcmp(_SBTheProYakyuu21, pSrc+(16+175)*512+40, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu22, pSrc+(16+175)*512+40, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu23, pSrc+(16+175)*512+40, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu24, pSrc+(16+175)*512+40, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu25, pSrc+(16+175)*512+40, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu26, pSrc+(16+175)*512+40, 4*5) == 0)||
				 (memcmp(_SBTheProYakyuu27, pSrc+(16+175)*512+40, 4*5) == 0))) //フィールド画面でストライクが表示されていたら
			{
				memcpy(pSrc+(16+175)*512+47, pSrc+(16+174)*512+47, 4*3); //VDC.cppで処理できなかった部分（ラスタ単位でキャラ色が書き換えられている）をここで処理
				memcpy(pSrc+(16+175)*512+53, pSrc+(16+174)*512+53, 4*3); //同上
			}
		}
	}
	//Uint32* pT = pSrc+(16+113)*512+212; PRINTF("Color = %08X %08X %08X %08X %08X %08X", *pT,*(pT+1),*(pT+2),*(pT+3),*(pT+4),*(pT+5)); //Test用

	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_Blt(pSrc, srcX, srcY, pSrcW, srcH, executeCode);
			break;
		case 2: //DirectDraw
			SCREENDD_Blt(pSrc, srcX, srcY, pSrcW, srcH, executeCode);
			break;
	}
}


//Kitao追加。VSync(垂直帰線待ち)を行うかどうかを設定。現在のディスプレイ表示環境でVSyncが行えるかどうかのチェックも行う。
void
SCREEN_SetSyncTo60HzScreen(
	BOOL	bSyncTo60HzScreen)
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_SetSyncTo60HzScreen(bSyncTo60HzScreen);
			break;
		case 2: //DirectDraw
			SCREENDD_SetSyncTo60HzScreen(bSyncTo60HzScreen);
			break;
	}
}

//Kitao追加。VSync(垂直帰線待ち)を行っているかどうかを得る（現在のディスプレイ表示環境でVSyncが行えるかどうかのチェックを反映した値）。
BOOL
SCREEN_GetSyncTo60HzScreen()
{
	BOOL	bSyncTo60HzScreen;

	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			bSyncTo60HzScreen = SCREEND3D_GetSyncTo60HzScreen();
			break;
		case 2: //DirectDraw
			bSyncTo60HzScreen = SCREENDD_GetSyncTo60HzScreen();
			break;
	}

	return bSyncTo60HzScreen;
}


//Kitao追加。テキストメッセージを設定
void
SCREEN_SetMessageText(
	char*	pText)
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_SetMessageText(pText);
			break;
		case 2: //DirectDraw
			SCREENDD_SetMessageText(pText);
			break;
	}
}


//Kitao追加。ガンマ（明るさ調整）を計算済みのテーブルを用意。v2.28更新。Direct3DとDirectDrawで共用にした。
void
SCREEN_SetGamma(
	Sint32	scanLineType,
	Sint32	scanLineDensity, //スキャンラインの濃度(%)
	BOOL	bTvMode)
{
	Sint32	magnification;
	Sint32	bitsPerPixel;
	int 	a,i;
	double	d = APP_GetGammaValue(); //縦横スキャンライン時の基本ガンマ値
	Sint32	b = APP_GetBrightValue(); //ブライトネス

	//基本設定ではなく、現在実際に表示されているMagnificationとBitsPerPixelを取得する。v2.28
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			magnification = SCREEND3D_GetMagnification();
			bitsPerPixel = 32; //32bit固定
			break;
		case 2: //DirectDraw
			magnification = SCREENDD_GetMagnification();
			bitsPerPixel = SCREENDD_GetBitsPerPixel();
			break;
		default:
			magnification = 2;
			bitsPerPixel = 32;
			break;
	}

	if (bTvMode)
	{
		if ((scanLineType != 0)&&(magnification >= 2)) //TV Mode でx2,x3,x4のとき
			scanLineType = 4;
	}
	else
	{
		if (scanLineType == 4) //スタートTV Mode が設定してあって、TvModeが一時的にFALSEの状態なら
			scanLineType = 1; //スペシャル(縦横)スキャンラインで描画
	}

	if ((scanLineType != 0)&&(magnification >= 2)) //スキャンラインの場合、スキャンラインで暗くなるぶん、ガンマを明るめに。
	{
		switch (scanLineType)
		{
			case 1: //縦横スキャンライン
				break;
			case 2: //横だけスキャンライン
			case 3: //縦だけスキャンライン(未実装)
			case 4: //TV Mode
				d = (1-15/800)*d; // (1-15/800)*1.305 縦横時と比べて明るいぶんを引く
				if (APP_GetOptimizeGamma())
					d = d * (1+(80-(double)scanLineDensity)*0.005); // ダウンした明るさ分を上げる。v2.35更新
				break;
			default:
				d = (1-95/800)*d;
				break;
		}
	}
	else //ノンスキャンラインもしくはx1の場合
		d = (1-95/800)*d; // (1-95/800)*1.305 縦横時と比べて明るいぶんを引く

	if ((scanLineType>=2)&&(scanLineType<=4)) //横スキャンラインの場合。v2.35更新
	{
		if (bitsPerPixel == 16)
		{
			for (i=0; i<=7 ; i++)
			{
				if (i == 0)
					a = 0; //黒は真っ黒に
				else
					a = (i << 2) + b; //+1。白が白く見える範囲で小さめにしたほうが目にきつくない。
				_Gamma[i] = (Uint32)((pow((double)a / 32.0, 1.0/d) * 32.0) +0.5);
				_GammaS80[i] = (Uint32)((pow((double)a*((double)scanLineDensity / 100) / 32.0, 1.0/d) * 32.0) +0.5); //スキャンライン本線用
				_GammaS90[i] = (Uint32)((pow((double)a*((double)(scanLineDensity+(100-scanLineDensity)/2) / 100) / 32.0, 1.0/d) * 32.0) +0.5); //スキャンラインとドットの境界用
			}
		}
		else //32ビットカラーの場合
		{
			for (i=0; i<=7 ; i++)
			{
				if (i == 0)
					a = 0; //黒は真っ黒に
				else
					a = (i << 5) + b; //+1。白が白く見える範囲で小さめにしたほうが目にきつくない。
				_Gamma[i] = (Uint32)((pow((double)a / 256.0, 1.0/d) * 256.0) +0.5);
				_GammaS80[i] = (Uint32)((pow((double)a*((double)scanLineDensity / 100) / 256.0, 1.0/d) * 256.0) +0.5); //スキャンライン本線用
				_GammaS90[i] = (Uint32)((pow((double)a*((double)(scanLineDensity+(100-scanLineDensity)/2) / 100) / 256.0, 1.0/d) * 256.0) +0.5); //スキャンラインとドットの境界用
			}
		}
	}
	else //横スキャンライン以外の場合。横スキャンライン80%を基準とする。v2.35更新
	{
		if (bitsPerPixel == 16)
		{
			for (i=0; i<=7 ; i++)
			{
				a = i*4 + (Sint32)((double)b/9 * ((double)i/7) + 0.5); //黒は真っ黒に。白は白く見える範囲で小さめにしたほうが目にきつくない。v2.35更新
				_Gamma[i] = (Uint32)((pow((double)a / 32.0, 1.0/d) * 32.0) +0.5);
				_GammaS80[i] = (Uint32)((pow((double)a*0.80 / 32.0, 1.0/d) * 32.0) +0.5); //スキャンライン80%用（スキャンライン本線）
				_GammaS90[i] = (Uint32)((pow((double)a*0.90 / 32.0, 1.0/d) * 32.0) +0.5); //スキャンライン90%用（スキャンラインとドットの境界用）
			}
		}
		else //32ビットカラーの場合
		{
			for (i=0; i<=7 ; i++)
			{
				a = i*32 + (Sint32)((double)b * ((double)i/7) + 0.5); //黒は真っ黒に。白は白く見える範囲で小さめにしたほうが目にきつくない。v2.35更新
				_Gamma[i] = (Uint32)((pow((double)a / 256.0, 1.0/d) * 256.0) +0.5);
				_GammaS80[i] = (Uint32)((pow((double)a*0.80 / 256.0, 1.0/d) * 256.0) +0.5); //スキャンライン80%用（スキャンライン本線）
				_GammaS90[i] = (Uint32)((pow((double)a*0.90 / 256.0, 1.0/d) * 256.0) +0.5); //スキャンライン90%用（スキャンラインとドットの境界用）
			}
		}
	}
}


//Kitao追加。前回のVBlank待ちが終わった時刻を返す。
DWORD
SCREEN_GetLastTimeSyncTime()
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			return SCREEND3D_GetLastTimeSyncTime();
		case 2: //DirectDraw
			return SCREENDD_GetLastTimeSyncTime();
		default:
			return 0;
	}
}


//Kitao追加。スクリーンショットのBitmapを書き込む。v2.12
void
SCREEN_WriteScreenshot(
	FILE*	fp)
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_WriteScreenshot(fp);
			break;
		case 2: //DirectDraw
			SCREENDD_WriteScreenshot(fp);
			break;
	}
}


//Kitao追加。描画倍率を設定する。v2.36
void
SCREEN_SetMagnification(
	Sint32	magnification)
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			SCREEND3D_SetMagnification(magnification);
			break;
		case 2: //DirectDraw
			SCREENDD_SetMagnification(magnification);
			break;
	}
}


//Kitao追加。実際に「描画処理で使用している」描画倍率を得る。v2.36
Sint32
SCREEN_GetMagnification()
{
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			return SCREEND3D_GetMagnification();
		case 2: //DirectDraw
			return SCREENDD_GetMagnification();
	}
	return 0;
}


//Kitao追加。描画時のドット拡大率を設定する。
void
SCREEN_SetPixelMagnification(
	Sint32*		wMag,
	Sint32*		hMag)
{
	Sint32	magnification;

	//基本設定ではなく、現在実際に表示されているMagnificationを取得する。v2.28
	switch (APP_GetDrawMethod())
	{
		case 1: //Direct3D
			magnification = SCREEND3D_GetMagnification();
			break;
		case 2: //DirectDraw
			magnification = SCREENDD_GetMagnification();
			break;
		default:
			magnification = 2;
			break;
	}

	if (magnification >= 2) //x2以上の場合
	{
		if (APP_GetScanLineType() != 0)
		{
			if (APP_GetTvMode())
				*wMag = 1;
			else
				*wMag = 2; //x2以上も横は２倍ドット固定。（速度アップ＋拡大されたときにバイリニアフィルタがいい感じにかかる）
			*hMag = magnification; //縦は倍率ぶんのソースを用意して転送。
		}
		else
		{
			*wMag = 2;
			if (magnification == 2)
				*hMag = magnification; //縦は倍率ぶんのソースを用意して転送。
			else
				*hMag = magnification-1; //3x,4xのときは、それぞれ2x,3xに拡大。（ジャギー軽減＆速度アップ）
		}
	}
	else //x1の場合
		*wMag = *hMag = 1;
}


//Kitao追加。ディスプレイの垂直走査周波数を測定して返す。v2.43
Sint32
SCREEN_GetVerticalScanFrequency()
{
	Sint32	vsf = 0;
	Sint32	a;
	DWORD	t, t2;
	BOOL	bHideMessage;

	if (APP_GetFullScreen())
	{
		bHideMessage = APP_GetFullHideMessage(); //退避
		APP_SetFullHideMessage(FALSE); //メッセージを必ず画面内に表示。
	}
	else
	{
		bHideMessage = APP_GetHideMessage(); //退避
		APP_SetHideMessage(FALSE); //表示速度が間に合うように、メッセージを必ず画面内に表示。
	}
	PRINTF("Checking Now... Please wait for 60 seconds.");
	MAINBOARD_ScreenUpdateFast();

	SetCursor(LoadCursor(NULL, IDC_WAIT)); //カーソルを砂時計に。

	SCREEN_WaitVBlank(FALSE); //"前回VBlankが終わった時刻"を更新するために必要。
	t = timeGetTime();
	t2 = t + 60000;
	while (t2 < t) //終了予定時刻のタイマーカウンタがオーバーフローしていた場合、開始予定時刻のタイマーカウンタが0に戻るまで待つ。
	{
		SCREEN_WaitVBlank(FALSE); //"前回VBlankが終わった時刻"を更新するために必要。
		t = timeGetTime();
		t2 = t + 60000;
	}
	
	while (timeGetTime() <= t2)
	{
		vsf++;
		Sleep(12); //12。連続V-Sync処理による計測不良を防ぐためウェイトが必要(とくにV-Sync設定オフ時に必要)。11だと計測不良(早送り現象)があった。あまり大きくSleepさうると遅いマシンで逆に処理落ちによる計測不良。
		SCREEN_WaitVBlank(FALSE);
	}

	vsf -= 6; //誤差があるのか、-6でちょうど実際の周波数表記(ディスプレイのinfoボタン)と同じになった。全ての環境でこれが当てはまらないと意味がないので要確認。
	vsf = vsf * 10 / 6;
	//四捨五入
	a = vsf % 10; //一の位を退避
	vsf /= 10; //一の位をカット
	if (a < 2)
		vsf *= 10;
	else if (a < 7)
		vsf = vsf * 10 + 5;
	else
		vsf = vsf * 10 + 10;

	//メッセージ表示設定を元に戻す。
	if (APP_GetFullScreen())
		APP_SetFullHideMessage(bHideMessage);
	else
		APP_SetHideMessage(bHideMessage);

	SetCursor(LoadCursor(NULL, IDC_ARROW)); //カーソルを元に戻す

	return vsf;
}
