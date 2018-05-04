#include "pch.hpp"
#include "TH155AddrDef.h"
#include "Haushaltsbuch.hpp"

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalPath.hpp"

static int (WINAPI *func_TH155AddrStartup)(int, HWND, UINT);
static int (WINAPI *func_TH155AddrCleanup)();
static DWORD_PTR (WINAPI *func_TH155AddrGetParam)(int);
static TH155STATE (WINAPI *func_TH155AddrGetState)();

static const TH155CHARNAME s_charNames[] = {
	{ _T("博丽灵梦"), _T("灵梦") },
	{ _T("雾雨魔理沙"), _T("魔理沙") },
	{ _T("云居一轮"), _T("一轮") },
	{ _T("圣白莲"), _T("白莲") },
	{ _T("物部布都"), _T("布都") },
	{ _T("丰聪耳神子"), _T("神子") },
	{ _T("河城荷取"), _T("荷取") },
	{ _T("古明地恋"), _T("恋") },
	{ _T("二岩猯藏"), _T("二岩") },
	{ _T("秦心"), _T("秦心") },
	{ _T("茨木华扇") , _T("华扇") },
	{ _T("藤原妹红"), _T("妹红") },
	{ _T("少名针妙丸"), _T("针妙丸") },
	{ _T("宇佐见堇子"), _T("堇子") },
	{ _T("铃仙·优昙华院·因幡"), _T("鈴仙") },
	{ _T("哆来咪·苏伊特"), _T("哆来咪") },
	{ _T("比那名居天子"), _T("天子") },
	{ _T("八云紫"), _T("紫") },
	{ _T("依神女苑"), _T("女苑") }
};

int TH155AddrInit(HWND callbackWnd, int callbackMsg)
{
	TCHAR TH155AddrPath[1024];
	g_settings.ReadString(_T("Path"), _T("TH155Addr"), _T("TH155Addr.dll"), TH155AddrPath, sizeof(TH155AddrPath));
	
	HMODULE TH155Addr = LoadLibrary(TH155AddrPath);
	if (TH155Addr == nullptr) {
		return 0;
	}

	*reinterpret_cast<FARPROC*>(&func_TH155AddrStartup) = GetProcAddress(TH155Addr, "TH155AddrStartup");
	*reinterpret_cast<FARPROC*>(&func_TH155AddrCleanup) = GetProcAddress(TH155Addr, "TH155AddrCleanup");
	*reinterpret_cast<FARPROC*>(&func_TH155AddrGetParam) = GetProcAddress(TH155Addr, "TH155AddrGetParam");
	*reinterpret_cast<FARPROC*>(&func_TH155AddrGetState) = GetProcAddress(TH155Addr, "TH155AddrGetState");
	if (func_TH155AddrStartup == nullptr || func_TH155AddrCleanup == nullptr || func_TH155AddrGetParam == nullptr || func_TH155AddrGetState == nullptr) {
		return 0;
	}

	return func_TH155AddrStartup(1, callbackWnd, callbackMsg) != 0;
}

int TH155AddrFinish()
{
	return func_TH155AddrCleanup != nullptr ? func_TH155AddrCleanup(): 0;
}

TH155STATE TH155AddrGetState()
{
	return func_TH155AddrGetState();
}

DWORD_PTR TH155AddrGetParam(int param)
{
	return func_TH155AddrGetParam(param);
}

const TH155CHARNAME * const TH155AddrGetCharName(int index)
{
	if (TH155AddrGetCharCount() <= index) return NULL;
	return &s_charNames[index];
}

int TH155AddrGetCharCount()
{
	return _countof(s_charNames);
}
