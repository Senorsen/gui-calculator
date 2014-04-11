#include <windows.h>
#include <process.h>
#include <excpt.h>
#include <stdio.h>
#include "header.h"
#define MAXSTACK 1024300000

BOOL saveflag;	//保存标记，用于退出时提醒。对表达式进行变动就重置，保存为1 
WNDPROC OldEditWndProc;
HANDLE tcalc;
DWORD threadid;
CRITICAL_SECTION cs;
pff fileformat;

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	MSG Msg;
	int iStyle;
	HWND hWndCtrlNext;
	ProgramInit(hInstance);
	while(GetMessage(&Msg,NULL,0,0)>0)
	{
		if(HandleEditRet(&Msg)) continue;
		iStyle=GetWindowLong(Msg.hwnd,GWL_STYLE);
		SetWindowLong(Msg.hwnd,GWL_STYLE,iStyle|WS_TABSTOP);
		if(Msg.message==WM_KEYDOWN&&Msg.wParam==VK_TAB)
		{
			hWndCtrlNext=GetNextDlgTabItem(GetParent(GetFocus()),GetFocus(),(GetKeyState(VK_SHIFT)&0x8000)?TRUE:FALSE);//取得当前焦点控件的下一个控件的句柄
			if(hWndCtrlNext)
		    {
		    	SetFocus(hWndCtrlNext);
		    }
			continue;
		}
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}

void ProgramInit(HINSTANCE hInstance)
{
	int i;
	HWND hwnd;
	HFONT hf;
	wnd=NULL;
	fileformat=(pff)malloc(sizeof(ff));
	for(i=0;i<sizeof(ff);i++)
	{
		((char*)(fileformat))[i]=rand();
	}
	saveflag=0;
	InitializeCriticalSection(&cs);
	memset(&threadinfo,0,sizeof(threadinfo));
	hwnd=CreateDialogA(GetModuleHandle(NULL),MAKEINTRESOURCE(DLG_MAIN),NULL,WndProc);
	wnd=hwnd;
	if(hwnd)
	{
		SendMessage(wnd,WM_SETTEXT,0,"表达式计算器");
		ShowWindow(hwnd,SW_SHOW);
		SendDlgItemMessage(hwnd,IDC_ACC,WM_SETTEXT,0,"3");
		SendDlgItemMessage(hwnd,IDC_RAD,BM_SETCHECK,BST_CHECKED,0);
		SendDlgItemMessage(hwnd,IDC_DEG,BM_SETCHECK,BST_UNCHECKED,0);
		hf=CreateFont(24,0,0,700,0,0,0,0,0,0,0,0,0,"微软雅黑");
		SendDlgItemMessage(hwnd,IDC_EXP,WM_SETFONT,hf,MAKELONG(TRUE,0));
		SendDlgItemMessage(hwnd,IDC_ANS,WM_SETFONT,hf,MAKELONG(TRUE,0));
		SendDlgItemMessage(hwnd,IDC_ANS,WM_SETTEXT,0,"工作线程启动中……(如果长时间看到此信息，则可能说明内存不足，线程启动失败。)");
		threadinfo.hwnd=GetDlgItem(hwnd,IDC_ANS);
		threadinfo.SThwnd=GetDlgItem(hwnd,IDC_DIGITS);
		//tcalc=_beginthread(CalcThreadProc,0,&threadinfo);
		tcalc=CreateThread(NULL,MAXSTACK,CalcThreadProc,&threadinfo,NULL,&threadid);
	}
	else
	{
		MessageBox(NULL,"创建主窗体失败。请检查资源文件是否已被包含在工程中！","错误",MB_OK|MB_ICONSTOP);
		exit(0);
	}
}

void InitMenu(HWND hwnd)
{
	HMENU hMenu;
	hMenu=LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(MENU_MAIN));
	SetMenu(hwnd,hMenu);
}

