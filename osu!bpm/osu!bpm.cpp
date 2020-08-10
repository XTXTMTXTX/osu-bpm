#include<tchar.h>
#include<windows.h>
#include<gl/gl.h>
#include<cstdio>
#include<cstdlib>
#include<cmath>
#include<vector>
#include<algorithm>
#include"SOIL.h"
#include<TlHelp32.h>
#define ll long long
#define max(x,y) ((x)>(y)?(x):(y))
#define min(x,y) ((x)<(y)?(x):(y))
#define abs(x) ((x)<0?-(x):(x))
using namespace std;
HANDLE hProcess;
DWORD PID=0;
bool quitflag=0,loading=1,delayF=1;
double BPMMax=0,BPMMin=0,BPM=0,beats=0;
volatile float speed = 1.00;
FILE *fp=0;
unsigned char aob[]={0x8B,0x76,0x10,0xDB,0x05,0x00,0x00,0x00,0x00,0xD9,0x5D,0xF8,0xD9,0x45,0xF8,0x6A},
             mask[]={   1,   1,   1,   1,   1,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1};
int aoboff=5;
struct info{
	ll x;
	double y;
}tmplist;
vector<info> list;
HMODULE DllInject(HANDLE hProcess,const char *dllname);
DWORD getPID(LPCSTR ProcessName) {
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)return 0;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap,&pe32)) {
		CloseHandle(hProcessSnap);          // clean the snapshot object
		return 0;
	}
	DWORD dwPid=0;
	do {
		if(!strcmp(ProcessName, pe32.szExeFile)) {
			dwPid=pe32.th32ProcessID;
			break;
		}
	} while(Process32Next(hProcessSnap,&pe32));
	CloseHandle(hProcessSnap);
	return dwPid;
}
LPVOID AOB(LPVOID Startpoint) {
	int len=sizeof(aob),pp=0;
	unsigned int faddr=0;
	unsigned char arBytes[2][4096],tmpByte[len*2+1];
	for(unsigned int addr=(unsigned int)0x00400000; (unsigned int)addr<=(unsigned int)2*1024*1024*1024; addr=(unsigned int)addr+4*1024) {
		ReadProcessMemory(hProcess,(LPVOID)addr,arBytes[0],4096,NULL);
		ReadProcessMemory(hProcess,(LPVOID)(addr+4*1024),arBytes[1],4096,NULL);
		for(int i=4096-len; i<4096; i++)tmpByte[i-4096+len]=arBytes[0][i];
		for(int i=0; i<len-1; i++)tmpByte[len+i]=arBytes[1][i];
		for(int i=(addr==0x00400000?0:len-1); i<4096; i++) {
			if(arBytes[0][i]*mask[pp]==aob[pp]*mask[pp]) {
				if(pp==len-1) {
					faddr=addr+i;
					if(DWORD(Startpoint)>faddr-len+1){
						i=i-pp;
						pp=0;
					}else
					return LPVOID(faddr-len+1);
				} else pp++;
			} else {
				i=i-pp;
				pp=0;
			}
		}
		for(int i=0; i<len-1; i++) {
			if(tmpByte[len+i]*mask[pp]==aob[pp]*mask[pp]) {
				if(pp==len-1) {
					faddr=addr+4096+i;
					if(DWORD(Startpoint)>faddr-len+1){
						i=i-pp;
						pp=0;
					}else
					return LPVOID(faddr-len+1);
				} else pp++;
			} else {
				i=i-pp;
				pp=0;
			}
		}

	}
	return 0;
}



GLuint Num0tex,Num1tex,Num2tex,Num3tex,Num4tex,Num5tex,Num6tex,Num7tex,Num8tex,Num9tex,BGtex,MASKtex,Lightingtex;

