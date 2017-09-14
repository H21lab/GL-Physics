#include"game.h"


float deltaOtoc=1.0f;	//pre hl. look

int MouseX=0, MouseY=0;
BOOL LB=0, RB=0;
int _MouseX=SCREEN_WIDTH/2, _MouseY=SCREEN_HEIGHT/2;
int MdX, MdY;	//odchylka
float MXSLOWDOWN=500.0f, MYSLOWDOWN=500.0f;	//mys x, y, spomalenie

mD3DObject *OBJ=NULL;
M3DVECTOR PosunChytenia=M3DVECTOR(0.0f, 0.0f, 0.0f);
M3DVECTOR Vel=M3DVECTOR(0.0f, 0.0f, 0.0f);

HRESULT KeyPress(void){
	if(GetAsyncKeyState(VK_F10)){
			UnloadMain();
			return MM_OK;
	}
	M3DVECTOR right=CrossProduct(CameraUp,CameraOrientation);
	if(GetAsyncKeyState(VK_LEFT)){
		CameraOrientation=POINTROTATE(CameraOrientation, M3DVECTOR(0.0f,0.0f,0.0f), CameraUp, deltaOtoc*Tc);
	}
	if(GetAsyncKeyState(VK_RIGHT)){
		CameraOrientation=POINTROTATE(CameraOrientation, M3DVECTOR(0.0f,0.0f,0.0f), CameraUp, -deltaOtoc*Tc);
	}
	if(GetAsyncKeyState(VK_UP)){
		CameraOrientation=POINTROTATE(CameraOrientation, M3DVECTOR(0.0f,0.0f,0.0f), right, -deltaOtoc*Tc);
		CameraUp=POINTROTATE(CameraUp, M3DVECTOR(0.0f,0.0f,0.0f), right, -deltaOtoc*Tc);
	}
	if(GetAsyncKeyState(VK_DOWN)){
		CameraOrientation=POINTROTATE(CameraOrientation, M3DVECTOR(0.0f,0.0f,0.0f), right, deltaOtoc*Tc);
		CameraUp=POINTROTATE(CameraUp, M3DVECTOR(0.0f,0.0f,0.0f), right, deltaOtoc*Tc);
	}
	/*Q*/
	if(GetAsyncKeyState(81)){
		CameraUp=POINTROTATE(CameraUp, M3DVECTOR(0.0f,0.0f,0.0f), CameraOrientation, -deltaOtoc*Tc);
	}
	/*W*/
	if(GetAsyncKeyState(87)){
		CameraUp=POINTROTATE(CameraUp, M3DVECTOR(0.0f,0.0f,0.0f), CameraOrientation, deltaOtoc*Tc);
	}
	/*A*/
	if(GetAsyncKeyState(65)){
		CameraPosition=CameraPosition+20.0f*CameraOrientation*Tc;
	}	
	/*Z*/
	if(GetAsyncKeyState(90)){
		CameraPosition=CameraPosition-20.0f*CameraOrientation*Tc;
	}
	if(GetAsyncKeyState('X')){
		CameraPosition=CameraPosition+20.0f*right*Tc;
	}	
	if(GetAsyncKeyState('C')){
		CameraPosition=CameraPosition-20.0f*right*Tc;
	}


	if(GetAsyncKeyState('B')){
		Light_Pos[0].x=Light_Pos[0].x+20.0f*Tc;
	}	
	if(GetAsyncKeyState('V')){
		Light_Pos[0].x=Light_Pos[0].x-20.0f*Tc;
	}	
	
	return MM_OK;
}
float uh_s=0.0f;
HRESULT Loop(void){
	ATTEMPT(ComputeScene());

	if(LB==1){
		M3DVECTOR A, u;
		GetLinefPixel(&A, &u, MouseX, MouseY);
		if(OBJ==NULL){
			OBJ=ComputeIntersection(A, u);
			if(OBJ!=NULL){
				OBJ->Velocity=M3DVECTOR(0.0f, 0.0f, 0.0f);
				PosunChytenia=LinePlane(OBJ->Position, CameraOrientation, A, u)-OBJ->Position;
			}
		}
		if(OBJ!=NULL){
			M3DVECTOR lastPos=OBJ->Position;
			//OBJ->Rotations=IdentityMatrix();
			OBJ->New_Velocity=(M3DVECTOR(LinePlane(OBJ->Position, CameraOrientation, A, u)-PosunChytenia-lastPos)/Tc)/5.0f;
		}
	}

	/*uh_s+=PI*Tc;
	while(uh_s>2.0f*PI){uh_s-=2.0f*PI;}
	Light_Pos[1].x=10.0f*cos(uh_s);
	Light_Pos[1].y=10.0f*sin(uh_s);

	Light_Pos[3]=Ball[0].Position;*/


	return MM_OK;
}
HRESULT Render(void){
	#ifdef CLEARSCENE
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	#endif
	#ifndef CLEARSCENE
		glClear(GL_DEPTH_BUFFER_BIT);
	#endif

	ATTEMPT(RenderFrame(scene));

//	TEXT
	InitTX();
	char Text[50];
	sprintf(Text, "%.0f", float(FPS));
	Printf(20,20, Text ,1.0f,1.0f,0.0f,1.0f);
	EndTX();	

	ATTEMPT(glGetError());
	SwapBuffers( hDC );

	return MM_OK;
}
