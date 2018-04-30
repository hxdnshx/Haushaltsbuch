#include <Windows.h>
#include <shlwapi.h>
#include <ImageHlp.h>
#include <tchar.h>
#include <aclapi.h>
#include <lmcons.h>
#include <Winternl.h>
#include "TH155AddrDef.hpp"
#include "stdio.h"

#define MINIMAL_USE_PROCESSHEAPSTRING
#define MINIMAL_USE_PROCESSHEAPARRAY
#include "MinimalPath.hpp"
#include "MinimalIniFile.hpp"
#include "MinimalArray.hpp"

#define TH155_APIVERSION 1

#define POLL_INTERVAL 50

#define Default_CoreBase        0x004dace4
#define Default_WindowClass     _T("th155")
#define Default_WindowCaption   _T("搶曽溸埶壺丂乣 Antinomy of Common Flowers. Ver")

#define SQSTRING_LIMIT 256

#undef OutputDebugString
#define OutputDebugString(x)
#define OutputDebugStringA(x)


#define TH155AddrIPCWndClass      _T("TH155AddrIPCWndClass")
#define TH155AddrInstallMutex     _T("TH155AddrInstallMutex")
#define TH155AddrInstalledEvent   _T("TH155AddrInstalledEvent")
#define TH155AddrIPCMutex         _T("TH155AddrIPCMutex")

#define WM_FINDRTCHILD (WM_APP + 0)
#define WM_RTCHILDTOSTRING (WM_APP + 1)

#pragma data_seg(".TH155HK")
HHOOK hookHandle = nullptr;
union {
	struct {
		DWORD len;
		char val[SQSTRING_LIMIT];
	} str;
	struct {
		UINT type;
		UINT val;
	} var;
} ipcData = {};
#pragma data_seg()

typedef struct {
	LPCTSTR windowClassName;
	LPCTSTR windowCaptionFormerPart;
	HWND hwndResult;
} MYFINDWINDOWPARAMS;

static const LPCSTR characterRomanNames[] = {
	"reimu",
	"marisa",
	"ichirin",
	"hijiri",
	"futo",
	"miko",
	"nitori",
	"koishi",
	"mamizou",
	"kokoro",
	"kasen",
	"mokou",
	"sinmyoumaru",
	"usami",
	"udonge",
	"doremy",
	"tenshi",
	"yukari",
	"jyoon"
};

static bool isClientInitialized = false;
static HANDLE clientWorkerThread = nullptr;
static DWORD clientWorkerThreadId = 0;
static HWND clientCallbackWnd = nullptr;
static UINT clientCallbackMsg = 0;

static bool isIPCWindowCreated = false;
static HMODULE libModuleSaved = nullptr;
static HWND ipcWindow = nullptr;

static TCHAR TH155WindowClass[256];
static TCHAR TH155WindowCaption[256];
static DWORD TH155WindowCaptionCount;
static DWORD TH155CoreBase = 0;

static int paramOld[TH155PARAM_MAX];
static char pnameBuff[SQSTRING_LIMIT];

static TH155STATE TH155State;

UINT WINAPI DllMain(HMODULE libModule, DWORD reason, LPVOID reserved)
{
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		libModuleSaved = libModule;
		DisableThreadLibraryCalls(libModule);
	default:
		break;
	}
	return TRUE;
}