void ProcCalc(HWND hwnd)
{
	char expr[10000];
	char tmp[50];
	int degreetype,eps=3;
	if(cs.OwningThread==threadid) return;	//仍然在计算中 
	SendDlgItemMessage(hwnd,IDC_ACC,WM_GETTEXT,50,tmp);
	sscanf(tmp,"%d",&eps);
	if(SendDlgItemMessage(hwnd,IDC_RAD,BM_GETCHECK,0,0)==BST_CHECKED) degreetype=1;
	if(SendDlgItemMessage(hwnd,IDC_DEG,BM_GETCHECK,0,0)==BST_CHECKED) degreetype=0;
	SendDlgItemMessage(hwnd,IDC_EXP,WM_GETTEXT,10000,expr);
	if(expr[0]==0)
	{
		//No need to bother if there's no TEXT.
		SendDlgItemMessage(hwnd,IDC_ANS,WM_SETTEXT,0,"表达式为空");
		SendDlgItemMessage(hwnd,IDC_DIGITS,WM_SETTEXT,0,"");
		return;
	}
	EnterCriticalSection(&cs);
	ResumeThread(tcalc);
	strcpy(threadinfo.expr,expr);
	threadinfo.degreetype=degreetype;
	threadinfo.eps=eps;
	SendDlgItemMessage(hwnd,IDC_ANS,WM_SETTEXT,0,"运算中，请稍候...");
	SendDlgItemMessage(hwnd,IDC_DIGITS,WM_SETTEXT,0,"");
	LeaveCriticalSection(&cs);
}

BOOL HandleEditRet(MSG *msg)
{
	MSG Msg;
	Msg=*msg;
	if(!wnd) return 0;
	if(Msg.message==WM_KEYDOWN&&(Msg.hwnd==GetDlgItem(wnd,IDC_EXP)||Msg.hwnd==GetDlgItem(wnd,IDC_ACC)))
	{
		if(Msg.wParam==VK_RETURN)
		{
			ProcCalc(wnd);
			return 1;
		}
		else
		{
			saveflag=1;
		}
	}
	return 0;
}

void SaveThis(HWND hwnd)
{
	OPENFILENAME ofn;
	char szFileName1[MAX_PATH];
	FILE *fp;
	int d,e;
	char tmp[50];
	szFileName1[0]=0;
	ZeroMemory(&ofn, sizeof(ofn));
	
	ofn.lpstrTitle="保存表达式";
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=hwnd;
	ofn.lpstrFilter="表达式文件(*.exp)\0*.exp\0所有文件(*.*)\0*.*\0";
	ofn.lpstrFile=szFileName1;
	ofn.nMaxFile=MAX_PATH;
	ofn.lpstrDefExt="exp";
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	
	if(GetSaveFileName(&ofn))
	{
		fp=NULL;
		fp=fopen(szFileName1,"wb");
		if(fp==NULL)
		{
			MessageBox(hwnd,"错误：保存失败，请重试","错误",MB_OK|MB_ICONSTOP);
			SaveThis(hwnd);	//重试吧骚年 
		}
		else
		{
			SendDlgItemMessage(hwnd,IDC_EXP,WM_GETTEXT,10000,fileformat->exp);
			SendDlgItemMessage(hwnd,IDC_ACC,WM_GETTEXT,50,tmp);
			sscanf(tmp,"%d",&e);
			if(SendDlgItemMessage(hwnd,IDC_RAD,BM_GETCHECK,0,0)==BST_CHECKED) d=1;
			if(SendDlgItemMessage(hwnd,IDC_DEG,BM_GETCHECK,0,0)==BST_CHECKED) d=0;
			fileformat->e=e;
			fileformat->d=d;
			strcpy(fileformat->prefix,"EXPRESSION");
			fwrite(fileformat,sizeof(ff),1,fp);
			fclose(fp);
			saveflag=0;
		}
	}
	else return;
}

void OpenThis(HWND hwnd)
{
	OPENFILENAME ofn;
	char szFileName1[MAX_PATH];
	FILE *fp;
	char tmp[50];
	szFileName1[0]=0;
	ZeroMemory(&ofn, sizeof(ofn));
	
	ofn.lpstrTitle="打开表达式文件";
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=hwnd;
	ofn.lpstrFilter="表达式文件(*.exp)\0*.exp\0所有文件(*.*)\0*.*\0";
	ofn.lpstrFile=szFileName1;
	ofn.nMaxFile=MAX_PATH;
	ofn.lpstrDefExt="exp";
	ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT;
	
	if(GetOpenFileName(&ofn))
	{
		fp=NULL;
		fp=fopen(szFileName1,"rb");
		if(fp==NULL)
		{
			MessageBox(hwnd,"错误：打开失败，请重试","错误",MB_OK|MB_ICONSTOP);
			OpenThis(hwnd);
		}
		else
		{
			fread(fileformat,sizeof(ff),1,fp);
			if(strcmp(fileformat->prefix,"EXPRESSION")!=0)
			{
				MessageBox(hwnd,"错误的文件格式","错误",MB_OK|MB_ICONSTOP);
				return; 
			}
			fclose(fp);
			SendDlgItemMessage(hwnd,IDC_EXP,WM_SETTEXT,0,fileformat->exp);
			sprintf(tmp,"%d",fileformat->e);
			SendDlgItemMessage(hwnd,IDC_ACC,WM_SETTEXT,0,tmp);
			SendDlgItemMessage(hwnd,IDC_DEG,BM_SETCHECK,BST_UNCHECKED,0);
			SendDlgItemMessage(hwnd,IDC_RAD,BM_SETCHECK,BST_UNCHECKED,0);
			if(fileformat->d==0)
			{
				SendDlgItemMessage(hwnd,IDC_DEG,BM_SETCHECK,BST_CHECKED,0);
			}
			else if(fileformat->d==1)
			{
				SendDlgItemMessage(hwnd,IDC_RAD,BM_SETCHECK,BST_CHECKED,0);
			}
			saveflag=0;
			ProcCalc(hwnd);
		}
	}
	else return;
}

