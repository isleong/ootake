/******************************************************************************
Ootake

 [Srartup.c]
	�X�^�[�g���̃��b�Z�[�W�\����CD-ROM�EHu�J�[�h�̑I��

Copyright(C)2006-2013 Kitao Nakamura.
	�����ŁE��p�ł����J�Ȃ���Ƃ��͕K���\�[�X�R�[�h��Y�t���Ă��������B
	���̍ۂɎ���ł��܂��܂���̂ŁA�ЂƂ��Ƃ��m�点����������ƍK���ł��B
	���I�ȗ��p�͋ւ��܂��B
	���Ƃ́uGNU General Public License(��ʌ��O���p�����_��)�v�ɏ����܂��B

******************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "Startup.h"
#include "resource.h"
#include "WinMain.h"
#include "Option.h"
#include "App.h"

#define LINE_LEN	92
#define N_LINES 	25

#define BUTTON_CDROM	1
#define BUTTON_CUE		2
#define BUTTON_HUCARD	3
#define BUTTON_REPLAY	4
#define BUTTON_OPTION	5
#define BUTTON_FAVORITE	6


static HBRUSH		_hMyb; //����u���V�F
static HFONT		_hFontB; //�{�^���p�t�H���g

static Uint32		_FontWidth;
static Uint32		_FontHeight;
static const char*	_pCaption = "To Enjoy \"Ootake\" Truly.";
static HINSTANCE	_hInstance = NULL;
static HWND 		_hWnd;


static Sint32		_CDorHu = -1; //�߂�l�BCD�Ȃ�1�BCD��CUE�N���Ȃ�2�BHu�J�[�h�Ȃ�3�B���v���C�Ȃ�4�B�L�����Z���Ȃ�-1�B���ݒ蒆��0�B
static Sint32*		_pCDorHu;


/* �t�H���g�̍������擾���� */
static Uint32
get_font_height(
	HWND			hWnd)
{
	HDC 			hDC;
	HFONT			hFont;
	HFONT			hFontOld;
	TEXTMETRIC		tm;

	hDC 	 = GetDC(hWnd);
	hFont	 = (HFONT)GetStockObject(OEM_FIXED_FONT);	 /* �Œ�s�b�`�t�H���g */
	hFontOld = (HFONT)SelectObject(hDC, hFont);

	GetTextMetrics(hDC, &tm);

	SelectObject(hDC, hFontOld);
	DeleteObject(hFont);
	ReleaseDC(hWnd, hDC);

	return (Uint32)(tm.tmHeight);
}

/* �t�H���g�̉������擾���� */
static Uint32
get_font_width(
	HWND			hWnd)
{
	HDC 			hDC;
	HFONT			hFont;
	HFONT			hFontOld;
	TEXTMETRIC		tm;

	hDC 	 = GetDC(hWnd);
	hFont	 = (HFONT)GetStockObject(OEM_FIXED_FONT);	 /* �Œ�s�b�`�t�H���g */
	hFontOld = (HFONT)SelectObject(hDC, hFont);

	GetTextMetrics(hDC, &tm);

	SelectObject(hDC, hFontOld);
	DeleteObject(hFont);
	ReleaseDC(hWnd, hDC);

	return (Uint32)tm.tmAveCharWidth;
}


static void
set_window_size(
	HWND			hWnd)
{
	RECT		rc;
	Uint32		wndW = _FontWidth  * LINE_LEN +2;
	Uint32		wndH = _FontHeight * N_LINES -2;
	Uint32		a;

	SetRect(&rc, 0, 0, wndW, wndH);
	AdjustWindowRectEx(&rc, GetWindowLong(hWnd, GWL_STYLE),
						GetMenu(hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE));
	wndW = rc.right - rc.left;
	wndH = rc.bottom - rc.top;
	GetWindowRect(WINMAIN_GetHwnd(), &rc);
	a = rc.bottom - rc.top - 16 -(rc.right - rc.left - 598)/2/*���g�����g�̕�*/; //a=���C���E�B���h�E�̏�g�̍����B16�̓��C���E�B���h�E(WinMain)�̃N���C�A���g�̍���
	MoveWindow(hWnd, rc.left, rc.top + a -1, wndW, wndH, TRUE);
}