#define LoadAddress(V,N) { TCHAR addrStr[16]; \
	profile.ReadString(_T("Address"), _T(#N), _T(""), addrStr, sizeof addrStr); \
	if (!::StrToIntEx(addrStr, STIF_SUPPORT_HEX, reinterpret_cast<int *>(&##V))) ##V = Default_##N;  }
#define LoadChar(V,N) profile.ReadString(_T("TH155"), _T(#N), Default_##N, ##V, sizeof(##V))
static bool TH155LoadProfileForClient()
{
	Minimal::ProcessHeapPath profPath;
	if (!TryGetModulePath(libModuleSaved, profPath)) return false;
	MinimalIniFile profile(profPath /= _T("TH155Addr.ini"));
	LoadChar(TH155WindowClass, WindowClass);
	TH155WindowCaptionCount = LoadChar(TH155WindowCaption, WindowCaption);
	return true;
}

static bool TH155LoadProfileForVictim()
{
	Minimal::ProcessHeapPath profPath;
	if (!TryGetModulePath(libModuleSaved, profPath)) return false;
	MinimalIniFile profile(profPath /= _T("TH155Addr.ini"));
	LoadAddress(TH155CoreBase, CoreBase);
	return true;
}

// strtok 偺傛偆側 iterator class
class StringSplitter {
private:
	Minimal::ProcessHeapStringA buff;
	LPCSTR begin;
	LPCSTR end;
	char sep;

public:
	StringSplitter(LPCSTR src, char separator) :
		begin(src), end(src), sep(separator) {}

public:
	LPCSTR Next() {
		if (begin == NULL) return NULL;
		end = ::StrChrA(begin, sep);
		if (end == NULL) {
			buff = begin;
			begin = end;
		}
		else {
			buff = Minimal::ProcessHeapStringA(begin, end).GetRaw();
			begin = end + 1;
		}
		return buff.GetRaw();
	}
};

static bool EnumRTChildProc(LPCSTR path, int level, FILE* fp = nullptr)
{
	DWORD_PTR items;
	DWORD_PTR itemVal;
	DWORD_PTR titemVal;
	DWORD_PTR itemVal2;
	char  itemStr[SQSTRING_LIMIT];
	DWORD itemType;
	DWORD titemType;
	DWORD itemNum;
	DWORD itemLen;
	DWORD itemIndex;
	DWORD readSize;
	BOOL ret;
	DWORD j;
	bool begin = false;



	//if (GetAsyncKeyState(VK_F1) < 0) __asm mov ds:[0], 0xdeadbeef

	HANDLE curProc = GetCurrentProcess();
	// CoreBase から table を読み込む
	ret = ::ReadProcessMemory(curProc, (LPVOID)((DWORD_PTR)GetModuleHandle(nullptr) + TH155CoreBase), &itemVal, sizeof itemVal, &readSize);
	if (!ret) return false;
	ret = ::ReadProcessMemory(curProc, (LPVOID)((DWORD_PTR)itemVal + 0x34), &itemVal, sizeof itemVal, &readSize);
	if (!ret) return false;

	itemType = 0x20;

	StringSplitter tokenizer(path, '/');
	LPCSTR pathToken;
	int curlevel = -1;
	//fprintf_s(fp, "Enum Proc %s\n",path);
	while (pathToken = tokenizer.Next()) {
		curlevel++;
		switch (itemType & 0xFFFFF) {
		case 0x20:	// TABLE
					// Table の情報を読み込む
			//fprintf_s(fp, "Enum Table%s\n",pathToken);
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x20), &items, sizeof items, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x24), &itemNum, sizeof itemNum, &readSize);
			if (!ret) return false;
			// Table items を探索
			for (j = 0; j < itemNum; ++j) {
				// Table item key を読み込む
				ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x08), &itemType, sizeof itemType, &readSize);
				if (!ret) return false;
				ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x0c), &itemVal, sizeof itemVal, &readSize);
				if (!ret) return false;
				
				//  Table item key が文字列のときだけ受け付ける
				if ((itemType & 0xFFFFF) == 0x10) {
					//  Table item key を文字列として読み込む
					ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x14), &itemLen, sizeof itemLen, &readSize);
					if (!ret) return false;
					// 常識的な長さの文字列だけ受け付ける
					if (0 < itemLen && itemLen < SQSTRING_LIMIT) {
						ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x1C), itemStr, itemLen, &readSize);
						if (!ret) return false;
						ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x00), &titemType, sizeof itemType, &readSize);
						if (!ret) return false;
						ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x04), &titemVal, sizeof itemVal, &readSize);
						if (!ret) return false;
						itemStr[itemLen] = 0;
						// Table item key の値が path のトークンと一致したら table item value を読み込む
						if (level == curlevel)
						{
							//int bsize = strlen(path) + itemLen + 30;
							{
								//char* newpath = new char[1500];
								fprintf_s(fp, "%s %s 0x%x %x %x \n", path, itemStr, (DWORD_PTR)(items + j * 0x14 + 0x04), (titemType & 0xFFFFF), titemVal);
							}
						}
						if (((titemType & 0xFFFFF) == 0x20 || (titemType & 0xFFFFF) == 0x8000) && level == curlevel)//Table 则继续向下遍历
						{
							Minimal::ProcessHeapArrayT<char> newpath(strlen(path) + itemLen + 2);
							strcpy_s(newpath.GetRaw(), strlen(path) + itemLen + 2, path);
							strcat_s(newpath.GetRaw(), strlen(path) + itemLen + 2, itemStr);
							strcat_s(newpath.GetRaw(), strlen(path) + itemLen + 2, "/");
							//fprintf_s(fp, "Enum Next %s\n", newpath.GetRaw());
							EnumRTChildProc(newpath.GetRaw(), level + 1, fp);
						}
						if (::lstrcmpA(itemStr, pathToken) == 0) {
							ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x00), &itemType, sizeof itemType, &readSize);
							if (!ret) return false;
							ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x04), &itemVal, sizeof itemVal, &readSize);
							if (!ret) return false;
							break;
						}
					}
				}
			}
			if (j == itemNum) return false;
			break;
		case 0x40:	// ARRAY
					// Array item の先頭アドレスと item 数を読み込む
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x18), &items, sizeof items, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x1C), &itemNum, sizeof itemNum, &readSize);
			if (!ret) return false;
			// Path のトークンを Array item の index と見なして範囲チェック
			itemIndex = StrToIntA(pathToken);
			if (itemNum <= itemIndex) return false;
			// Array item を読み込む
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + itemIndex * 0x08 + 0x00), &itemType, sizeof itemType, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + itemIndex * 0x08 + 0x04), &itemVal, sizeof itemVal, &readSize);
			if (!ret) return false;
			break;
		case 0x8000:	// INSTANCE
						// Instance members の情報を読み込む
			//fprintf_s(fp, "Enum Instance\n");
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x1C), &items, sizeof items, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + 0x18), &items, sizeof items, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + 0x24), &itemNum, sizeof itemNum, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + 0x20), &items, sizeof items, &readSize);
			if (!ret) return false;
			// Instance members を探索
			for (j = 0; j < itemNum; ++j) {
				// Instance member metadata value を読み込む
				ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x00), &itemType, sizeof itemType, &readSize);
				if (!ret) return false;
				ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x04), &itemVal2, sizeof itemVal, &readSize);
				if (!ret) return false;
				// Instance member metadata value が有効なときだけ受け付ける
				if ((itemType & 0xFFFFF) == 0x02) {
					// Instance member metadata value から instance member type と instance member index を抽出
					DWORD memberType = itemVal2 & 0xFF000000;
					DWORD memberIndex = itemVal2 & 0x00FFFFFF;
					// Instance member type が有効なときだけ受け付ける
					if (memberType == 0x02000000) {
						// Instance member metadata key を読み込む
						ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x08), &itemType, sizeof itemType, &readSize);
						if (!ret) return false;
						ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x0c), &itemVal2, sizeof itemVal, &readSize);
						if (!ret) return false;
						// Instance member metadata key が文字列のときだけ受け付ける
						if ((itemType & 0xFFFFF) == 0x10) {
							//  Instance member metadata key を文字列として読み込む
							ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal2 + 0x14), &itemLen, sizeof itemLen, &readSize);
							if (!ret) return false;
							// 常識的な長さの文字列だけ受け付ける
							if (0 < itemLen && itemLen < SQSTRING_LIMIT) {
								ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal2 + 0x1C), itemStr, itemLen, &readSize);
								if (!ret) return false;
								ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x2C + memberIndex * 0x08 + 0x00), &titemType, sizeof itemType, &readSize);
								if (!ret) return false;
								ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x2C + memberIndex * 0x08 + 0x04), &titemVal, sizeof itemVal, &readSize);
								if (level == curlevel)
								{
									//int bsize = strlen(path) + itemLen + 30;
									{
										//char* newpath = new char[1500];
										fprintf_s(fp, "%s %s %x %x \n", path, itemStr, (titemType & 0xFFFFF), titemVal);
									}
								}
								if (((titemType & 0xFFFFF) == 0x20 || (titemType & 0xFFFFF) == 0x8000) && level == curlevel)//Table 则继续向下遍历
								{
									Minimal::ProcessHeapArrayT<char> newpath(strlen(path) + itemLen + 100);
									strcpy_s(newpath.GetRaw(), strlen(path) + itemLen + 100, path);
									if (level != 0)
									{

									}
									strcat_s(newpath.GetRaw(), strlen(path) + itemLen + 100, itemStr);
									strcat_s(newpath.GetRaw(), strlen(path) + itemLen + 100, "/");
									//fprintf_s(fp, "Enum Next %s\n", newpath.GetRaw());
									EnumRTChildProc(newpath.GetRaw(), level + 1, fp);
								}
								itemStr[itemLen] = 0;
								// Instance member metadata key の値が path のトークンと一致したら instance member value を読み込む
								if (::lstrcmpA(itemStr, pathToken) == 0) {
									ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x2C + memberIndex * 0x08 + 0x00), &itemType, sizeof itemType, &readSize);
									if (!ret) return false;
									ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x2C + memberIndex * 0x08 + 0x04), &itemVal, sizeof itemVal, &readSize);
									if (!ret) return false;
									break;
								}
							}
						}
					}
				}
			}
			if (j == itemNum) return false;
			break;
		}
	}
	ipcData.var.type = itemType;
	ipcData.var.val = itemVal;
	return true;
}
// CoreBase class instance 偵偁傞 SQVM instance 偺 Root Table 偐傜 巜掕偝傟偨僷僗偺 item value 傪 fetch
static bool FindRTChildProc()
{
	DWORD_PTR items;
	DWORD_PTR itemVal;
	DWORD_PTR itemVal2;
	char  itemStr[SQSTRING_LIMIT];
	DWORD itemType;
	DWORD itemNum;
	DWORD itemLen;
	DWORD itemIndex;
	DWORD readSize;
	BOOL ret;
	DWORD j;

	//if (GetAsyncKeyState(VK_F1) < 0) __asm mov ds:[0], 0xdeadbeef

	HANDLE curProc = GetCurrentProcess();
	// CoreBase 偐傜 table 傪撉傒崬傓
	ret = ::ReadProcessMemory(curProc, (LPVOID)((DWORD_PTR)GetModuleHandle(nullptr) + TH155CoreBase), &itemVal, sizeof itemVal, &readSize);
	if (!ret) return false;
	ret = ::ReadProcessMemory(curProc, (LPVOID)((DWORD_PTR)itemVal + 0x34), &itemVal, sizeof itemVal, &readSize);
	if (!ret) return false;

	itemType = 0x20;

	StringSplitter tokenizer(ipcData.str.val, '/');
	LPCSTR pathToken;
	while (pathToken = tokenizer.Next()) {
		switch (itemType & 0xFFFFF) {
		case 0x20:	// TABLE
			// Table 偺忣曬傪撉傒崬傓
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x20), &items, sizeof items, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x24), &itemNum, sizeof itemNum, &readSize);
			if (!ret) return false;
			// Table items 傪扵嶕
			for (j = 0; j < itemNum; ++j) {
				// Table item key 傪撉傒崬傓
				ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x08), &itemType, sizeof itemType, &readSize);
				if (!ret) return false;
				ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x0c), &itemVal, sizeof itemVal, &readSize);
				if (!ret) return false;
				//  Table item key 偑暥帤楍偺偲偒偩偗庴偗晅偗傞
				if ((itemType & 0xFFFFF) == 0x10) {
					//  Table item key 傪暥帤楍偲偟偰撉傒崬傓
					ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x14), &itemLen, sizeof itemLen, &readSize);
					if (!ret) return false;
					// 忢幆揑側挿偝偺暥帤楍偩偗庴偗晅偗傞
					if (0 < itemLen && itemLen < SQSTRING_LIMIT) {
						ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x1C), itemStr, itemLen, &readSize);
						if (!ret) return false;
						itemStr[itemLen] = 0;
						// Table item key 偺抣偑 path 偺僩乕僋儞偲堦抳偟偨傜 table item value 傪撉傒崬傓
						if (::lstrcmpA(itemStr, pathToken) == 0) {
							ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x00), &itemType, sizeof itemType, &readSize);
							if (!ret) return false;
							ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x04), &itemVal, sizeof itemVal, &readSize);
							if (!ret) return false;
							break;
						}
					}
				}
			}
			if (j == itemNum) return false;
			break;
		case 0x40:	// ARRAY
			// Array item 偺愭摢傾僪儗僗偲 item 悢傪撉傒崬傓
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x18), &items, sizeof items, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x1C), &itemNum, sizeof itemNum, &readSize);
			if (!ret) return false;
			// Path 偺僩乕僋儞傪 Array item 偺 index 偲尒側偟偰斖埻僠僃僢僋
			itemIndex = StrToIntA(pathToken);
			if (itemNum <= itemIndex) return false;
			// Array item 傪撉傒崬傓
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + itemIndex * 0x08 + 0x00), &itemType, sizeof itemType, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + itemIndex * 0x08 + 0x04), &itemVal, sizeof itemVal, &readSize);
			if (!ret) return false;
			break;
		case 0x8000:	// INSTANCE
			// Instance members 偺忣曬傪撉傒崬傓
			ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x1C), &items, sizeof items, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + 0x18), &items, sizeof items, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + 0x24), &itemNum, sizeof itemNum, &readSize);
			if (!ret) return false;
			ret = ::ReadProcessMemory(curProc, (LPVOID)(items + 0x20), &items, sizeof items, &readSize);
			if (!ret) return false;
			// Instance members 傪扵嶕
			for (j = 0; j < itemNum; ++j) {
				// Instance member metadata value 傪撉傒崬傓
				ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x00), &itemType, sizeof itemType, &readSize);
				if (!ret) return false;
				ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x04), &itemVal2, sizeof itemVal, &readSize);
				if (!ret) return false;
				// Instance member metadata value 偑桳岠側偲偒偩偗庴偗晅偗傞
				if ((itemType & 0xFFFFF) == 0x02) {
					// Instance member metadata value 偐傜 instance member type 偲 instance member index 傪拪弌
					DWORD memberType = itemVal2 & 0xFF000000;
					DWORD memberIndex = itemVal2 & 0x00FFFFFF;
					// Instance member type 偑桳岠側偲偒偩偗庴偗晅偗傞
					if (memberType == 0x02000000) {
						// Instance member metadata key 傪撉傒崬傓
						ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x08), &itemType, sizeof itemType, &readSize);
						if (!ret) return false;
						ret = ::ReadProcessMemory(curProc, (LPVOID)(items + j * 0x14 + 0x0c), &itemVal2, sizeof itemVal, &readSize);
						if (!ret) return false;
						// Instance member metadata key 偑暥帤楍偺偲偒偩偗庴偗晅偗傞
						if ((itemType & 0xFFFFF) == 0x10) {
							//  Instance member metadata key 傪暥帤楍偲偟偰撉傒崬傓
							ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal2 + 0x14), &itemLen, sizeof itemLen, &readSize);
							if (!ret) return false;
							// 忢幆揑側挿偝偺暥帤楍偩偗庴偗晅偗傞
							if (0 < itemLen && itemLen < SQSTRING_LIMIT) {
								ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal2 + 0x1C), itemStr, itemLen, &readSize);
								if (!ret) return false;
								itemStr[itemLen] = 0;
								// Instance member metadata key 偺抣偑 path 偺僩乕僋儞偲堦抳偟偨傜 instance member value 傪撉傒崬傓
								if (::lstrcmpA(itemStr, pathToken) == 0) {
									ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x2C + memberIndex * 0x08 + 0x00), &itemType, sizeof itemType, &readSize);
									if (!ret) return false;
									ret = ::ReadProcessMemory(curProc, (LPVOID)(itemVal + 0x2C + memberIndex * 0x08 + 0x04), &itemVal, sizeof itemVal, &readSize);
									if (!ret) return false;
									break;
								}
							}
						}
					}
				}
			}
			if (j == itemNum) return false;
			break;
		}
	}
	ipcData.var.type = itemType;
	ipcData.var.val = itemVal;
	return true;
}