BOOL CALLBACK WndProc(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam)
{
	char tmp[10000];int ret;
	switch(Message)
	{
	case WM_CREATE:
		wnd=hwnd;
		break;
	case WM_INITDIALOG:
		InitMenu(hwnd);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDM_EXIT:
			SendMessage(hwnd,WM_CLOSE,0,0);
			break;
		case IDM_CALC:
			ProcCalc(hwnd);
			break;
		case IDM_SAVE:
			SaveThis(hwnd);
			break;
		case IDM_OPEN:
			OpenThis(hwnd);
			break;
		}
		break;
	case WM_CLOSE:
		{
		SendDlgItemMessage(hwnd,IDC_EXP,WM_GETTEXT,10000,tmp);
		if(saveflag&&tmp[0]!=0)
		{
			ret=MessageBox(hwnd,"您还没有保存修改后的表达式。要保存它吗？","询问",MB_YESNOCANCEL|MB_ICONQUESTION|MB_DEFBUTTON3);
			switch(ret)
			{
			case IDCANCEL:
				break;
			case IDNO:
				EndDialog(hwnd,0);
				PostQuitMessage(0);
				break;
			case IDYES:
				SaveThis(hwnd);
				EndDialog(hwnd,0);
				PostQuitMessage(0);
				break;
			}
		}
		else
		{
			PostQuitMessage(0);
		}
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI CalcThreadProc(LPVOID lpParam)
{
	pst p;
	char *ans;int ret;int len;
	SetUnhandledExceptionFilter(ErrHandler);
	p=(pst)lpParam;
	SendMessage(p->hwnd,WM_SETTEXT,0,"工作线程已启动"); 
	SuspendThread(tcalc);
	while(1)
	{
		//进入临界区
		EnterCriticalSection(&cs);
		_setmode(p->eps,p->degreetype);
		ans=(char*)malloc(2560000*sizeof(char));
		if(!ans)
		{
			MessageBox(NULL,"CalcThreadProc 内存不足","错误",MB_OK|MB_ICONSTOP);
			SendMessage(p->hwnd,WM_SETTEXT,0,"CalcThreadProc 内存不足");
		}
		else
		{
			ret=_calc(p->expr,ans);
			SendMessage(p->hwnd,WM_SETTEXT,0,ans);
			len=strlen(ans);
			sprintf(ans,"%d 位",len);
			if(!ret) SendMessage(p->SThwnd,WM_SETTEXT,0,ans);
			free(ans);
		}
		LeaveCriticalSection(&cs);
		SuspendThread(tcalc);
	}
}

DWORD WINAPI ErrHandler(EXCEPTION_POINTERS* lpEi)
{
	char tmpstr[200];DWORD old;
	int e;e=lpEi->ExceptionRecord->ExceptionCode;
	if(e==EXCEPTION_STACK_OVERFLOW)
	{
		sprintf(tmpstr,"工作线程捕获到异常（栈溢出）。\r\n工作线程已停止。\r\n希望您能将您输入的表达式样本发给作者，谢谢！"); 
	}
	else
	{
		sprintf(tmpstr,"工作线程捕获到异常（0x%.8x）。\r\n工作线程已停止。\r\n希望您能将您输入的表达式样本发给作者，谢谢！",e); 
	}
	MessageBox(NULL,tmpstr,"意料之外的异常",MB_OK);
	LeaveCriticalSection(&cs);
	old=threadid;
	tcalc=CreateThread(NULL,MAXSTACK,CalcThreadProc,&threadinfo,NULL,&threadid);
	ExitThread(old);
	return 0;
}