static void
update_window(
	HWND			hWnd)
{
	HDC 			hDC;
	HFONT			hFont;
	HFONT			hFontOld;
	PAINTSTRUCT 	ps;
	Uint32			x;
	Uint32			y;

	/* �`�揀�� */
	hDC = BeginPaint(hWnd, &ps);
	SetBkMode(hDC, OPAQUE);	//�����̔w�i��h��Ԃ�
	SetBkColor(hDC, RGB(64,128,64));
	SetTextColor(hDC, RGB(240,240,240));
	hFont = (HFONT)GetStockObject(OEM_FIXED_FONT);
	hFontOld = (HFONT)SelectObject(hDC, hFont);

	x = _FontWidth*2 +1;
	y = _FontHeight;

	TextOut(hDC, x, y, "In English. A little firm talk before it starts.", 48);	y += _FontHeight;
	TextOut(hDC, x, y, "Using ROM-image of the software not owned in oneself becomes an illegal act of lacking", 86);	y += _FontHeight;
	TextOut(hDC, x, y, "propriety to the production of the software by the sweat of one's brow with love, and", 85);	y += _FontHeight;
	TextOut(hDC, x, y, "use it with the software owned without fail, please. And, this is not a meaning that", 84);		y += _FontHeight;
	TextOut(hDC, x, y, "recommends ROM-image of the software owned in oneself to be downloaded illegal.", 79);			y += _FontHeight;
	y += _FontHeight;
	TextOut(hDC, x, y, "It is a situation in which being not able to win the age though it is regrettable, and", 86);	y += _FontHeight;
	TextOut(hDC, x, y, "exhausting the parts longevity of a \"PC Engine\" real machine every year, too progress.", 88);	y += _FontHeight;
	TextOut(hDC, x, y, "I fortunately think that happiness is born even a little by this emulator.", 74);	y += _FontHeight;
	y += _FontHeight;
	TextOut(hDC, x, y, "In Japanese. �N���O�ɏ����������b���B", 37);	y += _FontHeight;
	TextOut(hDC, x, y, "���g�ŏ��L���Ă��Ȃ��\�t�g�̂q�n�l�C���[�W���g�p���邱�Ƃ́A���������ĐS�����߂Ă��̃\�t", 88);	y += _FontHeight;
	TextOut(hDC, x, y, "�g���𐧍삳�ꂽ���ɑ΂��Ă̗�V����������@�ȍs�ׂɂȂ�܂��̂ŁA�K�����g�ŏ��L���Ă���", 88);	y += _FontHeight;
	TextOut(hDC, x, y, "�\�t�g�ŗ��p�Ȃ����Ă��������B�܂����̂��Ƃ́A���g�ŏ��L���Ă���\�t�g�̂q�n�l�C���[�W��", 88);	y += _FontHeight;
	TextOut(hDC, x, y, "��@�Ƀ_�E�����[�h���邱�Ƃ𐄏�����Ӗ��ł͂���܂���B", 56);	y += _FontHeight;
	y += _FontHeight;
	TextOut(hDC, x, y, "�c�O�Ȃ��Ƃł����A�΂ɂ͏��Ă��A�o�b�G���W�����@�̕��i�������N�X�s������󋵂ł��c�B", 88);	y += _FontHeight;
	TextOut(hDC, x, y, "���̃G�~�����[�^�[�ɂ���āA�����ł��n�b�s�[�ȋC���������܂�Ă����ƍK���Ɏv���܂��B", 86);	y += _FontHeight;
	y += _FontHeight;

	x = _FontWidth*46 + _FontWidth/2;
	y += 11;
	TextOut(hDC, x  , y, "<- Please choose.", 17);							y += _FontHeight;
	TextOut(hDC, x-1, y, "   [Put CD-ROM ahead of pushing button]", 39);	y += _FontHeight;
	TextOut(hDC, x-1, y, "   [CD-ROM���Z�b�g��Ƀ{�^���������ĉ�����]", 43);

	/* �I������ */
	EndPaint(hWnd, &ps);
	SelectObject(hDC, hFontOld);
	DeleteObject(hFont);
	ReleaseDC(hWnd, hDC);
}