// Squirrel item value 偐傜 string 傪 fetch
static bool RTChildToStringProc(DWORD_PTR childVal)
{
	DWORD strLen;
	DWORD readSize;
	BOOL ret;
	HANDLE curProc = GetCurrentProcess();

	ret = ::ReadProcessMemory(curProc, (LPVOID)(childVal + 0x14), &strLen, sizeof strLen, &readSize);
	if (!ret) return false;
	if (!(0 < strLen && strLen < SQSTRING_LIMIT)) return false;

	ret = ::ReadProcessMemory(curProc, (LPVOID)(childVal + 0x1C), ipcData.str.val, strLen, &readSize);
	if (!ret) return false;

	ipcData.str.val[strLen] = 0;
	ipcData.str.len = strLen;
	return true;
}

LRESULT CALLBACK ipcWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_FINDRTCHILD:
		return FindRTChildProc() != false;
	case WM_RTCHILDTOSTRING:
		return RTChildToStringProc(wparam) != false;
	case WM_RTCHILDTOSTRING + 1:
	{
		FILE* fp;
		fp = fopen("result.txt", "w");
		EnumRTChildProc("", 0, fp);
		fclose(fp);
	}
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
static bool IPCWindowExists()
{
	return IsWindow(ipcWindow) || (ipcWindow = FindWindow(TH155AddrIPCWndClass, nullptr));
}

