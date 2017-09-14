#include"moleFyz.h"


BOOL running=0;
BOOL loaded=0;
BOOL resset=0;	//bolo prepnute rozlisenie
BOOL sceneinit=0;


mD3DFrame scene;

mD3DMesh sky;
mD3DFrame fsky;

mD3DTexture text, text1, text2, tsky;
#define SMAPS 5
mD3DTexture SphereMap[SMAPS];

M3DMATERIAL Mat1={0.5f, 0.5f, 0.5f, 1.0f,		 0.6f, 0.6f, 0.6f, 1.0f,	 1.0f, 1.0f, 1.0f, 1.0f,	 0.0f, 0.0f, 0.0f, 1.0f,	 30.0f, 0.0f, 1.0f, 1.0f, "white"};
M3DMATERIAL Mat2={0.2f, 0.2f, 0.2f, 1.0f,	 0.6f, 0.6f, 0.6f, 1.0f,	 1.0f, 1.0f, 1.0f, 1.0f,	 0.0f, 0.0f, 0.0f, 1.0f,	 20.0f, 0.0f, 1.0f, 1.0f, "gold"};
M3DMATERIAL Mat3={0.6f, 0.6f, 0.6f, 1.0f,		 0.7f, 0.7f, 0.7f, 1.0f,	 1.0f, 1.0f, 1.0f, 1.0f,	 0.0f, 0.0f, 0.0f, 1.0f,	 40.0f, 0.0f, 1.0f, 1.0f, "gglass"};

HRESULT UnloadMain(void){
	if(sceneinit==1){
		ReleasemD3DFrame(scene,1);
		glDeleteTextures ( 1, (GLuint*)&text.Text);	
		glDeleteTextures ( 1, (GLuint*)&text1.Text);	
		glDeleteTextures ( 1, (GLuint*)&text2.Text);
		glDeleteTextures ( 1, (GLuint*)&tsky.Text);	
		for(int q=0;q<SMAPS;q++){
			glDeleteTextures ( 1, (GLuint*)&SphereMap[q].Text);	
		}
	}
	if(loaded==1){
		wglMakeCurrent( NULL, NULL );
		wglDeleteContext(hRC);
		ReleaseDC(hWnd,hDC);
	}
	if(FullScreen==1&&resset==1){
		ChangeDisplaySettings(NULL,0);
		//ShowCursor(TRUE);
	}
	FullScreen=0; 
	resset=0;
	running=0;
	sceneinit=0;
	loaded=0;

	return MM_OK;
}
HRESULT Minimize(void){
	if(FullScreen==1&&resset==1){
		if(ChangeDisplaySettings(NULL,0)!=DISP_CHANGE_SUCCESSFUL){return MM_DISP_CHANGE_UNSUCCESSFUL;}
		//ShowCursor(TRUE);
	}
	return MM_OK;
}
HRESULT RestoreUp(void){
	if(FullScreen==1){
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL){return MM_DISP_CHANGE_UNSUCCESSFUL;}
		//ShowCursor(FALSE);
	}	
	return MM_OK;
}
HRESULT InitScene(void){
	CameraPosition=M3DVECTOR(0.0f,0.0f,10.0f);
 	CameraOrientation=M3DVECTOR(0.0f,0.0f,-1.0f);
 	CameraUp=M3DVECTOR(0.0f,1.0f,0.0f);
	
//	ATTEMPT(LoadBMP_to_RGBA(0, "x.bmp", 0.0f, 0.0f, 1.0f, 1.0f, NULL, 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, &text, MIPMAPPING));
//	ATTEMPT(LoadBMP_to_RGBA(1, "y.bmp", 0.0f, 0.0f, 1.0f, 1.0f, NULL, 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, &text1, MIPMAPPING));

	ATTEMPT(LoadBMP(0, "x.bmp", 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, 0, 0, 0    , 0, 0, &text, MIPMAPPING));
	ATTEMPT(LoadBMP(1, "y.bmp", 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, 0, 0, 0    , 0, 0, &text1, MIPMAPPING));
	ATTEMPT(LoadBMP(2, "z.bmp", 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, 0, 0, 0    , 0, 0, &text2, MIPMAPPING));
	ATTEMPT(LoadBMP(3, "sky.bmp", 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, 0, 0, 0    , 0, 0, &tsky, MIPMAPPING));


	for(int q=0;q<SMAPS;q++){
		SphereMap[q].Text=1000+q;
	}

	ATTEMPT(CreatemD3DFrame(NULL, &scene));

//SKY
	ATTEMPT(LoadBMP_to_RGBA(3, "sky.bmp", 0.0f, 0.0f, 1.0f, 1.0f, NULL, 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, &tsky, MIPMAPPING));
	
	ATTEMPT(CreatemD3DFrame(scene, &fsky));
	ATTEMPT(CreateSphere(100, 10, 10, 0.0f, 0.0f, 1.0f, 1.0f, 2, &sky));
	ATTEMPT(sky->SetTexture(0,&tsky));
	sky->Textures=1;
	ATTEMPT(sky->SetFlags(MD3DMESHF_NOCULLING | MD3DMESHF_RENDEREDFIRST | MD3DMESHF_ALWAYSVISIBLE));
	ATTEMPT(fsky->AddMesh(1, sky) );
	fsky->Orientation=M3DVECTOR(0.0f, 0.0f, -1.0f);
//

	sceneinit=1;	
	return MM_OK;
}

