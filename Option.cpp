/******************************************************************************
Ootake

 [Option.cpp]
	�X�^�[�g���ɐݒ��ύX���邽�߂̃t�H�[��

Copyright(C)2006-2013 Kitao Nakamura.
	�����ŁE��p�ł����J�Ȃ���Ƃ��͕K���\�[�X�R�[�h��Y�t���Ă��������B
	���̍ۂɎ���ł��܂��܂���̂ŁA�ЂƂ��Ƃ��m�点����������ƍK���ł��B
	���I�ȗ��p�͋ւ��܂��B
	���Ƃ́uGNU General Public License(��ʌ��O���p�����_��)�v�ɏ����܂��B

******************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "Option.h"
#include "resource.h"
#include "Startup.h"
#include "WinMain.h"
#include "App.h"

#define LINE_LEN	48
#define N_LINES 	21

enum OptionComponent
{
	COMBOBOX_SCREENTYPE = 1,
	COMBOBOX_DRAWMODE,
	COMBOBOX_DRAWMETHOD,
	COMBOBOX_VSYNC,
	COMBOBOX_PSGTYPE,
	COMBOBOX_SOUNDBUFFER,
	BUTTON_LIGHT,
	BUTTON_MIDDLE,
	BUTTON_DEFAULT,
	BUTTON_SET,
};
static HWND			_hWndC[10+1]; //�e�R���|�[�l���g�̃n���h��

static HBRUSH		_hMyb; //����u���V�F
static HFONT		_hFontB; //�{�^���p�t�H���g

static Uint32		_FontWidth;
static Uint32		_FontHeight;
static const char*	_pCaption = "\"Ootake\" Option";
static HINSTANCE	_hInstance = NULL;
static HWND 		_hWnd;


static Sint32		_ScreenType = -1; //�߂�l(���肵���ꍇ1�`5���Ԃ����)�B�L�����Z���Ȃ�-1�B���ݒ蒆��0�B
static Sint32*		_pScreenType;
static Sint32		_DrawMode = -1; //�߂�l(���肵���ꍇ1�`4���Ԃ����)�B�L�����Z���Ȃ�-1�B���ݒ蒆��0�B
static Sint32*		_pDrawMode;
static Sint32		_DrawMethod = -1; //�߂�l(���肵���ꍇ1�`2���Ԃ����)�B�L�����Z���Ȃ�-1�B���ݒ蒆��0�B
static Sint32*		_pDrawMethod;
static Sint32		_VSync = -1; //�߂�l(���肵���ꍇ1�`2���Ԃ����)�B�L�����Z���Ȃ�-1�B���ݒ蒆��0�B
static Sint32*		_pVSync;
static Sint32		_PSGType = -1; //�߂�l(���肵���ꍇ1�`3���Ԃ����)�B�L�����Z���Ȃ�-1�B���ݒ蒆��0�B
static Sint32*		_pPSGType;
static Sint32		_SoundBuffer = -1; //�߂�l(���肵���ꍇ1�`9���Ԃ����)�B�L�����Z���Ȃ�-1�B���ݒ蒆��0�B
static Sint32*		_pSoundBuffer;


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
	Uint32		wndW = _FontWidth  * LINE_LEN +_FontWidth/2 +2;
	Uint32		wndH = _FontHeight * N_LINES +_FontHeight/2 -2;

	SetRect(&rc, 0, 0, wndW, wndH);
	AdjustWindowRectEx(&rc, GetWindowLong(hWnd, GWL_STYLE),
						GetMenu(hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE));
	wndW = rc.right - rc.left;
	wndH = rc.bottom - rc.top;
	GetWindowRect(STARTUP_GetHwnd(), &rc);
	MoveWindow(hWnd, rc.left, rc.bottom - wndH, wndW, wndH, TRUE);
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

	x = _FontWidth*2 +2;
	y = _FontHeight -2;

	TextOut(hDC, x, y, "Video Setting  �r�f�I���̐ݒ�", 31);	y += _FontHeight*9+_FontHeight/2;
	TextOut(hDC, x, y, "Sound Setting  �T�E���h���̐ݒ�", 33);

	/* �I������ */
	EndPaint(hWnd, &ps);
	SelectObject(hDC, hFontOld);
	DeleteObject(hFont);
	ReleaseDC(hWnd, hDC);
}