LRESULT CALLBACK TH155HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && !isIPCWindowCreated) {
		if (TH155LoadProfileForVictim()) {
			WNDCLASSEX wc;
			wc.cbSize = sizeof(WNDCLASSEX);
			wc.hInstance = libModuleSaved;
			wc.lpszClassName = TH155AddrIPCWndClass;
			wc.lpfnWndProc = ipcWindowProc;
			wc.style = CS_DBLCLKS;
			wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
			wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
			wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wc.lpszMenuName = nullptr;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
			if (RegisterClassEx(&wc) == 0) {
				OutputDebugString(_T("Failed registering ipcWindow class."));
			} else {
				if (CreateWindowEx(0, TH155AddrIPCWndClass, _T(""), WS_POPUPWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 300, 400, HWND_DESKTOP, nullptr, libModuleSaved, nullptr) == nullptr) {
					OutputDebugString(_T("Failed creating ipcWindow."));
				} else {
					TCHAR libModuleName[1024];
					GetModuleFileName(libModuleSaved, libModuleName, sizeof(libModuleName));
					LoadLibrary(libModuleName);

					isIPCWindowCreated = true;
					HANDLE hookedEvent = CreateEvent(nullptr, FALSE, FALSE, TH155AddrInstalledEvent);
					SetEvent(hookedEvent);
					CloseHandle(hookedEvent);
					OutputDebugString(_T("Creating ipcWindow succeeded."));
				}
			}
		}
		else {
			OutputDebugString(_T("Failed to load TH155 Profile."));
		}

	}
	return CallNextHookEx(hookHandle, nCode, wParam, lParam);
}