HRESULT SetGL(void){
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	//perspective
	glMatrixMode(GL_PROJECTION);
	//gluPerspective(P_fov_vert, float(SCREEN_WIDTH)/float(SCREEN_HEIGHT), P_NPlane, P_FPlane);
	ProjectionMatrix=CreateProjectionMatrix( P_NPlane, P_FPlane, P_fov_horiz*(PI/180.0f), P_fov_vert*(PI/180.0f));
	glLoadMatrixf(&ProjectionMatrix._11);
	SMMatrix=CreateProjectionMatrix( P_NPlane, P_FPlane, SP_fov_horiz*(PI/180.0f), SP_fov_vert*(PI/180.0f));
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();	

	glShadeModel(GL_SMOOTH);
	glClearColor(0.3f, 0.3f, 0.6f, 1.0f);
	glClearDepth(1.0f);

	/*glEnable(GL_FOG);
	GLfloat fogColor[4] = {0.5, 0.6, 0.7, 1.0};
	long fogMode = GL_LINEAR;
	glFogi (GL_FOG_MODE, fogMode);
	glFogfv (GL_FOG_COLOR, fogColor);
	glFogf (GL_FOG_DENSITY, 0.05);
	glHint (GL_FOG_HINT, GL_DONT_CARE);
	glFogf (GL_FOG_START, 1.0);
	glFogf (GL_FOG_END, 200.0);*/
	
	glEnable(GL_LIGHTING);	//light
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_BLEND);	//alpha blending
	glEnable(GL_TEXTURE_2D);

	if(ZBUFFER==1){glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);}
	else{glDisable(GL_DEPTH_TEST);}
	if(PERSPECTIVE==1){glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);}
	else{glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);}
	if(DITHER==1){glEnable(GL_DITHER);}
	else{glDisable(GL_DITHER);}
	if(ANTIALIASING==1){glEnable(GL_POINT_SMOOTH); glEnable(GL_LINE_SMOOTH); glEnable(GL_POLYGON_SMOOTH);}
	else{glDisable(GL_POINT_SMOOTH);glDisable(GL_LINE_SMOOTH);glDisable(GL_POLYGON_SMOOTH);}
	glEnable(GL_CULL_FACE);glCullFace(GL_BACK);		


	M4DVECTOR v4;

	Light[0]=0;
	float source_light[] = {0.5f, 0.5f, 0.5f, 1.0f};
	float source_speclight[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float XX=1.0f, YY=0.0f, ZZ=0.0f;
	Light_Pos[0]=M3DVECTOR(10.0f, 0.0f, 0.0f);
   	v4=M4DVECTOR(Light_Pos[0]);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, source_light );
	glLightfv(GL_LIGHT0, GL_SPECULAR, source_speclight );
   	glLightfv(GL_LIGHT0, GL_POSITION, &v4.x );
	glLightfv(GL_LIGHT0, GL_CONSTANT_ATTENUATION, &XX); 
	glLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, &YY); 
	glLightfv(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, &ZZ); 
   	glEnable( GL_LIGHT0 );

	Light[1]=1;
	float source_light1[] = {0.2f, 0.2f, 0.2f, 1.0f};
	float source_speclight1[] = {0.3f, 0.3f, 0.3f, 1.0f};
	float XX1=1.0f, YY1=0.0f, ZZ1=0.0f;
	Light_Pos[1]=M3DVECTOR(-10.0f, 0.0f, 0.0f);
	v4=M4DVECTOR(Light_Pos[1]);
   	glLightfv(GL_LIGHT1, GL_DIFFUSE, source_light1 );
	glLightfv(GL_LIGHT1, GL_SPECULAR, source_speclight1 );
   	glLightfv(GL_LIGHT1, GL_POSITION, &v4.x );
	glLightfv(GL_LIGHT1, GL_CONSTANT_ATTENUATION, &XX1); 
	glLightfv(GL_LIGHT1, GL_LINEAR_ATTENUATION, &YY1); 
	glLightfv(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, &ZZ1); 
   	glEnable( GL_LIGHT1 );