static LRESULT CALLBACK
startup_wnd_proc(
	HWND		hWnd,
	UINT		uMsg,
	WPARAM		wParam,
	LPARAM		lParam)
{
	MSG				msg;
	Sint32			screenType;
	Sint32			drawMode;
	Sint32			drawMethod;
	Sint32			vsync;
	Sint32			psgType;
	Sint32			soundBuffer;

	switch (uMsg)
	{
	case WM_CREATE:
		EnableWindow(WINMAIN_GetHwnd(), FALSE);//���C���E�B���h�E�𖳌������ă��[�_���ɁB
		_hFontB = CreateFont(  0,						// �����B0 = �f�t�H���g
		                       0,						// ���B0�Ȃ獂���ɍ�������
    		                   0,						// �p�x
        		               0,						// �x�[�X���C���Ƃ̊p�x
            		           FW_NORMAL,				// ����
                		       FALSE,					// �C�^���b�N
	                    	   FALSE,					// �A���_�[���C��
		                       FALSE,					// �ł�������
    		                   0,						// ���{�����舵���Ƃ���SHIFTJIS_CHARSET�ɂ���B
        		               0,						// �o�͐��x
            		           0,						// �N���b�s���O���x
                		       0,						// �o�͕i��
                    		   0,						// �s�b�`�ƃt�@�~���[
		                       ""						// ���̖�
							); //�p��̃f�t�H���g�t�H���g�ɐݒ�
		_FontWidth	= get_font_width(hWnd);
		_FontHeight = get_font_height(hWnd);
		set_window_size(hWnd);
		break;

	case WM_PAINT:
		update_window(hWnd);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
			case BUTTON_CDROM:
				_CDorHu = 1; //�߂�l��ݒ�
				STARTUP_Deinit();
				return 0;
			case BUTTON_CUE:
				_CDorHu = 2; //�߂�l��ݒ�
				STARTUP_Deinit();
				return 0;
			case BUTTON_HUCARD:
				_CDorHu = 3; //�߂�l��ݒ�
				STARTUP_Deinit();
				return 0;
			case BUTTON_REPLAY:
				_CDorHu = 4; //�߂�l��ݒ�
				STARTUP_Deinit();
				return 0;
			case BUTTON_OPTION:
				soundBuffer = 0; //�S���ڂ̒l���ݒ肳�ꂽ���ǂ����̔��f�ɂ��g�p�B
				OPTION_Init(_hInstance, &screenType, &drawMode, &drawMethod, &vsync, &psgType, &soundBuffer); //Option�E�B���h�E��\��
				//���b�Z�[�W���[�v
				while (soundBuffer == 0) //�_�C�A���O�����ʂ�Ԃ��܂Ń��[�v
				{ 
					GetMessage(&msg ,NULL, 0, 0);
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				//Ok(SET)�{�^����������Ă����ꍇ�A�X�N���[���ݒ��ύX����B
				if (screenType > 0)
				{
					switch (screenType)
					{
						case 1:	APP_SetStartFullScreen(FALSE); break;
						case 2:
							APP_SetMagnification(2); //x2�{�ɐݒ�
							APP_SetStartFullScreen(FALSE);
							break;
						case 3:
							APP_SetMagnification(3); //x3�{�ɐݒ�
							APP_SetStartFullScreen(FALSE);
							break;
						case 4:
							APP_SetMagnification(4); //x4�{�ɐݒ�
							APP_SetStartFullScreen(FALSE);
							break;
						case 5:	APP_SetStartFullScreen(TRUE); break;
						case 6:
							APP_SetFullScreen640(); //640x480�ɐݒ�
							APP_SetStartFullScreen(TRUE);
							break;
					}
				}
				//Ok(SET)�{�^����������Ă����ꍇ�ADrawMode�ݒ��ύX����B
				if (drawMode > 0)
				{
					switch (drawMode)
					{
						case 1: APP_SetScanLineType(1,TRUE); break;
						case 2: APP_SetScanLineType(4,TRUE); break;
						case 3: APP_SetScanLineType(2,TRUE); break;
						case 4: APP_SetScanLineType(0,TRUE); break;
					}
				}
				//Ok(SET)�{�^����������Ă����ꍇ�ADrawMethod�ݒ��ύX����B
				if (drawMethod > 0)
				{
					APP_SetDrawMethod(drawMethod);
				}
				//Ok(SET)�{�^����������Ă����ꍇ�AV-Sync�ݒ��ύX����B
				if (screenType > 0)
				{
					if (vsync == 2)
						APP_SetSyncTo60HzScreen(FALSE);
					else //==1
						APP_SetSyncTo60HzScreen(TRUE);
				}
				//Ok(SET)�{�^����������Ă����ꍇ�APSG�N�H���e�B�ݒ��ύX����B
				if (psgType > 0)
				{
					if (psgType == 3)
						APP_SetPsgQuality(1);
					else if (psgType == 2)
						APP_SetPsgQuality(2);
					else //==1
						APP_SetPsgQuality(4);
				}
				//Ok(SET)�{�^����������Ă����ꍇ�APSG�N�H���e�B�ݒ��ύX����B
				if (soundBuffer > 0)
				{
					switch (soundBuffer)
					{
						case  1: APP_SetSoundBufferSize(1024); break;
						case  2: APP_SetSoundBufferSize(1152); break;
						case  3: APP_SetSoundBufferSize(1280); break;
						case  4: APP_SetSoundBufferSize(1408); break;
						case  5: APP_SetSoundBufferSize(1536); break;
						case  7: APP_SetSoundBufferSize(1792); break;
						case  8: APP_SetSoundBufferSize(2048); break;
						case  9: APP_SetSoundBufferSize(2176); break;
						case 10: APP_SetSoundBufferSize(2304); break;
						case 11: APP_SetSoundBufferSize(2560); break;
						case 12: APP_SetSoundBufferSize(3072); break;
						default: //==6
							APP_SetSoundBufferSize(1664); break;
					}
				}
				break;
		}
		break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			STARTUP_Deinit();
			return 0;
		}
		if ((wParam == 'C')||(wParam == VK_RETURN)) //�uC�v�L�[�܂��� �uEnter�v�L�[
		{
			_CDorHu = 1; //�߂�l��ݒ�
			STARTUP_Deinit();
			return 0;
		}
		if (wParam == 'P') //�uP�v�L�[
		{
			_CDorHu = 2; //�߂�l��ݒ�
			STARTUP_Deinit();
			return 0;
		}
		if ((wParam == 'H')||(wParam == 'O')||(wParam == VK_SPACE)) //�uH�v�L�[�uO�v�L�[�܂��� �X�y�[�X�L�[
		{
			_CDorHu = 3; //�߂�l��ݒ�
			STARTUP_Deinit();
			return 0;
		}
		if (wParam == 'R') //�uR�v�L�[
		{
			_CDorHu = 4; //�߂�l��ݒ�
			STARTUP_Deinit();
			return 0;
		}
		break;

	case WM_CLOSE:
		STARTUP_Deinit();
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


static BOOL
startup_main()
{
	WNDCLASS	wc;
	HWND		hWnd;
	RECT		rc;
	Uint32		x;
	Uint32		y;
	HWND		hWndTmp;

	ZeroMemory(&wc, sizeof(wc));
	wc.style		 = 0;
	wc.lpfnWndProc	 = startup_wnd_proc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = _hInstance;
	wc.hIcon		 = LoadIcon(_hInstance, MAKEINTRESOURCE(OOTAKEICON)); //�A�C�R����ǂݍ��݁Bv2.00�X�V
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	_hMyb = CreateSolidBrush(RGB(64,128,64)); //�u���V�����
	wc.hbrBackground = _hMyb;
	wc.lpszMenuName  = "";
	wc.lpszClassName = _pCaption;

	if (RegisterClass(&wc) == 0)
		return FALSE;

	hWnd = CreateWindow(
		_pCaption,
		_pCaption,
		WS_SYSMENU | WS_CAPTION,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		NULL,
		NULL,
		_hInstance,
		NULL
	);

	if (hWnd == NULL)
		return FALSE;

	_hWnd = hWnd;

	//�{�^�����쐬
	x = _FontWidth*2 +1;
	y = _FontHeight*20 +3;
	hWndTmp = CreateWindow(
		"BUTTON", "\"&CD-ROM\" Game",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, _FontWidth*21, _FontHeight*2-4,
		_hWnd, (HMENU)BUTTON_CDROM, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�����̃t�H���g��ݒ�
	
	x += _FontWidth*21 +3;
	hWndTmp = CreateWindow(
		"BUTTON", "\"&Hu-CARD\" Game",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, _FontWidth*22, _FontHeight*2-4,
		_hWnd, (HMENU)BUTTON_HUCARD, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�����̃t�H���g��ݒ�
	
	x = _FontWidth*2 +1;
	y += _FontHeight*2 -2;
	hWndTmp = CreateWindow(
		"BUTTON", "Option",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, _FontWidth*9, _FontHeight + _FontHeight/2+2,
		_hWnd, (HMENU)BUTTON_OPTION, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�����̃t�H���g��ݒ�
/*
	x = _FontWidth*2;
	hWndTmp = CreateWindow(
		"BUTTON", "&Favorite",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, _FontWidth*11, _FontHeight + _FontHeight/2+2,
		_hWnd, (HMENU)BUTTON_FAVORITE, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�����̃t�H���g��ݒ�
*/
	x += _FontWidth*9 +3;
	hWndTmp = CreateWindow(
		"BUTTON", "&Play Installed CD",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, _FontWidth*21, _FontHeight + _FontHeight/2+2,
		_hWnd, (HMENU)BUTTON_CUE, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�����̃t�H���g��ݒ�

	x += _FontWidth*21 +3;
	hWndTmp = CreateWindow(
		"BUTTON", "&Replay",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, (_FontWidth*30 + 4 + _FontWidth*15)-x, _FontHeight + _FontHeight/2+2,
		_hWnd, (HMENU)BUTTON_REPLAY, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�����̃t�H���g��ݒ�

	ShowWindow(_hWnd, SW_SHOWNORMAL);
	UpdateWindow(_hWnd);
	GetWindowRect(_hWnd, &rc);
	SetWindowPos(_hWnd, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_FRAMECHANGED); //��O�ɕ\��
	ImmAssociateContext(_hWnd, 0); //Kitao�ǉ��BIME�𖳌��ɂ���Bv0.79

	return TRUE;
}


BOOL
STARTUP_Init(
	HINSTANCE	hInstance,
	Sint32*		CDorHu)
{
	if (_hInstance != NULL)
		STARTUP_Deinit();

	_hInstance = hInstance;

	_pCDorHu = CDorHu;
	_CDorHu = -1;//�L�����Z��

	return startup_main();
}


void
STARTUP_Deinit()
{
	if (_hInstance != NULL)
	{
		DestroyWindow(_hWnd);
		_hWnd = NULL;
		UnregisterClass(_pCaption, _hInstance);
		_hInstance = NULL;
		DeleteObject(_hFontB); //�{�^���p�t�H���g���J��
		DeleteObject(_hMyb); //�u���V���J��
		
		//���C���E�B���h�E�Ƀt�H�[�J�X��߂��O�ʂɁB
		EnableWindow(WINMAIN_GetHwnd(), TRUE);
		APP_SetForegroundWindowOotake(); //�m���ɃA�N�e�B�u�ɂ���
		
		*_pCDorHu = _CDorHu; //�߂�l��ݒ�B���̏u�ԂɃ��C���E�B���h�E�͓����o���B
	}
}


const HWND
STARTUP_GetHwnd()
{
	return _hWnd;
}