static bool FindRTChild(LPCSTR path, DWORD &retType, DWORD &retVal)
{
	if (!IPCWindowExists()) return false;
	DWORD pathLen = lstrlenA(path) + 1;
	if (!(0 < pathLen && pathLen < SQSTRING_LIMIT)) return false;
	HANDLE ipcLock = CreateMutex(nullptr, FALSE, TH155AddrIPCMutex);
	WaitForSingleObject(ipcLock, INFINITE);
	::RtlCopyMemory(ipcData.str.val, path, pathLen);
	LRESULT ret = SendMessage(ipcWindow, WM_FINDRTCHILD, 0, 0);
	if (ret != FALSE) {
		OutputDebugStringA(path);
		retType = ipcData.var.type;
		retVal = ipcData.var.val;
	}
	ReleaseMutex(ipcLock);
	CloseHandle(ipcLock);
	return ret != FALSE;
}


static bool RTChildToString(DWORD_PTR childVal, LPSTR outBuffer, DWORD bufLen)
{
	if (!IPCWindowExists()) return false;
	HANDLE ipcLock = CreateMutex(nullptr, FALSE, TH155AddrIPCMutex);
	WaitForSingleObject(ipcLock, INFINITE);
	LRESULT ret = SendMessage(ipcWindow, WM_RTCHILDTOSTRING, childVal, 0);
	if (ret != FALSE) {
		::RtlCopyMemory(outBuffer, ipcData.str.val, ipcData.str.len + 1);
		OutputDebugStringA(outBuffer);
	}
	ReleaseMutex(ipcLock);
	CloseHandle(ipcLock);
	return ret != FALSE;
}