static LRESULT CALLBACK
option_wnd_proc(
	HWND		hWnd,
	UINT		uMsg,
	WPARAM		wParam,
	LPARAM		lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		EnableWindow(STARTUP_GetHwnd(), FALSE);//�X�^�[�g�A�b�v�E�B���h�E�𖳌������ă��[�_���ɁB
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

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			OPTION_Deinit();
			return 0;
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
			case COMBOBOX_SCREENTYPE:
			case COMBOBOX_DRAWMODE:
			case COMBOBOX_DRAWMETHOD:
			case COMBOBOX_VSYNC:
			case COMBOBOX_PSGTYPE:
			case COMBOBOX_SOUNDBUFFER:
				if (HIWORD(wParam) == CBN_CLOSEUP)
					SetFocus(GetDlgItem(hWnd, BUTTON_SET));//OK�{�^���Ƀt�H�[�J�X
				break;

			case BUTTON_LIGHT:
				SendMessage(_hWndC[COMBOBOX_SCREENTYPE], CB_SETCURSEL, 5, 0);
				SendMessage(_hWndC[COMBOBOX_DRAWMODE], CB_SETCURSEL, 1, 0);
				SendMessage(_hWndC[COMBOBOX_DRAWMETHOD], CB_SETCURSEL, 0, 0);
				SendMessage(_hWndC[COMBOBOX_VSYNC], CB_SETCURSEL, 1, 0);
				SendMessage(_hWndC[COMBOBOX_PSGTYPE], CB_SETCURSEL, 2, 0);
				SendMessage(_hWndC[COMBOBOX_SOUNDBUFFER], CB_SETCURSEL, 7, 0);
				break;

			case BUTTON_MIDDLE:
				SendMessage(_hWndC[COMBOBOX_SCREENTYPE], CB_SETCURSEL, 1, 0);
				SendMessage(_hWndC[COMBOBOX_DRAWMODE], CB_SETCURSEL, 1, 0);
				SendMessage(_hWndC[COMBOBOX_DRAWMETHOD], CB_SETCURSEL, 0, 0);
				SendMessage(_hWndC[COMBOBOX_VSYNC], CB_SETCURSEL, 0, 0);
				SendMessage(_hWndC[COMBOBOX_PSGTYPE], CB_SETCURSEL, 0, 0);
				SendMessage(_hWndC[COMBOBOX_SOUNDBUFFER], CB_SETCURSEL, 5, 0);
				break;

			case BUTTON_DEFAULT:
				SendMessage(_hWndC[COMBOBOX_SCREENTYPE], CB_SETCURSEL, 2, 0);
				SendMessage(_hWndC[COMBOBOX_DRAWMODE], CB_SETCURSEL, 0, 0);
				SendMessage(_hWndC[COMBOBOX_DRAWMETHOD], CB_SETCURSEL, 0, 0);
				SendMessage(_hWndC[COMBOBOX_VSYNC], CB_SETCURSEL, 0, 0);
				SendMessage(_hWndC[COMBOBOX_PSGTYPE], CB_SETCURSEL, 0, 0);
				SendMessage(_hWndC[COMBOBOX_SOUNDBUFFER], CB_SETCURSEL, 5, 0);
				break;

			case BUTTON_SET:
				//�߂�l��ݒ�
				_ScreenType = SendMessage(GetDlgItem(hWnd, COMBOBOX_SCREENTYPE), CB_GETCURSEL, 0, 0) + 1;
				_DrawMode = SendMessage(GetDlgItem(hWnd, COMBOBOX_DRAWMODE), CB_GETCURSEL, 0, 0) + 1;
				_DrawMethod = SendMessage(GetDlgItem(hWnd, COMBOBOX_DRAWMETHOD), CB_GETCURSEL, 0, 0) + 1;
				_VSync = SendMessage(GetDlgItem(hWnd, COMBOBOX_VSYNC), CB_GETCURSEL, 0, 0) + 1;
				_PSGType = SendMessage(GetDlgItem(hWnd, COMBOBOX_PSGTYPE), CB_GETCURSEL, 0, 0) + 1;
				_SoundBuffer = SendMessage(GetDlgItem(hWnd, COMBOBOX_SOUNDBUFFER), CB_GETCURSEL, 0, 0) + 1;
				OPTION_Deinit();
				return 0;
		}
		break;

	case WM_CLOSE:
		OPTION_Deinit();
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


