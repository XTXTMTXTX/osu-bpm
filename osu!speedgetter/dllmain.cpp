#undef UNICODE
#include<cstdio>
#include<string>
#include<windows.h>
#include<algorithm>
#include"MinHook.h"
#define _LIMIT__ 0
using namespace std;

typedef BOOL (WINAPI *pBASS_ChannelSetAttribute)(DWORD, DWORD, float);
HWND mainhWnd=NULL;
BOOL WINAPI MyBASS_ChannelSetAttribute(DWORD, DWORD, float);
pBASS_ChannelSetAttribute pOrigAttr = NULL,pAttr = NULL;
BOOL basshook;
volatile float speed = 1;
BOOL worked = 0;
extern "C" __declspec(dllexport) void initOSUSG(){
	MH_Initialize();
	pAttr = (pBASS_ChannelSetAttribute) GetProcAddress(GetModuleHandle("bass.dll"), "BASS_ChannelSetAttribute");
	basshook = MH_CreateHook((LPVOID)pAttr,(LPVOID)MyBASS_ChannelSetAttribute,(PVOID*)&pOrigAttr);
	if(basshook) {
		MessageBoxA(NULL,"Injection failed: bass hook", "Info", MB_ICONEXCLAMATION);
		exit(0);
	} else {
		worked = TRUE;
		MH_EnableHook(MH_ALL_HOOKS);
	}
}
double CPUclock() {
	LARGE_INTEGER nFreq;
	LARGE_INTEGER t1;
	double dt;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&t1);
	dt=(t1.QuadPart)/(double)nFreq.QuadPart;
	return(dt*1000);
}
INT APIENTRY DllMain(HMODULE hDLL,DWORD Reason,LPVOID Reserved) {
	switch(Reason){
		case DLL_PROCESS_ATTACH:{
			HANDLE h1=CreateThread(0,0,(LPTHREAD_START_ROUTINE)initOSUSG,0,0,0);CloseHandle(h1);
			break;
		}
		case DLL_PROCESS_DETACH:
			worked=0;
			MH_DisableHook(MH_ALL_HOOKS);
			MH_RemoveHook((LPVOID)pOrigAttr);
			MH_Uninitialize();
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}
double counter=0;
BOOL WINAPI MyBASS_ChannelSetAttribute(DWORD handle, DWORD attrib, float value) {
	if(CPUclock()<counter){return pOrigAttr(handle, attrib, value);}
	if(attrib==65536){
		speed=(value+100.0)/100.0;
		COPYDATASTRUCT cdata;
		cdata.dwData = 1;
	    cdata.lpData = (PVOID)&speed;
	    cdata.cbData = 4;
	    mainhWnd = FindWindow("osubpmreceiver",NULL);
	    SendMessage(mainhWnd, WM_COPYDATA, 0, (LPARAM)&cdata);
    }
    if(attrib==1){
    	if(abs(value-44100*1.5)>1e-4&&abs(value-48000*1.5)>1e-4)return pOrigAttr(handle, attrib, value);
    	counter=CPUclock()+2000;
		speed=1.5;
		COPYDATASTRUCT cdata;
		cdata.dwData = 1;
	    cdata.lpData = (PVOID)&speed;
	    cdata.cbData = 4;
	    mainhWnd = FindWindow("osubpmreceiver",NULL);
	    SendMessage(mainhWnd, WM_COPYDATA, 0, (LPARAM)&cdata);
    }
    
	return pOrigAttr(handle, attrib, value);
}