static DWORD_PTR GetCharacterIdFromRomanName(LPCSTR romanName)
{
	for (DWORD_PTR i = 0; i < _countof(characterRomanNames); ++i) {
		if (!lstrcmpA(romanName, characterRomanNames[i])) {
			return i;
		}
	}
	return static_cast<DWORD_PTR>(-1);
}

static void APIENTRY InterruptAPCCallback(ULONG_PTR)
{
}

static void TH155Callback(short Msg, short param1, int param2)
{
	if (clientCallbackWnd) {
		::PostMessage(clientCallbackWnd, clientCallbackMsg, MAKELONG(Msg, param1), param2);
	}
}

static BOOL CALLBACK TH155FindWindowProc(HWND hwnd, LPARAM param)
{
	TCHAR textBuff[256];
	if (::GetClassName(hwnd, textBuff, _countof(textBuff)) > 0 &&
		::lstrcmp(TH155WindowClass, textBuff) == 0 &&
		::GetWindowText(hwnd, textBuff, _countof(textBuff)) > 0 &&
		::StrCmpN(TH155WindowCaption, textBuff, TH155WindowCaptionCount) == 0) {
		*reinterpret_cast<HWND*>(param) = hwnd;
		return FALSE;
	}

	return TRUE;
}

extern "C"
void WINAPI VirtualPress(int keyid)
{
	SleepEx(200, true);
	INPUT w;
	memset(&w, 0, sizeof(INPUT));
	int i = MapVirtualKey(keyid, 0);
	w.type = INPUT_KEYBOARD;
	w.ki.time = 0;
	w.ki.wVk = 0;
	w.ki.wScan = i;
	w.ki.dwFlags = KEYEVENTF_SCANCODE;
	//SendInput(1, &w, sizeof(w));

	keybd_event(keyid, i, 0, 1);
	SleepEx(1000, true);
	keybd_event(keyid, i, KEYEVENTF_KEYUP, 1);
	w.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;
	//SendInput(1, &w, sizeof(w));
	SleepEx(200, true);

}

extern "C"
void WINAPI VirtualPress_Extra(int keyid)
{
	
	SleepEx(50, true);
	INPUT ip;
	//Set up the INPUT structure
	ip.type = INPUT_KEYBOARD;
	ip.ki.time = 60;
	ip.ki.wVk = 0; //We're doing scan codes instead
	ip.ki.dwExtraInfo = 0;

	//This let's you do a hardware scan instead of a virtual keypress
	ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY;
	ip.ki.wScan = MapVirtualKey(keyid, 0);  //Set a unicode character to use (A)

						 //Send the press
	SendInput(1, &ip, sizeof(INPUT));
	SleepEx(50, true);
	//Prepare a keyup event
	ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
	SendInput(1, &ip, sizeof(INPUT));
	SleepEx(50, true);
}

extern "C"
HWND WINAPI TH155FindWindow()
{
	HWND hwndResult = nullptr;
	if (TH155WindowCaptionCount > 0) {
		::EnumWindows(TH155FindWindowProc, reinterpret_cast<LPARAM>(&hwndResult));
	}
	return hwndResult;
}