static BOOL
option_main()
{
	WNDCLASS	wc;
	HWND		hWnd;
	RECT		rc;
	Uint32		x;
	Uint32		y;
	HWND		hWndTmp;
	
	ZeroMemory(&wc, sizeof(wc));
	wc.style		 = 0;
	wc.lpfnWndProc	 = option_wnd_proc;
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

	//�t���X�N���[��or�E�B���h�E���[�h�ؑ֗p�R���{�{�b�N�X���쐬
	x = _FontWidth*2;
	y = _FontHeight*2;
	_hWndC[1] = CreateWindow(
		"COMBOBOX", "",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
		x, y, _FontWidth*42, _FontHeight*10,
		_hWnd, (HMENU)COMBOBOX_SCREENTYPE, _hInstance, NULL
	);
	SendMessage(_hWndC[1], WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�
	SendMessage(_hWndC[1], CB_ADDSTRING, 0, (LPARAM)"Start Window mode");
	SendMessage(_hWndC[1], CB_ADDSTRING, 0, (LPARAM)"Start Window (x2)");
	SendMessage(_hWndC[1], CB_ADDSTRING, 0, (LPARAM)"Start Window (x3)");
	SendMessage(_hWndC[1], CB_ADDSTRING, 0, (LPARAM)"Start Window (x4)");
	SendMessage(_hWndC[1], CB_ADDSTRING, 0, (LPARAM)"Start FullScreen mode");
	SendMessage(_hWndC[1], CB_ADDSTRING, 0, (LPARAM)"Start FullScreen (640x480)");
	if (APP_GetStartFullScreen())
		SendMessage(_hWndC[1], CB_SETCURSEL, 4, 0);
	else
		SendMessage(_hWndC[1], CB_SETCURSEL, 0, 0);

	//TV or RGB�ؑ֗p�R���{�{�b�N�X���쐬
	x = _FontWidth*2;
	y = _FontHeight*4;
	_hWndC[2] = CreateWindow(
		"COMBOBOX", "",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
		x, y, _FontWidth*42, _FontHeight*8,
		_hWnd, (HMENU)COMBOBOX_DRAWMODE, _hInstance, NULL
	);
	SendMessage(_hWndC[2], WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�
	SendMessage(_hWndC[2], CB_ADDSTRING, 0, (LPARAM)"Special Scanlined (Sharp&Gentle)");
	SendMessage(_hWndC[2], CB_ADDSTRING, 0, (LPARAM)"TV Mode (Fast)");
	SendMessage(_hWndC[2], CB_ADDSTRING, 0, (LPARAM)"Horizontal Scanlined");
	SendMessage(_hWndC[2], CB_ADDSTRING, 0, (LPARAM)"Non-Scanlined (Fast)");
	switch (APP_GetScanLineType())
	{
		case 0: SendMessage(_hWndC[2], CB_SETCURSEL, 3, 0); break;
		case 1: SendMessage(_hWndC[2], CB_SETCURSEL, 0, 0); break;
		case 2: SendMessage(_hWndC[2], CB_SETCURSEL, 2, 0); break;
		//case 3: �c�X�L�������C���B���ݖ������B
		case 4: SendMessage(_hWndC[2], CB_SETCURSEL, 1, 0); break;
	}

	//Direct3D or DirectDraw�ؑ֗p�R���{�{�b�N�X���쐬
	x = _FontWidth*2;
	y = _FontHeight*6;
	_hWndC[3] = CreateWindow(
		"COMBOBOX", "",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
		x, y, _FontWidth*42, _FontHeight*8,
		_hWnd, (HMENU)COMBOBOX_DRAWMETHOD, _hInstance, NULL
	);
	SendMessage(_hWndC[3], WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�
	SendMessage(_hWndC[3], CB_ADDSTRING, 0, (LPARAM)"Use Direct3D (Default)");
	SendMessage(_hWndC[3], CB_ADDSTRING, 0, (LPARAM)"Use DirectDraw (Old)");
	if (APP_GetDrawMethod() == 2)
		SendMessage(_hWndC[3], CB_SETCURSEL, 1, 0);
	else //1
		SendMessage(_hWndC[3], CB_SETCURSEL, 0, 0);

	//V-Sync�ؑ֗p�R���{�{�b�N�X���쐬
	x = _FontWidth*2;
	y = _FontHeight*8;
	_hWndC[4] = CreateWindow(
		"COMBOBOX", "",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
		x, y, _FontWidth*42, _FontHeight*8,
		_hWnd, (HMENU)COMBOBOX_VSYNC, _hInstance, NULL
	);
	SendMessage(_hWndC[4], WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�
	SendMessage(_hWndC[4], CB_ADDSTRING, 0, (LPARAM)"V-Sync 59-61Hz (Beauty,Default)");
	SendMessage(_hWndC[4], CB_ADDSTRING, 0, (LPARAM)"Non-Use V-Sync (Fast,Flicker)");
	if (APP_GetSyncTo60HzScreen())
		SendMessage(_hWndC[4], CB_SETCURSEL, 0, 0);
	else
		SendMessage(_hWndC[4], CB_SETCURSEL, 1, 0);

	//PSG�T�E���h�^�C�v�p�R���{�{�b�N�X���쐬
	x = _FontWidth*2;
	y = _FontHeight*11+_FontHeight/2;
	_hWndC[5] = CreateWindow(
		"COMBOBOX", "",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
		x, y, _FontWidth*42, _FontHeight*8,
		_hWnd, (HMENU)COMBOBOX_PSGTYPE, _hInstance, NULL
	);
	SendMessage(_hWndC[5], WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�
	SendMessage(_hWndC[5], CB_ADDSTRING, 0, (LPARAM)"High Quality PSG (Default)");
	SendMessage(_hWndC[5], CB_ADDSTRING, 0, (LPARAM)"A Little Light PSG");
	SendMessage(_hWndC[5], CB_ADDSTRING, 0, (LPARAM)"Light PSG (Fast)");
	switch (APP_GetPsgQuality())
	{
		case 1: SendMessage(_hWndC[5], CB_SETCURSEL, 2, 0); break;
		case 2: SendMessage(_hWndC[5], CB_SETCURSEL, 1, 0); break;
		default: //4
			SendMessage(_hWndC[5], CB_SETCURSEL, 0, 0); break;
	}

	//�T�E���h�o�b�t�@�p�R���{�{�b�N�X���쐬
	x = _FontWidth*2;
	y = _FontHeight*13+_FontHeight/2;
	_hWndC[6] = CreateWindow(
		"COMBOBOX", "",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
		x, y, _FontWidth*42, _FontHeight*16,
		_hWnd, (HMENU)COMBOBOX_SOUNDBUFFER, _hInstance, NULL
	);
	SendMessage(_hWndC[6], WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 1024 (Not Delay)");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 1152");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 1280");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 1408");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 1536");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 1664 (Default)");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 1792");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 2048");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 2176");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 2304");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 2560");
	SendMessage(_hWndC[6], CB_ADDSTRING, 0, (LPARAM)"Buffer 3072 (Beauty,Delay)");
	switch (APP_GetSoundBufferSize())
	{
		case 1024: SendMessage(_hWndC[6], CB_SETCURSEL,  0, 0); break;
		case 1152: SendMessage(_hWndC[6], CB_SETCURSEL,  1, 0); break;
		case 1280: SendMessage(_hWndC[6], CB_SETCURSEL,  2, 0); break;
		case 1408: SendMessage(_hWndC[6], CB_SETCURSEL,  3, 0); break;
		case 1536: SendMessage(_hWndC[6], CB_SETCURSEL,  4, 0); break;
		case 1792: SendMessage(_hWndC[6], CB_SETCURSEL,  6, 0); break;
		case 2048: SendMessage(_hWndC[6], CB_SETCURSEL,  7, 0); break;
		case 2176: SendMessage(_hWndC[6], CB_SETCURSEL,  8, 0); break;
		case 2304: SendMessage(_hWndC[6], CB_SETCURSEL,  9, 0); break;
		case 2560: SendMessage(_hWndC[6], CB_SETCURSEL, 10, 0); break;
		case 3072: SendMessage(_hWndC[6], CB_SETCURSEL, 11, 0); break;
		default: //1664
			SendMessage(_hWndC[6], CB_SETCURSEL, 5, 0); break;
	}

	//Light�{�^�����쐬
	x = _FontWidth*2;
	y = _FontHeight*16;
	hWndTmp = CreateWindow(
		"BUTTON", "Light(Fast)",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, _FontWidth*13, _FontHeight*2-6,
		_hWnd, (HMENU)BUTTON_LIGHT, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�

	//MiddleLight�{�^�����쐬
	x = _FontWidth*16;
	y = _FontHeight*16;
	hWndTmp = CreateWindow(
		"BUTTON", "MiddleLight",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, _FontWidth*14, _FontHeight*2-6,
		_hWnd, (HMENU)BUTTON_MIDDLE, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�

	//Default�{�^�����쐬
	x = _FontWidth*31;
	y = _FontHeight*16;
	hWndTmp = CreateWindow(
		"BUTTON", "Default(HQ)",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, _FontWidth*14, _FontHeight*2-6,
		_hWnd, (HMENU)BUTTON_DEFAULT, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�

	//SET�{�^�����쐬
	x = _FontWidth*36;
	y = _FontHeight*19;
	hWndTmp = CreateWindow(
		"BUTTON", "Set",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		x, y, _FontWidth*10, _FontHeight*2-6,
		_hWnd, (HMENU)BUTTON_SET, _hInstance, NULL
	);
	SendMessage(hWndTmp, WM_SETFONT, (WPARAM)_hFontB, MAKELPARAM(TRUE, 0));//�t�H���g��ݒ�

	ShowWindow(_hWnd, SW_SHOWNORMAL);
	UpdateWindow(_hWnd);
	GetWindowRect(_hWnd, &rc);
	SetWindowPos(_hWnd, HWND_TOP, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_FRAMECHANGED);
	ImmAssociateContext(_hWnd, 0); //IME�𖳌��ɂ���

	return TRUE;
}


BOOL
OPTION_Init(
	HINSTANCE	hInstance,
	Sint32*		screenType,
	Sint32*		drawMode,
	Sint32*		drawMethod,
	Sint32*		vsync,
	Sint32*		psgType,
	Sint32*		soundBuffer)
{
	if (_hInstance != NULL)
		OPTION_Deinit();

	_hInstance = hInstance;

	_pScreenType = screenType;
	_ScreenType = -1;//�L�����Z��
	_pDrawMode = drawMode;
	_DrawMode = -1;//�L�����Z��
	_pDrawMethod = drawMethod;
	_DrawMethod = -1;//�L�����Z��
	_pVSync = vsync;
	_VSync = -1;//�L�����Z��
	_pPSGType = psgType;
	_PSGType = -1;//�L�����Z��
	_pSoundBuffer = soundBuffer;
	_SoundBuffer = -1;//�L�����Z��

	return option_main();
}


void
OPTION_Deinit()
{
	if (_hInstance != NULL)
	{
		DestroyWindow(_hWnd);
		_hWnd = NULL;
		UnregisterClass(_pCaption, _hInstance);
		_hInstance = NULL;
		
		DeleteObject(_hFontB); //�{�^���p�t�H���g���J��
		DeleteObject(_hMyb); //�u���V���J��
		
		//�X�^�[�g�A�b�v�E�B���h�E�Ƀt�H�[�J�X��߂��O�ʂɁB
		EnableWindow(STARTUP_GetHwnd(), TRUE);
		SetForegroundWindow(STARTUP_GetHwnd());
		
		//�߂�l��ݒ�
		*_pScreenType = _ScreenType;
		*_pDrawMode = _DrawMode;
		*_pDrawMethod = _DrawMethod;
		*_pVSync = _VSync;
		*_pPSGType = _PSGType;
		*_pSoundBuffer = _SoundBuffer; //���̏u�Ԃ�Startup.c �ɐ��䂪�߂�B
	}
}