/*	Light[2]=1;
	float source_light2[] = {0.0f, 0.0f, 0.5f, 1.0f};
	float source_speclight2[] = {0.0f, 0.0f, 1.0f, 1.0f};
	float XX2=1.0f, YY2=0.0f, ZZ2=0.0f;
	Light_Pos[2]=M3DVECTOR(0.0f, 10.0f, 0.0f);
	v4=M4DVECTOR(Light_Pos[2]);
   	glLightfv(GL_LIGHT2, GL_DIFFUSE, source_light2 );
	glLightfv(GL_LIGHT2, GL_SPECULAR, source_speclight2 );
   	glLightfv(GL_LIGHT2, GL_POSITION, &v4.x );
	glLightfv(GL_LIGHT2, GL_CONSTANT_ATTENUATION, &XX2); 
	glLightfv(GL_LIGHT2, GL_LINEAR_ATTENUATION, &YY2); 
	glLightfv(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, &ZZ2); 
   	glEnable( GL_LIGHT2 );

	Light[3]=1;
	float source_light3[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float source_speclight3[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float XX3=1.0f, YY3=0.0f, ZZ3=0.0f;
	Light_Pos[3]=M3DVECTOR(0.0f, 0.0f, 0.0f);
	v4=M4DVECTOR(Light_Pos[3]);
   	glLightfv(GL_LIGHT3, GL_DIFFUSE, source_light3 );
	glLightfv(GL_LIGHT3, GL_SPECULAR, source_speclight3 );
   	glLightfv(GL_LIGHT3, GL_POSITION, &v4.x );
	glLightfv(GL_LIGHT3, GL_CONSTANT_ATTENUATION, &XX3); 
	glLightfv(GL_LIGHT3, GL_LINEAR_ATTENUATION, &YY3); 
	glLightfv(GL_LIGHT3, GL_QUADRATIC_ATTENUATION, &ZZ3); 
   	glEnable( GL_LIGHT3 );
	*/

	//tieto zlozky sa budu citat z vertex color
	float ambient_light[] = {0.9f, 0.9f, 0.9f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient_light );
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0 );

	return MM_OK;
}
HRESULT MainX(void){
	//vraj je to lepsie
	_control87(MCW_EM, MCW_EM);


	PIXELFORMATDESCRIPTOR pfd={
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		BPP,
		0, 0, 0, 0, 0, 0,// Color Bits Ignored
		0,// No Alpha Buffer
		0,// Shift Bit Ignored
		0,// Accumulation Buffer
		0, 0, 0, 0,// Accumulation Bits Ignored
		ZBPP,// Z-Buffer (Depth Buffer)
		0,// Stencil Buffer
		0,// Auxiliary Buffer
		PFD_MAIN_PLANE,// Main Drawing Layer
		0,// Reserved
		0, 0, 0// Layer Masks Ignored
	};
	hDC=GetDC(hWnd);
	if (hDC==NULL){return MM_INVALID_DC;}
	
	PixelFormat=ChoosePixelFormat(hDC,&pfd);
	if (PixelFormat==NULL){return MM_INVALID_PIXEL_FORMAT;}
	if(!SetPixelFormat(hDC,PixelFormat,&pfd)){return MM_INVALID_PIXEL_FORMAT;}

	hRC=wglCreateContext(hDC);
	if (hRC==NULL){return MM_HRC;}
	if(!wglMakeCurrent(hDC,hRC)){return MM_HRC;}
/**********************************/
	loaded=1;

	if(FullScreen==1){
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= SCREEN_WIDTH;
		dmScreenSettings.dmPelsHeight	= SCREEN_HEIGHT;	
		dmScreenSettings.dmBitsPerPel	= BPP;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
	
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL){return MM_DISP_CHANGE_UNSUCCESSFUL;}
		//ShowCursor(FALSE);
		resset=1;
	}	
	/******************** EXTENSIONS ********************/
        FILE *file;
	file=fopen("GL.txt", "wt");
	if(file==NULL){return MM_ERROR;}

	char *VERSION=(char*)glGetString(GL_VERSION);
	fprintf(file, "%s\n", VERSION);
	#ifdef MULTITEXTURING 
		MULTI_TEXTURE_SUPPORTED=InitMultitexture();
	#endif
	char *extensions;
	extensions=strdup((char *) glGetString(GL_EXTENSIONS));	
	fprintf(file, "%s", extensions);
	fclose(file);
	/***************************************************/
	ATTEMPT(SetGL());
	//aby tam nebol WINDOWS
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SwapBuffers( hDC );

	ATTEMPT(InitScene());
	//font
	makeRasterFont();
	ATTEMPT(glGetError());
	
	//aby sa nezmenila kamera
	SetCursorPos(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);

	running=1;
	return MM_OK;
}