double CPUclock() {
	LARGE_INTEGER nFreq;
	LARGE_INTEGER t1;
	double dt;
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&t1);
	dt=(t1.QuadPart)/(double)nFreq.QuadPart;
	return(dt*1000);
}
int read(FILE *fp) {
	int x=0,f=1;
	char ch=fgetc(fp);
	while(ch<'0'||ch>'9') {
		if(ch=='-')f=-1;
		ch=fgetc(fp);
	}
	while(ch>='0'&&ch<='9') {
		x=x*10+ch-'0';
		ch=fgetc(fp);
	}
	return x*f;
}
GLuint Drawnumber(const int &x) {
	switch(abs(x)) {
	case 0:
		return Num0tex;
		break;
	case 1:
		return Num1tex;
		break;
	case 2:
		return Num2tex;
		break;
	case 3:
		return Num3tex;
		break;
	case 4:
		return Num4tex;
		break;
	case 5:
		return Num5tex;
		break;
	case 6:
		return Num6tex;
		break;
	case 7:
		return Num7tex;
		break;
	case 8:
		return Num8tex;
		break;
	case 9:
		return Num9tex;
		break;
	}
	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
void EnableOpenGL(HWND hWnd,HDC *hDC,HGLRC *hRC);
void DisableOpenGL(HWND hWnd,HDC hDC,HGLRC hRC);
void Work();
double g_offset=0;
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int iCmdShow) {
	FILE *fp=fopen("offset.ini","r");
	if(fp==NULL){
		fp=fopen("offset.ini","w");
		fprintf(fp,"0\n");
		fclose(fp);
	}
	fscanf(fp,"%lf",&g_offset);
	fclose(fp);
	printf("%lf\n",g_offset);
	WNDCLASS wc;
	HWND hWnd;
	HDC hDC;
	HGLRC hRC;
	MSG msg;
	BOOL bQuit=FALSE;
	wc.style=CS_OWNDC;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInstance;
	wc.hIcon=LoadIcon(hInstance,MAKEINTRESOURCE(3056));
	wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=_T("osubpmreceiver");
	RegisterClass(&wc);
	hWnd=CreateWindow(_T("osubpmreceiver"),"osu!bpm",WS_CAPTION|WS_POPUPWINDOW|WS_VISIBLE,100,100,336,98,NULL,NULL,hInstance,NULL);
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIcon);
	HANDLE h1=CreateThread(0,0,(LPTHREAD_START_ROUTINE)Work,0,0,0);CloseHandle(h1);
	EnableOpenGL(hWnd,&hDC,&hRC);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	BGtex=SOIL_load_OGL_texture("SKIN/BG.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num0tex=SOIL_load_OGL_texture("SKIN/0.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num1tex=SOIL_load_OGL_texture("SKIN/1.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num2tex=SOIL_load_OGL_texture("SKIN/2.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num3tex=SOIL_load_OGL_texture("SKIN/3.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num4tex=SOIL_load_OGL_texture("SKIN/4.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num5tex=SOIL_load_OGL_texture("SKIN/5.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num6tex=SOIL_load_OGL_texture("SKIN/6.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num7tex=SOIL_load_OGL_texture("SKIN/7.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num8tex=SOIL_load_OGL_texture("SKIN/8.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Num9tex=SOIL_load_OGL_texture("SKIN/9.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	MASKtex=SOIL_load_OGL_texture("SKIN/mask.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	Lightingtex=SOIL_load_OGL_texture("SKIN/lighting.png",SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID,SOIL_FLAG_POWER_OF_TWO|SOIL_FLAG_DDS_LOAD_DIRECT);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	while (!bQuit&&!quitflag) {
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
			if(msg.message==WM_QUIT) {
				bQuit=TRUE;
			} else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else {
			const double constFps=(double)(500);
			double timeInPerFrame=1000.0f/constFps;
			double timeBegin=CPUclock();
			/* OpenGL animation code goes here */
			glClearColor(0.0f,0.0f,0.0f,0.0f);
			glClear(GL_COLOR_BUFFER_BIT);


			glPushMatrix();

			glBindTexture(GL_TEXTURE_2D,BGtex);

			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-1.0f,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-1.0f,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (1.0f,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (1.0f,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D,Drawnumber((int(round(speed*BPMMin)+0.0001)/100)%10));
			glTranslatef(-0.799,-0.313,0.0f); 
			glScaled(1.0-0.561,1.0-0.561,1.0f);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-45.0/330.0,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-45.0/330.0,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (45.0/330.0,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (45.0/330.0,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D,Drawnumber((int(round(speed*BPMMin)+0.0001)/10)%10));
			glTranslatef(-0.658,-0.313,0.0f); 
			glScaled(1.0-0.561,1.0-0.561,1.0f);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-45.0/330.0,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-45.0/330.0,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (45.0/330.0,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (45.0/330.0,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D,Drawnumber(int(round(speed*BPMMin)+0.0001)%10));
			glTranslatef(-0.519,-0.313,0.0f); 
			glScaled(1.0-0.561,1.0-0.561,1.0f);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-45.0/330.0,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-45.0/330.0,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (45.0/330.0,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (45.0/330.0,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D,Drawnumber((int(round(speed*BPMMax)+0.0001)/100)%10));
			glTranslatef(0.483,-0.313,0.0f); 
			glScaled(1.0-0.561,1.0-0.561,1.0f);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-45.0/330.0,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-45.0/330.0,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (45.0/330.0,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (45.0/330.0,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D,Drawnumber((int(round(speed*BPMMax)+0.0001)/10)%10));
			glTranslatef(0.620,-0.313,0.0f); 
			glScaled(1.0-0.561,1.0-0.561,1.0f);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-45.0/330.0,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-45.0/330.0,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (45.0/330.0,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (45.0/330.0,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D,Drawnumber(int(round(speed*BPMMax)+0.0001)%10));
			glTranslatef(0.762,-0.313,0.0f); 
			glScaled(1.0-0.561,1.0-0.561,1.0f);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-45.0/330.0,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-45.0/330.0,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (45.0/330.0,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (45.0/330.0,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D,Drawnumber((int(round(speed*BPM)+0.0001)/100)%10));
			glTranslatef(-0.257,0.153,0.0f); 
			glScaled(1.0-0.2334,1.0-0.2334,1.0f);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-45.0/330.0,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-45.0/330.0,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (45.0/330.0,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (45.0/330.0,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D,Drawnumber((int(round(speed*BPM)+0.0001)/10)%10));
			glTranslatef(-0.024,0.153,0.0f); 
			glScaled(1.0-0.2334,1.0-0.2334,1.0f);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-45.0/330.0,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-45.0/330.0,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (45.0/330.0,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (45.0/330.0,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D,Drawnumber(int(round(speed*BPM)+0.0001)%10));
			glTranslatef(0.201,0.153,0.0f); 
			glScaled(1.0-0.2334,1.0-0.2334,1.0f);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-45.0/330.0,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-45.0/330.0,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (45.0/330.0,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (45.0/330.0,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			
			
			glPushMatrix();
			glColor4f(1, 1, 1, (1.0-beats*beats));
    		glBlendFunc( GL_SRC_ALPHA, GL_ONE );
			glBindTexture(GL_TEXTURE_2D,Lightingtex);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-1.0f,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-1.0f,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (1.0f,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (1.0f,-1.0f);
			glEnd ();
			glPopMatrix ();
			
			
			glPushMatrix();
			glColor4f(1, 1, 1, 1.0);
    		glBlendFunc( GL_SRC_ALPHA, GL_ONE );
			glBindTexture(GL_TEXTURE_2D,MASKtex);
			glBegin (GL_QUADS);
			glTexCoord2f(0.0f,1.0f);
			glVertex2f (-1.0f,-1.0f);
			glTexCoord2f(0.0f,0.0f);
			glVertex2f (-1.0f,1.0f);
			glTexCoord2f(1.0f,0.0f);
			glVertex2f (1.0f,1.0f);
			glTexCoord2f(1.0f,1.0f);
			glVertex2f (1.0f,-1.0f);
			glEnd ();
			glPopMatrix ();

			SwapBuffers (hDC);

			double timePhase=CPUclock()-timeBegin;
			if(timePhase<timeInPerFrame)Sleep(int(timeInPerFrame-timePhase));
		}
	}
	DisableOpenGL(hWnd,hDC,hRC);
	DestroyWindow(hWnd);
	return msg.wParam;
}
bool UpPrivilege(){   
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    bool result = OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken);   
    if(!result)return result;   
    result=LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&tkp.Privileges[0].Luid);   
    if(!result)return result;   
    tkp.PrivilegeCount=1; 
    tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;   
    result=AdjustTokenPrivileges(hToken,FALSE,&tkp,sizeof(TOKEN_PRIVILEGES),(PTOKEN_PRIVILEGES)NULL,(PDWORD)NULL);   
    return result;   
}

void Work(){
	UpPrivilege();
	while(!PID){
		PID=getPID("osu!.exe");
		Sleep(50);
	}
	Sleep(1000);
	ShellExecute(0,"open","osu!beatmapfinder.exe",0,0,SW_SHOWNORMAL);
	
	if(!(hProcess=OpenProcess(PROCESS_ALL_ACCESS,0,PID))){
		printf("Opening Failed\n");
		quitflag=1;return;
	}
	{
		char ch0[512];
		strcpy(ch0,_pgmptr);
		{
			char* p=ch0;
			while(strchr(p,'\\')) {
				p = strchr(p,'\\');
				p++;
			}
			*p = '\0';
		}
		strcat(ch0,"osu!speedgetter");
		DllInject(hProcess,ch0);
	}
	LPVOID timeaddraddr=0,timeaddr=0;
	do{
		timeaddraddr=LPVOID((unsigned int)(AOB(timeaddraddr))+aoboff);
		ReadProcessMemory(hProcess,timeaddraddr,&timeaddr,4,NULL);
	}while(DWORD(timeaddr)<=0x00001000);
	double time=0,Lsystime=CPUclock();
	int Ltime=0;
	int pp=0;
	while(PID==getPID("osu!.exe")&&(getPID("osu!beatmapfinder.exe")!=0)){
		if(loading){delayF=1;BPMMax=0;BPMMin=0;BPM=0;beats=0;continue;}
		if(delayF){Sleep(15);delayF=0;time=0;pp=0;Lsystime=CPUclock();Ltime=0;}
		if(abs(CPUclock()-Lsystime)>50){
			ReadProcessMemory(hProcess,timeaddr,&Ltime,4,NULL);
			Lsystime=CPUclock();
		}
		time=Ltime+(CPUclock()-Lsystime)*speed+g_offset;
		//printf("%d\n",time);
		while((pp+1<list.size())&&(list[pp+1].x<=time))pp++;
		while((pp-1>=0)&&(list[pp].x>time))pp--;
		BPM=list[pp].y;
		beats=time-list[pp].x;
		
		while(beats>=0)beats-=60000/min(2000,list[pp].y);
		while(beats<0)beats+=60000/min(2000,list[pp].y);
		beats/=60000/min(2000,list[pp].y);
		beats=max(0,beats);
		Sleep(2);
	}
	quitflag=1;
	CloseHandle(hProcess);
}
LRESULT CALLBACK WndProc (HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	switch (message) {
	case WM_CREATE:
		return 0;
	case WM_CLOSE:
		PostQuitMessage (0);
		return 0;
	case WM_DESTROY:
		return 0;
	case WM_KEYDOWN:
		return 0;
	case WM_COPYDATA: {
		COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT*)lParam;
		//printf("%s\n",pCopyData->lpData);
		if(pCopyData->dwData==0){
			loading=1;delayF=1;
			BPMMin=0;BPM=0;BPMMax=0;
			Sleep(15);
			if(!(fp=fopen((char*)pCopyData->lpData,"rt"))){
				return 0;
			}
			list.clear();
			while(!feof(fp)){
				if((fgetc(fp)=='[')&&(fgetc(fp)=='T')&&(fgetc(fp)=='i')&&(fgetc(fp)=='m')&&(fgetc(fp)=='i')&&(fgetc(fp)=='n')&&(fgetc(fp)=='g')
				&&(fgetc(fp)=='P')&&(fgetc(fp)=='o')&&(fgetc(fp)=='i')&&(fgetc(fp)=='n')&&(fgetc(fp)=='t')&&(fgetc(fp)=='s')&&(fgetc(fp)==']')){
					char tmps[512];
					long long timeP=0;double beatlen=1000,tbpm=0;
					fgets(tmps,512,fp);
					while(1){
						fgets(tmps,512,fp);
						if(!strchr(tmps,','))break;
						sscanf(tmps,"%lld,%lf",&timeP,&beatlen);
						if(beatlen<=0)continue;
						tbpm=60000.0/beatlen;
						if(tbpm>BPMMax||BPMMax==0)BPMMax=tbpm;
						if(tbpm<BPMMin||BPMMin==0)BPMMin=tbpm;
						tmplist.x=timeP;
						tmplist.y=tbpm;
						list.push_back(tmplist);
					}
					break;
				}
			}
			fclose(fp);
			loading=0;
			//printf("%.0lf - %.0lf\n",BPMMin,BPMMax);
			return 0;
		}
		if(pCopyData->dwData==1){
			speed=*(float*)(pCopyData->lpData);
			//printf("%f\n",speed);
		}
	}
	default:
		return DefWindowProc(hWnd,message,wParam,lParam);
	}
}

void EnableOpenGL(HWND hWnd,HDC *hDC,HGLRC *hRC) {
	PIXELFORMATDESCRIPTOR pfd;
	int iFormat;
	*hDC=GetDC(hWnd);
	ZeroMemory(&pfd,sizeof(pfd));
	pfd.nSize=sizeof(pfd);
	pfd.nVersion=1;
	pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
	pfd.iPixelType=PFD_TYPE_RGBA;
	pfd.cColorBits=24;
	pfd.cDepthBits=16;
	pfd.iLayerType=PFD_MAIN_PLANE;
	iFormat=ChoosePixelFormat(*hDC,&pfd);
	SetPixelFormat(*hDC,iFormat,&pfd);
	*hRC=wglCreateContext(*hDC);
	wglMakeCurrent(*hDC,*hRC);

}
void DisableOpenGL(HWND hWnd,HDC hDC,HGLRC hRC) {
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC);
	ReleaseDC(hWnd,hDC);
}
HMODULE DllInject(HANDLE hProcess,const char *dllname){
	unsigned long  (__stdcall *faddr)(void*);
	size_t abc;
	HMODULE hdll;
	HANDLE ht;
	LPVOID paddr;
	unsigned long exitcode;
	int dllnamelen;
	hdll=GetModuleHandleA("kernel32.dll");
	if(hdll==0) return 0;
	faddr=(unsigned long (__stdcall *)(void*))GetProcAddress(hdll,"LoadLibraryA");
	if(faddr==0) return 0;
	dllnamelen=strlen(dllname)+1;
	paddr=VirtualAllocEx(hProcess,NULL,dllnamelen,MEM_COMMIT,PAGE_READWRITE);
	if(paddr==0) return 0;
	WriteProcessMemory(hProcess,paddr,(void*)dllname,strlen(dllname)+1,(SIZE_T*) &abc);
	ht=CreateRemoteThread(hProcess,NULL,0,faddr, paddr,0,NULL);
	if(ht==0){
		VirtualFreeEx(hProcess,paddr,dllnamelen,MEM_DECOMMIT);
		return 0;
	}
	WaitForSingleObject(ht,INFINITE);
	GetExitCodeThread(ht,&exitcode);
	CloseHandle(ht);
	VirtualFreeEx(hProcess,paddr,dllnamelen,MEM_DECOMMIT);
	return (HMODULE)exitcode;
}