static TH155STATE TH155StateWaitForHooking()
{
	TH155STATE ret = TH155STATE_NOTFOUND;
	HWND TH155Window = TH155FindWindow();
	if (TH155Window != nullptr) {
		DWORD TH155TId, TH155PId;
		TH155TId = GetWindowThreadProcessId(TH155Window, &TH155PId);

		HANDLE hookLock = CreateMutex(nullptr, FALSE, TH155AddrInstallMutex);
		WaitForSingleObject(hookLock, INFINITE);
		if (!IPCWindowExists()) {
			hookHandle = SetWindowsHookEx(WH_GETMESSAGE, TH155HookProc, libModuleSaved, TH155TId);
			if (hookHandle != nullptr) {
				HANDLE hookedEvent = CreateEvent(nullptr, FALSE, FALSE, TH155AddrInstalledEvent);
				PostMessage(TH155Window, WM_NULL, 0, 0);
				if (WaitForSingleObject(hookedEvent, 10000) == 0) {
					TH155Callback(TH155MSG_STATECHANGE, TH155STATE_WAITFORNETBATTLE, 0);
					::ZeroMemory(paramOld, sizeof paramOld);
					ret = TH155STATE_WAITFORNETBATTLE;
					OutputDebugString(_T("HookCompleted"));
				}
				CloseHandle(hookedEvent);
				UnhookWindowsHookEx(hookHandle);
				hookHandle = nullptr;
			}
		} else {
			TH155Callback(TH155MSG_STATECHANGE, TH155STATE_WAITFORNETBATTLE, 0);
			::ZeroMemory(paramOld, sizeof paramOld);
			ret = TH155STATE_WAITFORNETBATTLE;
		}
		ReleaseMutex(hookLock);
		CloseHandle(hookLock);
	}
	return ret;
}

static TH155STATE TH155StateWaitForNetBattle()
{
	if (!IPCWindowExists()) {
		TH155Callback(TH155MSG_STATECHANGE, TH155STATE_NOTFOUND, 0);
		return TH155STATE_NOTFOUND;
	}

	DWORD childType, childVal;
	if ((::FindRTChild("network/inst", childType, childVal) && (childType & 0x8000) != 0) &&
		(::FindRTChild("network/is_watch", childType, childVal) && (childType & 0x8) != 0 && childVal == 0)) {
		TH155Callback(TH155MSG_STATECHANGE, TH155STATE_NETBATTLE, 0);
		return TH155STATE_NETBATTLE;
	}
	else {
		return TH155STATE_WAITFORNETBATTLE;
	}
}

static TH155STATE TH155StateNetBattle()
{
	if (!IPCWindowExists()) {
		TH155Callback(TH155MSG_STATECHANGE, TH155STATE_NOTFOUND, 0);
		return TH155STATE_NOTFOUND;
	}

	DWORD childType, childVal;
	if ((::FindRTChild("network/inst", childType, childVal) && (childType & 0x8000) != 0) &&
		(::FindRTChild("network/is_watch", childType, childVal) && (childType & 0x8) != 0 && childVal == 0)) {
		int param;
		for (int i = 0; i < TH155PARAM_MAX; ++i) {
			if ((param = TH155AddrGetParam(i)) != -1) {
				if (param != paramOld[i])
					TH155Callback(TH155MSG_PARAMCHANGE, i, param);
				paramOld[i] = param;
			}
			else paramOld[i] = 0;
		}
		return TH155STATE_NETBATTLE;
	}
	else {
		TH155Callback(TH155MSG_STATECHANGE, TH155STATE_WAITFORNETBATTLE, 0);
		return TH155STATE_WAITFORNETBATTLE;
	}
}

static DWORD WINAPI TH155AddrWorkerThread(LPVOID)
{
	MSG msg;

	TH155State = TH155STATE_NOTFOUND;
	while (!::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) || msg.message != WM_QUIT) {
		switch (TH155State) {
		case TH155STATE_NOTFOUND:            TH155State = TH155StateWaitForHooking(); break;
		case TH155STATE_WAITFORNETBATTLE:    TH155State = TH155StateWaitForNetBattle(); break;
		case TH155STATE_NETBATTLE:           TH155State = TH155StateNetBattle(); break;
		};
		::SleepEx(POLL_INTERVAL, TRUE);
	}
	::ExitThread(0);
	return 0;
}

extern "C"
bool WINAPI TH155IsConnect()
{
	DWORD childType, childVal;
	if ((::FindRTChild("network/inst", childType, childVal) && (childType & 0x8000) != 0)) {
		return true;
	}
	return false;
}

extern "C"
BOOL WINAPI TH155AddrStartup(int APIVersion, HWND callbackWnd, int callbackMsg)
{
	if (isClientInitialized) return FALSE;
	if (APIVersion != TH155_APIVERSION) return FALSE;

	if (!TH155LoadProfileForClient()) return FALSE;
	clientCallbackWnd = callbackWnd;
	clientCallbackMsg = callbackMsg;
	clientWorkerThread = ::CreateThread(NULL, 0, TH155AddrWorkerThread, NULL, 0, &clientWorkerThreadId);
	if (clientWorkerThread == nullptr) return FALSE;

	isClientInitialized = true;
	return TRUE;
}

