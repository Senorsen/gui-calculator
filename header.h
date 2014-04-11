#include <windows.h>
#include <math.h>
#include "resource.h"
#pragma pack(1)
#define BOOL int

int wnd;

typedef struct
{
	char expr[10000];
	int degreetype,eps;
	HWND hwnd;		//ÎÄ±¾¿òµÄhwnd 
	HWND SThwnd;
}sthr,*pst;

typedef struct
{
	char prefix[11];
	int e;
	int d;
	char exp[10000];
}ff,*pff;

typedef struct bIgInTeGeR{
    int dig[30000];
} biginteger, *bigintegerP;

typedef struct vArIaBlE{
    int type;   // long, double, biginteger
    long vl;
    double vd;
    biginteger vb;
} variable, *variableP;

sthr threadinfo;

void ProgramInit(HINSTANCE hInstance);
void InitMenu(HWND hwnd);
void ProcCalc(HWND hwnd);
BOOL HandleEditRet(MSG *msg);
void SaveThis(HWND hwnd);
void OpenThis(HWND hwnd);
BOOL CALLBACK WndProc(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam);
BOOL WINAPI CalcThreadProc(LPVOID lpParam);
DWORD WINAPI ErrHandler(EXCEPTION_POINTERS* lpEi);

void _setmode(int e,int d);
int _calc(char *ex,char *ans);
void _outputAnswer(variable ans,char *szans);


