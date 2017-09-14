#include "resource.h"
#include "loop.h"

// Foward declarations of functions included in this code module:
ATOM			MyRegisterClass(HINSTANCE hInstance);
BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

HRESULT RetVal=0;
LPCSTR buf;	//error message

int APIENTRY WinMain(HINSTANCE hInstance,  HINSTANCE hPrevInstance,  LPSTR lpCmdLine, int nCmdShow){
	hPrevInstance;    //kvoli warningu
	lpCmdLine;

	MSG msg;

	// Initialize global strings
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (InitInstance (hInstance, nCmdShow)!=TRUE){return FALSE;}

	// Main message loop:
	while ( loaded!=0 ) 
	{
		if(GetMessage(&msg, NULL, 0U, 0U)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(running==1){
			t2=clock();
			sTime=float(time_t(t2-t1))/float(CLOCKS_PER_SEC);
			while(sTime>float(Ncactual)*Tc){
				Ncactual++;
				//KLAVESNICA
				RetVal=KeyPress();
				if(RetVal!=MM_OK){
					UnloadMain();
					ShowWindow(hWnd, 0);
					UpdateWindow(hWnd);
					if(RetVal!=MM_HIDDEN){
						buf= strerror(RetVal);
						MessageBox(NULL,buf,"Fatal Error",MB_OK);
						delete []buf;
					}
					PostQuitMessage(0);
					return FALSE;
				}
			
				//LOOP
				if(running==1){
					RetVal=Loop();
					if(RetVal!=MM_OK){
						UnloadMain();
						ShowWindow(hWnd, 0);
						UpdateWindow(hWnd);
						if(RetVal!=MM_HIDDEN){
							buf= strerror(RetVal);
							MessageBox(NULL,buf,"Error",MB_OK);
							delete []buf;
						}
						PostQuitMessage(0);
						return FALSE;
					}
				}
			} 	
			if(sTime>=1.0f){
				t1=clock();
				#ifdef AUTO_ADJUST_FPS
					N_of_cycles=(Nrenderedframe+Ncactual)/2; Tc=1.0f/N_of_cycles;
				#endif
				sTime=0.0f; Ncactual=0; 
				FPS=float(Nrenderedframe); Nrenderedframe=0;
			}
			#ifdef AUTO_ADJUST_FPS
			else{
			#endif
			#ifndef AUTO_ADJUST_FPS
			else if(Nrenderedframe<Ncactual){
			#endif
				Nrenderedframe++;
				if(running==1){
					RetVal=Render();
					if(RetVal!=MM_OK){
						UnloadMain();
						ShowWindow(hWnd, 0);
						UpdateWindow(hWnd);
						if(RetVal!=MM_HIDDEN){
							buf= strerror(RetVal);
							MessageBox(NULL,buf,"Error",MB_OK);
							delete []buf;
						}
						PostQuitMessage(0);
						return FALSE;
					}
				}
			}
		}
	}
	UnloadMain();
	DestroyWindow( hWnd );
	PostQuitMessage(0);
	
	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
		wcex.cbSize 		= sizeof(WNDCLASSEX); 
		wcex.style			=  CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wcex.lpfnWndProc		= (WNDPROC)WndProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= hInstance;
		wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_ICON);
		wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground	= NULL;
		wcex.lpszMenuName		= NULL;
		wcex.lpszClassName	= "Cname";
		wcex.hIconSm		= LoadIcon(hInstance, (LPCTSTR)IDI_ICON);
	
	return RegisterClassEx(&wcex);

}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow){
   DWORD dwStyle, dwExStyle;
   if(FullScreen==1){
	dwStyle=WS_POPUP;
	dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
   }
   else{
	dwStyle=WS_OVERLAPPEDWINDOW;
	dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
  }
   AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);	
   hWnd = CreateWindowEx(dwExStyle, "Cname", "GLPhysics", WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle, 0, 0, WindowRect.right-WindowRect.left, WindowRect.bottom-WindowRect.top, NULL, NULL, hInstance, NULL);
   if (!hWnd){return FALSE;}
   hInst=hInstance;

   ShowWindow(hWnd, nCmdShow /*3*/);
   UpdateWindow(hWnd);
   SetForegroundWindow(hWnd);
   SetFocus(hWnd);	//keyboard

   RetVal=MainX();
   if(RetVal!=MM_OK){
	UnloadMain();
	ShowWindow(hWnd, 0);
	UpdateWindow(hWnd);
	if(RetVal!=MM_HIDDEN){
		buf= strerror(RetVal);
		MessageBox(NULL,buf,"Error",MB_OK);
		delete []buf;
	}
	PostQuitMessage(0);
	return FALSE;
   }
   RetVal=InitSys();
   if(RetVal!=MM_OK){
	if(RetVal!=MM_HIDDEN){
		ShowWindow(hWnd, 0);
		UpdateWindow(hWnd);
		buf= strerror(RetVal);
		MessageBox(NULL,buf,"Error",MB_OK);
	}
	UnloadMain();
	PostQuitMessage(0);
	return FALSE;
   }   

   t1=clock();
   return TRUE;
}
LRESULT CALLBACK WndProc(HWND _hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL _running;

	switch (message) 
	{
		case WM_CREATE:
			break;
		case WM_PAINT:
			break;
		case WM_ACTIVATEAPP:
			_running = (BOOL)wParam;
			if(loaded==1){
				if (_running==1){
					t1=clock();
					if(running==0){RestoreUp();
						if(SCREEN_WIDTH<=0){SCREEN_WIDTH=1;}if(SCREEN_HEIGHT<=0){SCREEN_HEIGHT=1;}
					}
				}
				else{
					if(running==1){Minimize();}				
				}
				running=_running;
			}
            	break;
		case WM_SIZE:
			SCREEN_WIDTH=LOWORD(lParam); SCREEN_HEIGHT=HIWORD(lParam);
			if(FullScreen==0){glViewport(0, 0, LOWORD(lParam),HIWORD(lParam));}

			break;
		case WM_CLOSE:
			UnloadMain();
			PostQuitMessage( 0 );
			break;
		case WM_DESTROY:
			UnloadMain();
			PostQuitMessage( 0 );
			break;
		case WM_LBUTTONDOWN:
			LB=1;
			break;
		case WM_LBUTTONUP:
			OBJ=NULL;
			LB=0;
			break;
		case WM_RBUTTONDOWN:
			RB=1;
			break;
		case WM_RBUTTONUP:
			RB=0;
			break;
		case WM_MOUSEMOVE:
			_MouseX=MouseX; _MouseY=MouseY;
			MouseX = (int)LOWORD(lParam);
			MouseY = (int)HIWORD(lParam);
			MdX=MouseX-_MouseX; MdY=MouseY-_MouseY;
			break;
		default:
			return DefWindowProc(_hWnd, message, wParam, lParam);
   }
   return 0;
}