extern "C"
BOOL WINAPI TH155AddrCleanup()
{
	if (!isClientInitialized)  return FALSE;

	::PostThreadMessage(clientWorkerThreadId, WM_QUIT, 0, 0);
	::QueueUserAPC(InterruptAPCCallback, clientWorkerThread, 0);
	::WaitForSingleObject(clientWorkerThread, INFINITE);
	::CloseHandle(clientWorkerThread);

	return 0;
}

extern "C"
TH155STATE WINAPI TH155AddrGetState()
{
	return TH155State;
}

extern "C"
int WINAPI TH155GetRTChildInt(LPCSTR param)
{
	DWORD childType, childVal;
	if (::FindRTChild(param, childType, childVal) && ((childType & 0xFFFFF) == 0x02 || (childType & 0xFFFFF) == 0x08)) {
		return childVal;
	}
	return -1;
}

extern "C"
int WINAPI TH155GetRTChildStr(LPCSTR param,LPSTR result)
{
	DWORD childType, childVal;
	char paramBuff[SQSTRING_LIMIT] = "Failed";
	if (::FindRTChild(param, childType, childVal) && (childType & 0xFFFFF) == 0x10 && RTChildToString(childVal, paramBuff, sizeof(paramBuff))) {
		memcpy(result, paramBuff, SQSTRING_LIMIT);
		return 0;
	}
	memcpy(result, paramBuff, SQSTRING_LIMIT);
	return 1;
}

extern "C"
int WINAPI TH155EnumRTCHild()
{
	return SendMessage(ipcWindow, WM_FINDRTCHILD + 2, 0, 0);
}

extern "C"
DWORD_PTR WINAPI TH155AddrGetParam(int param)
{
	DWORD childType, childVal;
	char paramBuff[SQSTRING_LIMIT];

	switch (param) {
	case TH155PARAM_BATTLESTATE:
		if (::FindRTChild("battle/state", childType, childVal) && (childType & 0xFFFFF) == 0x02) {
			return childVal;
		}
		break;
	case TH155PARAM_ISNETCLIENT:
		if (::FindRTChild("network/is_client", childType, childVal) && (childType & 0xFFFFF) == 0x08) {
			return childVal;
		}
		break;
	case TH155PARAM_P1CHAR:
		if (::FindRTChild("battle/team/0/master_name", childType, childVal) && (childType & 0xFFFFF) == 0x10 && RTChildToString(childVal, paramBuff, sizeof(paramBuff))) {
			return ::GetCharacterIdFromRomanName(paramBuff);
		}
		break;
	case TH155PARAM_P1CHAR_SLAVE:
		if (::FindRTChild("battle/team/0/slave_name", childType, childVal) && (childType & 0xFFFFF) == 0x10 && RTChildToString(childVal, paramBuff, sizeof(paramBuff))) {
			return ::GetCharacterIdFromRomanName(paramBuff);
		}
		break;
	case TH155PARAM_P2CHAR:
		if (::FindRTChild("battle/team/1/master_name", childType, childVal) && (childType & 0xFFFFF) == 0x10 && RTChildToString(childVal, paramBuff, sizeof(paramBuff))) {
			return ::GetCharacterIdFromRomanName(paramBuff);
		}
		break;
	case TH155PARAM_P2CHAR_SLAVE:
		if (::FindRTChild("battle/team/1/slave_name", childType, childVal) && (childType & 0xFFFFF) == 0x10 && RTChildToString(childVal, paramBuff, sizeof(paramBuff))) {
			return ::GetCharacterIdFromRomanName(paramBuff);
		}
		break;
	case TH155PARAM_P1WIN:
		if (::FindRTChild("battle/win/0", childType, childVal) && (childType & 0xFFFFF) == 0x02) {
			return childVal;
		}
		break;
	case TH155PARAM_P2WIN:
		if (::FindRTChild("battle/win/1", childType, childVal) && (childType & 0xFFFFF) == 0x02) {
			return childVal;
		}
		break;
	case TH155PARAM_P1NAME:
		if (::FindRTChild("network/player_name/0", childType, childVal) && (childType & 0xFFFFF) == 0x10 && RTChildToString(childVal, pnameBuff, sizeof(pnameBuff))) {
			return reinterpret_cast<DWORD_PTR>(pnameBuff);
		}
		break;
	case TH155PARAM_P2NAME:
		if (::FindRTChild("network/player_name/1", childType, childVal) && (childType & 0xFFFFF) == 0x10 && RTChildToString(childVal, pnameBuff, sizeof(pnameBuff))) {
			return reinterpret_cast<DWORD_PTR>(pnameBuff);
		}
		break;
	default:
		break;
	}
	return -1;
}

