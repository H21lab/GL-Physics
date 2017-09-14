#include<stdlib.h>
#include<time.h>
#include<windows.h>
#include<math.h>
#include<gl/gl.h>
#include<gl/glu.h>
#include<gl/glext.h>
#include<float.h>
#include"Errors.h"
#include"vectors.h"
#include<stdio.h>

HRESULT _RetVal;	//pre ATTEMPT
#define ATTEMPT(x) _RetVal=x; if(_RetVal!=MM_OK){return _RetVal;}

#define PI 3.1415926535897f

/**********************PRE hl. okno*********************/
// Global Variables:
HINSTANCE hInst;				// current instance
HWND hWnd=NULL;
/*******************************************************/
HDC	hDC=NULL;
HGLRC	hRC=NULL;

BOOL FullScreen=0;
int SCREEN_WIDTH=800;
int SCREEN_HEIGHT=600;
int SM_WIDTH=64, SM_HEIGHT=64;	//velkost obrazu pre SPHERE MAP
BYTE BPP=24;
BYTE ZBPP=24;				//z-buff
DWORD TEXTF=GL_RGBA;
RECT WindowRect={0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};	
DEVMODE dmScreenSettings;	//nastavenie pre prepinanie rozlisenia		
GLuint PixelFormat;

BOOL PERSPECTIVE=1;
BOOL ZBUFFER=1;
int ZSORT=1;	//1 sortovat obj. od najvzdialenejsieho, -1 naopak, ci funguje env. mapping
			//bere stredy framov pre porovnavanie
float VIEWDIST1=0.0f, VIEWDIST2=1.0f;	// v predu, v zadu, do akej vzdialenosti renderovat (0=neobmedzene), vyzaduje ZSORT
BOOL DITHER=1;
BOOL MIPMAPPING=1;
BOOL ANTIALIASING=1;
#define MULTITEXTURING
#define CLEARSCENE	//mazat obrazovku kazdy frame

int TEXT_MAG_FILTER=GL_LINEAR;
int TEXT_MIN_FILTER=GL_LINEAR;
int TEXT_MIN_FILTER_MIP=GL_LINEAR_MIPMAP_LINEAR;

/*********************TIME*********************/
#define AUTO_ADJUST_FPS		//medzi N_of_cycles FPS hlada optimum

float N_of_cycles=60.0f;	//pocet opakovani za 1s
float Tc=1.0f/N_of_cycles;	//trvanie 1 opakovania
time_t t1,t2;
float sTime=0.0f;		//cas cyklu <0.0f;1.0f)s
int Ncactual=0;		//cislo cyklu - poradie <0;N_of_cycles)
int Nrenderedframe=0;	//cislo uz vyrederovanych obrazkov od zac s
float FPS=0.0f;

/**********************************************/
/************************ENGINE*****************/
/*************CAMERA***********/
M3DVECTOR CameraPosition,CameraOrientation,CameraUp;
M3DMATRIX ViewMatrix;
M3DMATRIX ProjectionMatrix, SMMatrix;

//PROJECTION
float P_NPlane=0.1f;
float P_FPlane=999999.0f;
float P_fov_vert=50.0f;	//celkovo
float P_fov_horiz=(float(SCREEN_WIDTH)/float(SCREEN_HEIGHT))*P_fov_vert;
float SP_fov_vert=160.0f; //celkovo DSPHEREMAP
float SP_fov_horiz=160.0f;
/*******************************/

#define MAXMESHS 20		//max # of meshs in one mesh or frame
#define MAXFRAMES 150		//max # of frames in one frame
#define MAXNFACE 50		//max # of vertex per face
#define MAXOBJS 30000		//max # of objects in the scene - pre Z sort
#define MAXFRAMESS 30000	//max # of frames in the scene - pre Z sort
#define NAMELENGHT 16		//lenght of Name of the Mesh in chars

/*********OPENGL EXTENSION***********/
BOOL __ARB_ENABLE=1;	//pouzivat ARB ?
BOOL MULTI_TEXTURE_SUPPORTED=0;
int MaxTexelUnits=1;
//MULTITEXT
PFNGLMULTITEXCOORD1FARBPROC		glMultiTexCoord1fARB	= NULL;
PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2fARB	= NULL;
PFNGLMULTITEXCOORD3FARBPROC		glMultiTexCoord3fARB	= NULL;
PFNGLMULTITEXCOORD4FARBPROC		glMultiTexCoord4fARB	= NULL;
PFNGLACTIVETEXTUREARBPROC		glActiveTextureARB	= NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC	glClientActiveTextureARB= NULL;
//EXT ARRAY
PFNGLARRAYELEMENTEXTPROC		glArrayElementEXT		=NULL;
PFNGLCOLORPOINTEREXTPROC		glColorPointerEXT		=NULL;
PFNGLDRAWARRAYSEXTPROC			glDrawArraysEXT		=NULL;
PFNGLEDGEFLAGPOINTEREXTPROC		glEdgeFlagPointerEXT	=NULL;
PFNGLGETPOINTERVEXTPROC			glGetPointervEXT		=NULL;
PFNGLINDEXPOINTEREXTPROC		glIndexPointerEXT		=NULL;
PFNGLNORMALPOINTEREXTPROC		glNormalPointerEXT	=NULL;
PFNGLTEXCOORDPOINTEREXTPROC		glTexCoordPointerEXT	=NULL;
PFNGLVERTEXPOINTEREXTPROC		glVertexPointerEXT	=NULL;
/************************************/

/*FLAGS for Mesh*/
#define MD3DMESHF_DISABLED 1l	//mesh sa nerenderuje
#define MD3DMESHF_SPHERMAP 2l	//mesh ma env. mapping-staticky
#define MD3DMESHF_DSPHERMAP 4l	//mesh ma dynamicky env. mapping
#define MD3DMESHF_SPHERMAP2 8l	//mesh ma env. mapping-staticky na 2.text. coor
#define MD3DMESHF_NOCULLING 16l	//vykreslenie zadnych stien
#define MD3DMESHF_RENDEREDFIRST 32l	//vyrenderuje ako prve ak je ZSORT zapnuty
#define MD3DMESHF_ALWAYSVISIBLE 64l	//renderuje do akejkolvek vzdialenosti


/*FLAGS PRE FLEXIBLE VERTEX FORMAT*/
#define M3DFVF_XYZ 	0x00000001l
#define M3DFVF_NORMAL 	0x00000002l
#define M3DFVF_COLOR 	0x00000004l
#define M3DFVF_TEX_MASK 0x00000038l
#define M3DFVF_TEX1	0x00000008l 
#define M3DFVF_TEX2 	0x00000010l
#define M3DFVF_TEX3 	0x00000018l
#define M3DFVF_TEX4 	0x00000020l
#define M3DFVF_dalsie 	0x00000040l

/***********Pre Z sort************/
typedef struct _zsort{ 
	float Z;
	void *hMesh;
	M3DMATRIX World;
}ZSORTstruct;
ZSORTstruct zsort[MAXOBJS], zsort_first[MAXOBJS]; //pre RENDEREDFIRST
long Count_zsort=0, Count_zsort_first=0;		// ------||-------

/************RELEASE*************/
int ReleasedMesh_N;
void *ReleasedMesh[MAXOBJS];     //ked sa releasuju Meshe
int ReleasedFrame_N;
void *ReleasedFrame[MAXFRAMESS]; //ked sa releasuju Framy

/***********************/

char *FNAME="M3D v1.0";

/**************************LIGHT*******************************/
//treba doplnit
M3DVECTOR Light_Pos[8];
BOOL Light[8]={0};
/******************MATICE**********************/
M3DMATRIX IdentityMatrix(void){
	M3DMATRIX mtx;
	for(int a=0;a<4;a++){
		for(int b=0;b<4;b++){
			mtx(a,b) = (a==b) ? 1.0f : 0.0f;
		}
	}
	return mtx;
}
M3DMATRIX ZeroMatrix(void){
	M3DMATRIX mtx;
	for(int a=0;a<4;a++){
		for(int b=0;b<4;b++){
			mtx(a,b) = 0.0f;
		}
	}
	return mtx;
}
//ak je theta>0 proti smeru hod. ruciciek
M3DVECTOR POINTROTATE(M3DVECTOR B, M3DVECTOR A, M3DVECTOR os, float theta){
	M3DVECTOR P,osX,osY,BN;
	M3DVECTOR t;
	//ci nahodou bod B nelezi na osi otocenia
	M3DVECTOR test=CrossProduct(os, M3DVECTOR(B-A));
	if(test==M3DVECTOR(0.0f, 0.0f, 0.0f) || os==M3DVECTOR(0.0f, 0.0f, 0.0f)){return B;}
	//nelezi
	t=( DotProduct(os, M3DVECTOR(B-A)) )/SquareMagnitude(os);
	P=M3DVECTOR(A+M3DVECTOR(t*os) );
	osX=M3DVECTOR(B-P);
	osY=CrossProduct(Normalize(os), osX);
      BN=M3DVECTOR( M3DVECTOR(float(cos(theta))*osX)+M3DVECTOR(float(sin(theta))*osY) );
	BN=M3DVECTOR(BN+P);
	return BN;	
}
//ak je uhol>0 tak proti hodinam
M3DMATRIX POINTROTATE_MATRIX(M3DVECTOR A, M3DVECTOR os_old, float theta){
        M3DMATRIX mtx;
        M3DVECTOR os=os_old;
        float CS=(float)cos(float(theta)), SN=(float)sin(float(theta)),temp;
        //normalization
        temp=SquareMagnitude(os_old);
        if(temp==0.0f){return IdentityMatrix();}
        if(temp!=1.0f){temp=float(sqrt(temp)); os=os/temp;}

        mtx(0,0)=CS+(1-CS)*os.x*os.x;		mtx(0,1)=SN*os.z+(1-CS)*os.x*os.y;	mtx(0,2)=-SN*os.y+(1-CS)*os.x*os.z;	mtx(0,3)=0.0f;
        mtx(1,0)=-SN*os.z+(1-CS)*os.x*os.y;	mtx(1,1)=CS+(1-CS)*os.y*os.y;		mtx(1,2)=SN*os.x+(1-CS)*os.y*os.z;	mtx(1,3)=0.0f;
        mtx(2,0)=SN*os.y+(1-CS)*os.x*os.z;	mtx(2,1)=-SN*os.x+(1-CS)*os.y*os.z;	mtx(2,2)=CS+(1-CS)*os.z*os.z;		mtx(2,3)=0.0f;
        //ak A=(0,0,0), tak 3 dalsie riadky su nulove - ziadne posunutie
	  mtx(3,0)=(1-CS)*A.x-(1-CS)*os.x*os.x*A.x-(1-CS)*os.x*os.y*A.y-(1-CS)*os.x*os.z*A.z-A.y*os.z*SN+A.z*os.y*SN;
        mtx(3,1)=(1-CS)*A.y-(1-CS)*os.y*os.y*A.y-(1-CS)*os.x*os.y*A.x-(1-CS)*os.y*os.z*A.z-A.z*os.x*SN+A.x*os.z*SN;
        mtx(3,2)=(1-CS)*A.z-(1-CS)*os.z*os.z*A.z-(1-CS)*os.x*os.z*A.x-(1-CS)*os.y*os.z*A.y-A.x*os.y*SN+A.y*os.x*SN;
        mtx(3,3)=1.0f;
        return mtx;
}
//vynasobi vector(1x4, vec(0,3)=1.0f) s maticou 4x4
M3DVECTOR VxM(M3DVECTOR vec, M3DMATRIX mat){
        M3DVECTOR ret;
        ret.x=vec.x*mat(0,0)+vec.y*mat(1,0)+vec.z*mat(2,0)+mat(3,0);
        ret.y=vec.x*mat(0,1)+vec.y*mat(1,1)+vec.z*mat(2,1)+mat(3,1);
        ret.z=vec.x*mat(0,2)+vec.y*mat(1,2)+vec.z*mat(2,2)+mat(3,2);
        return ret;
}
/*******************TRANSFORMATIONS***************/
M3DMATRIX CreateProjectionMatrix( float near_plane, float far_plane, float fov_horiz, float fov_vert){
  	float    h, w, Q;
	w = float(cos(fov_horiz*0.5)/sin(fov_horiz*0.5));
	h = float(cos(fov_vert*0.5)/sin(fov_vert*0.5));
	Q = far_plane/(far_plane - near_plane);
	M3DMATRIX ret;
	ret._11 = w   ;	 ret._12 = 0.0f;	ret._13 = 0.0f;			ret._14 = 0.0f;   
	ret._21 = 0.0f;	 ret._22 = h;	ret._23 = 0.0f;			ret._24 = 0.0f;
	ret._31 = 0.0f;	 ret._32 = 0.0f;	ret._33 = -Q;			ret._34 = -1.0f; 
 	ret._41 = 0.0f;	 ret._42 = 0.0f; 	ret._43 = -Q*near_plane;		ret._44 = 0.0f;
	return ret;
}  
M3DMATRIX CreateCameraMatrix(M3DVECTOR position, M3DVECTOR orient, M3DVECTOR upworld){
	M3DMATRIX mat;
	M3DVECTOR vView;
 	vView = Normalize(-orient);
	M3DVECTOR vRight = CrossProduct(upworld, vView);
	M3DVECTOR vUp = CrossProduct(vView, vRight);
	vRight=Normalize(vRight);
	vUp=Normalize(vUp);
	mat._11 = vRight.x;  mat._12 = vUp.x;  mat._13 = vView.x;  mat._14 = 0.0f;
    	mat._21 = vRight.y;  mat._22 = vUp.y;  mat._23 = vView.y;  mat._24 = 0.0f;
    	mat._31 = vRight.z;  mat._32 = vUp.z;  mat._33 = vView.z;  mat._34 = 0.0f;
    	mat._41 = - DotProduct( position, vRight );
    	mat._42 = - DotProduct( position, vUp );
    	mat._43 = - DotProduct( position, vView );
    	mat._44 = 1.0f;
	return mat;
}
inline M3DMATRIX CreateWorldMatrix(M3DVECTOR position, M3DVECTOR orientation, M3DVECTOR up){
	M3DVECTOR osZ=Normalize(orientation);
	M3DVECTOR osX=CrossProduct(up, osZ);
	M3DVECTOR osY=CrossProduct(osZ, osX);
	osX=Normalize(osX);
	osY=Normalize(osY);
	M3DMATRIX mat=IdentityMatrix();
	mat(0,0)=osX.x;mat(1,0)=osY.x;mat(2,0)=osZ.x;mat(3,0)=position.x;
	mat(0,1)=osX.y;mat(1,1)=osY.y;mat(2,1)=osZ.y;mat(3,1)=position.y;
	mat(0,2)=osX.z;mat(1,2)=osY.z;mat(2,2)=osZ.z;mat(3,2)=position.z;
	return mat;
}
/***************************************************************/
/***************************EXTENSIONS*************************/
//Search for an extension
BOOL IsInString(char *string, const char *search) {
        int pos; int maxpos=strlen(search)-1; int len=strlen(string);
	for (int i=0; i<len; i++) {
		if ((i==0) || ((i>1) && string[i-1]=='\n')) {				// New Extension Begins Here!
			pos=0;									// Begin New Search
			while (i<=len && string[i]!='\n') {						// Search Whole Extension-String
				if (string[i]==search[pos]) pos++;				// Next Position
				if ((pos>maxpos) && string[i+1]=='\n') return true;	// We Have A Winner!
				i++;
			}
		}
	}
	return false;										// Sorry, Not Found!
}
#ifdef MULTITEXTURING
BOOL InitMultitexture(void) {
	char *extensions;
	extensions=strdup((char *) glGetString(GL_EXTENSIONS));			// Fetch Extension String
	int len=strlen(extensions);
	for (int i=0; i<len; i++){if (extensions[i]==' ') extensions[i]='\n';}

	if (__ARB_ENABLE && IsInString(extensions,"GL_ARB_multitexture") && IsInString(extensions,"GL_EXT_texture_env_combine") && IsInString(extensions,"GL_EXT_vertex_array")){
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&MaxTexelUnits);
		glMultiTexCoord1fARB = (PFNGLMULTITEXCOORD1FARBPROC) wglGetProcAddress("glMultiTexCoord1fARB");
		glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC) wglGetProcAddress("glMultiTexCoord2fARB");
		glMultiTexCoord3fARB = (PFNGLMULTITEXCOORD3FARBPROC) wglGetProcAddress("glMultiTexCoord3fARB");
		glMultiTexCoord4fARB = (PFNGLMULTITEXCOORD4FARBPROC) wglGetProcAddress("glMultiTexCoord4fARB");
		glActiveTextureARB   = (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress("glActiveTextureARB");
		glClientActiveTextureARB= (PFNGLCLIENTACTIVETEXTUREARBPROC) wglGetProcAddress("glClientActiveTextureARB");
		
		glArrayElementEXT = (PFNGLARRAYELEMENTEXTPROC) wglGetProcAddress("glArrayElementEXT");
		glColorPointerEXT = (PFNGLCOLORPOINTEREXTPROC) wglGetProcAddress("glColorPointerEXT");
		glDrawArraysEXT = (PFNGLDRAWARRAYSEXTPROC) wglGetProcAddress("glDrawArraysEXT");
		glEdgeFlagPointerEXT = (PFNGLEDGEFLAGPOINTEREXTPROC) wglGetProcAddress("glEdgeFlagPointerEXT");
		glGetPointervEXT = (PFNGLGETPOINTERVEXTPROC) wglGetProcAddress("glGetPointervEXT");
 		glIndexPointerEXT = (PFNGLINDEXPOINTEREXTPROC) wglGetProcAddress("glIndexPointerEXT");
		glNormalPointerEXT = (PFNGLNORMALPOINTEREXTPROC) wglGetProcAddress("glNormalPointerEXT");
		glTexCoordPointerEXT = (PFNGLTEXCOORDPOINTEREXTPROC) wglGetProcAddress("glTexCoordPointerEXT");
		glVertexPointerEXT = (PFNGLVERTEXPOINTEREXTPROC) wglGetProcAddress("glVertexPointerEXT");
		return TRUE;
	}
	return FALSE;
}
#endif
/**************************************************************/
/************************STRUCTURES***************************/
/************************M3DMATERIAL**************************/
typedef struct _M3DMATERIAL{
	float AR, AG, AB, AA;	//ambient
	float DR, DG, DB, DA;	//diffuse
	float SR, SG, SB, SA;	//specular
	float ER, EG, EB, EA;	//emission
	float SH;			//shininess
	float am, dm, sm;		//ambient col. index, diffuse, specular
	char Name[NAMELENGHT];
}M3DMATERIAL;
M3DMATERIAL DEFMATERIAL={0.5f, 0.5f, 0.5f, 1.0f,  0.8f, 0.8f, 0.8f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  10.0f, 0.0f, 1.0f, 1.0f, "default"};
/***********************mD3DTexture**************************/
typedef struct _mD3DTexture{
	int Width, Height;
	DWORD Text;
	char Name[NAMELENGHT];
}mD3DTexture;
/**********************mD3DMesh*****************************/
class ImD3DMesh{
    public:
	char Name[NAMELENGHT];			//nazov objektu
	DWORD Flags;				//vlastnosti
	DWORD *Data;				//pointer na pole face-ov
	DWORD VertexStruct;			//ako vyzera vertex
	long VertexSize;				//velkost vertexu v bytoch
	long VertexCount;				//pocet vertexov v obj
	float *Vertex;				//pointer na pole vertexov
	DWORD SrcBlend;				//alphablending obj
	DWORD DestBlend;				//ab obj
	int Textures;				//pocet textur
	mD3DTexture *Texture[8];		//pointre na textury
	int TextureEnvMode[8];			//sposob blendovania textur (multitext)
	DWORD TextureWrapS, TextureWrapT;	//sposob mapovania text, kde u, v je mimo <0.0f, 1.0f>
	DWORD MatFlags;				//ake farby materialu budu moct byt zapisane vo vertexoch
	M3DMATERIAL Material;			//material
	int Meshs;					//pocet poli pointrov na meshe
	int NMeshs[MAXMESHS];			//kolko jedno pole obsahuje pointrov
	ImD3DMesh *Mesh[MAXMESHS];		//pointre na polia pointrov, obsahujuce dalsie meshe
	
	HRESULT SetFlags(DWORD par){
		Flags=par;
		return MM_OK;}
	HRESULT AddMesh(int NN, ImD3DMesh *msh){
		if(msh==NULL){return MM_INVALID_PARAMS;}
		Mesh[Meshs]=msh;
		NMeshs[Meshs]=NN;
		Meshs++;
		return MM_OK;}
	HRESULT RemoveMesh(ImD3DMesh *msh);
	HRESULT SetTexture(int N, mD3DTexture *txt){
		if(txt==NULL){return MM_INVALID_PARAMS;}
		Texture[N]=txt;
		return MM_OK;}
	HRESULT SetMaterial(M3DMATERIAL mat){
		Material=mat;
		return MM_OK;}
	HRESULT SetVertex(long N, float *vertex){
		if(vertex==NULL){return MM_INVALID_PARAMS;}
		long strsize=VertexSize/sizeof(float);
		memcpy(&Vertex[N*strsize], vertex, strsize*sizeof(float));
		return MM_OK;}
	HRESULT GetVertex(long N, float *vertex){
		if(vertex==NULL){return MM_INVALID_PARAMS;}
		long strsize=VertexSize/sizeof(float);
		memcpy(vertex, &Vertex[N*strsize], strsize*sizeof(float));
		return MM_OK;}
	//najprv vsetky normaly zrata a na zaver znormalizuje
	HRESULT GenerateNormals(void);
};
HRESULT ImD3DMesh::RemoveMesh(ImD3DMesh *msh){
		if(msh==NULL){return MM_INVALID_PARAMS;}
		for(int A=Meshs-1;A>=0;A--){
			if(msh==Mesh[A]){
				for(int B=A;B<Meshs-1;B++){
					Mesh[B]=Mesh[B+1];
					NMeshs[B]=NMeshs[B+1];
				}
				Meshs--;
				return MM_OK;
			}
		}
		return MM_OK;
}
HRESULT ImD3DMesh::GenerateNormals(void){
	long strsize=VertexSize/sizeof(float);
	float *NVERTEX;
	long i,j,k,n;
	//vymazanie povodnych normal
	for(i=0;i<VertexCount;i++){
		memset(&Vertex[i*strsize+3], 0, sizeof(float)*3);
	}
	M3DVECTOR N,v0,v1,v2,Norm[MAXNFACE];
	i=0;
	float TT;
	while(Data[i]!=0){
		ZeroMemory(&Norm[0], MAXNFACE*sizeof(M3DVECTOR));
		k=i+1; n=0;
		for(j=0;j<long(Data[i]);j++){
			n++;
			if(n==1){
				NVERTEX=(float*)&Vertex[Data[i+1]*strsize]; v0.x=NVERTEX[0];v0.y=NVERTEX[1];v0.z=NVERTEX[2];
			}
			if(n==3){n=2;
				//ziskanie vertexov
				NVERTEX=(float*)&Vertex[Data[k-1]*strsize];
				v1.x=NVERTEX[0];v1.y=NVERTEX[1];v1.z=NVERTEX[2];
				NVERTEX=(float*)&Vertex[Data[k]*strsize];
				v2.x=NVERTEX[0];v2.y=NVERTEX[1];v2.z=NVERTEX[2];
				N=CrossProduct(M3DVECTOR(v1-v0), M3DVECTOR(v2-v0));
				TT=N.x*N.x+N.y*N.y+N.z*N.z;
				if(TT==0.0f){N=M3DVECTOR(0.0f,0.0f,0.0f);}
				else{N=N/float(sqrt(TT));}
				Norm[0]=M3DVECTOR(Norm[0]+N);
				Norm[j-1]=M3DVECTOR(Norm[j-1]+N);
				Norm[j]=M3DVECTOR(Norm[j]+N);
			}
			k++;
		}
		for(j=1;j<=long(Data[i]);j++){
			NVERTEX=(float*)&Vertex[Data[i+j]*strsize];
			NVERTEX[3]+=Norm[j-1].x;NVERTEX[4]+=Norm[j-1].y;NVERTEX[5]+=Norm[j-1].z;					
		}
		i=k;
	}
	//normalizovanie normal
	i=0;
	while(Data[i]!=0){
		for(j=1;j<=long(Data[i]);j++){
			NVERTEX=(float*)&Vertex[Data[i+j]*strsize];
			Norm[0].x=NVERTEX[3];Norm[0].y=NVERTEX[4];Norm[0].z=NVERTEX[5];
			TT=Norm[0].x*Norm[0].x+Norm[0].y*Norm[0].y+Norm[0].z*Norm[0].z;
			if(TT==0.0f){Norm[0]=M3DVECTOR(0.0f,0.0f,0.0f);}
			else{Norm[0]=Norm[0]/float(sqrt(TT));}
			NVERTEX[3]=Norm[0].x;NVERTEX[4]=Norm[0].y;NVERTEX[5]=Norm[0].z;					
		}
		i+=Data[i]+1;
	}
	return MM_OK;
}
class ImD3DMesh;
typedef ImD3DMesh *mD3DMesh;

HRESULT _ReleasemD3DMesh(mD3DMesh mesh, BOOL DeleteSubMeshes){
	if(mesh==NULL){return MM_INVALID_PARAMS;}
	int i, j;
	j=0;
	for(i=0;i<ReleasedMesh_N;i++){
		if(ReleasedMesh[i]==(void*)mesh){j++;}
	}
	if(j!=0){return MM_OK;}
	ReleasedMesh[ReleasedMesh_N]=(void*)mesh;
	ReleasedMesh_N++;
	if(DeleteSubMeshes==1){
		for(i=0;i<mesh->Meshs;i++){
			ImD3DMesh *MM=(ImD3DMesh*)mesh->Mesh[i];
			for(int j=0;j<mesh->NMeshs[i];j++){
				ATTEMPT(_ReleasemD3DMesh(&MM[j], 1));
			}
		}
	}
	return MM_OK;
}
HRESULT ReleasemD3DMesh(mD3DMesh &mesh, BOOL DeleteSubMeshes){
	if(mesh==NULL){return MM_INVALID_PARAMS;}
	ReleasedMesh_N=0;
	ReleasedMesh[ReleasedMesh_N]=(void*)mesh;
	ReleasedMesh_N++;
	int i;
	if(DeleteSubMeshes==1){
		for(i=0;i<mesh->Meshs;i++){
			ImD3DMesh *MM=(ImD3DMesh*)mesh->Mesh[i];
			for(int j=0;j<mesh->NMeshs[i];j++){
				ATTEMPT(_ReleasemD3DMesh(&MM[j], 1));
			}
		}
	}
	for(i=0;i<ReleasedMesh_N;i++){
		free(mD3DMesh(ReleasedMesh[i])->Data);
		free(mD3DMesh(ReleasedMesh[i])->Vertex);
		free(mD3DMesh(ReleasedMesh[i]));
	}
	ReleasedMesh_N=0;
	mesh=NULL;
	return MM_OK;
}
HRESULT CreatemD3DMesh(DWORD VertexType, void* Vertex, DWORD* Data, mD3DMesh* Mesh){
	if(Mesh==NULL){return MM_INVALID_PARAMS;}
	long strsize=0;
	if((VertexType&M3DFVF_XYZ)==M3DFVF_XYZ){strsize+=sizeof(float)*3;}
	if((VertexType&M3DFVF_NORMAL)==M3DFVF_NORMAL){strsize+=sizeof(float)*3;}
	if((VertexType&M3DFVF_COLOR)==M3DFVF_COLOR){strsize+=sizeof(float)*4;}
	if((VertexType&M3DFVF_TEX_MASK)==M3DFVF_TEX1){strsize+=sizeof(float)*2;}
	else if((VertexType&M3DFVF_TEX_MASK)==M3DFVF_TEX2){strsize+=sizeof(float)*4;}
	else if((VertexType&M3DFVF_TEX_MASK)==M3DFVF_TEX3){strsize+=sizeof(float)*6;}
	else if((VertexType&M3DFVF_TEX_MASK)==M3DFVF_TEX4){strsize+=sizeof(float)*8;}
	mD3DMesh tmesh;
	tmesh=(mD3DMesh)malloc(sizeof(ImD3DMesh));
	if(tmesh==NULL){return MM_OUT_OF_MEMORY;}
	ZeroMemory(tmesh, sizeof(ImD3DMesh));
	tmesh->VertexSize=strsize;
	tmesh->Flags=0;
	tmesh->Textures=0;
	tmesh->SrcBlend=GL_SRC_ALPHA;
	tmesh->DestBlend=GL_ONE_MINUS_SRC_ALPHA;
	tmesh->TextureWrapS=GL_REPEAT;
	tmesh->TextureWrapT=GL_REPEAT;
	int q;
	tmesh->Meshs=0;
	for(q=0;q<NAMELENGHT;q++){
		tmesh->Name[q]=0;
	}
	for(q=0;q<8;q++){
		tmesh->TextureEnvMode[q]=GL_MODULATE;
	}
	tmesh->VertexCount=0;
	if(Data!=NULL){
                DWORD i=0, j, Vnum=0;
		while(Data[i]!=0){
			//zistenie VertexCountu podla poslanych dat
			j=i+1;
			i=Data[i]+i+1;
			for(j;j<i;j++){	
				if(Vnum<Data[j]){Vnum=Data[j];}
			}
		}
		tmesh->VertexCount=Vnum+1;
		i++;
		tmesh->Data=(DWORD*)malloc(sizeof(DWORD)*i);
		if(tmesh->Data==NULL){return MM_OUT_OF_MEMORY;}
		memcpy(tmesh->Data, Data, sizeof(DWORD)*i);
	}
	else{tmesh->Data=NULL;}
	tmesh->VertexStruct=VertexType;
	if((Vertex!=NULL)&&(tmesh->VertexCount>0)){
		tmesh->Vertex=(float*)malloc(strsize*tmesh->VertexCount);
		if(tmesh->Vertex==NULL){return MM_OUT_OF_MEMORY;}
		memcpy(tmesh->Vertex, Vertex, strsize*tmesh->VertexCount);
	}
	else{tmesh->Vertex=NULL;}
	tmesh->MatFlags=GL_DIFFUSE;
	tmesh->Material=DEFMATERIAL;

	memcpy(Mesh, &tmesh, sizeof(mD3DMesh));
	return MM_OK;
}
//prenastavi mesh bez jeho noveho alokovania
HRESULT SetmD3DMesh(DWORD VertexType, void* Vertex, DWORD* Data, mD3DMesh Mesh){
	if(Mesh==NULL){return MM_INVALID_PARAMS;}
	long strsize=0;
	if((VertexType&M3DFVF_XYZ)==M3DFVF_XYZ){strsize+=sizeof(float)*3;}
	if((VertexType&M3DFVF_NORMAL)==M3DFVF_NORMAL){strsize+=sizeof(float)*3;}
	if((VertexType&M3DFVF_COLOR)==M3DFVF_COLOR){strsize+=sizeof(float)*4;}
	if((VertexType&M3DFVF_TEX_MASK)==M3DFVF_TEX1){strsize+=sizeof(float)*2;}
	else if((VertexType&M3DFVF_TEX_MASK)==M3DFVF_TEX2){strsize+=sizeof(float)*4;}
	else if((VertexType&M3DFVF_TEX_MASK)==M3DFVF_TEX3){strsize+=sizeof(float)*6;}
	else if((VertexType&M3DFVF_TEX_MASK)==M3DFVF_TEX4){strsize+=sizeof(float)*8;}
	mD3DMesh tmesh=Mesh;
	if(tmesh->VertexCount>0){
		free(tmesh->Data);
		free(tmesh->Vertex);
	}
	ZeroMemory(tmesh, sizeof(ImD3DMesh));
	tmesh->VertexSize=strsize;
	tmesh->Flags=0;
	tmesh->Textures=0;
	tmesh->SrcBlend=GL_SRC_ALPHA;
	tmesh->DestBlend=GL_ONE_MINUS_SRC_ALPHA;
	tmesh->TextureWrapS=GL_REPEAT;
	tmesh->TextureWrapT=GL_REPEAT;
	int q;
	tmesh->Meshs=0;
	for(q=0;q<NAMELENGHT;q++){
		tmesh->Name[q]=0;
	}
	for(q=0;q<8;q++){
		tmesh->TextureEnvMode[q]=GL_MODULATE;
	}
	tmesh->VertexCount=0;
	if(Data!=NULL){
                DWORD i=0, j, Vnum=0;
		while(Data[i]!=0){
			//zistenie VertexCountu podla poslanych dat
			j=i+1;
			i=Data[i]+i+1;
			for(j;j<i;j++){	

				if(Vnum<Data[j]){Vnum=Data[j];}
			}
		}
		tmesh->VertexCount=Vnum+1;
		i++;
		tmesh->Data=(DWORD*)malloc(sizeof(DWORD)*i);
		if(tmesh->Data==NULL){return MM_OUT_OF_MEMORY;}
		memcpy(tmesh->Data, Data, sizeof(DWORD)*i);
	}
	else{tmesh->Data=NULL;}
	tmesh->VertexStruct=VertexType;
	if((Vertex!=NULL)&&(tmesh->VertexCount>0)){
		tmesh->Vertex=(float*)malloc(strsize*tmesh->VertexCount);
		if(tmesh->Vertex==NULL){return MM_OUT_OF_MEMORY;}
		memcpy(tmesh->Vertex, Vertex, strsize*tmesh->VertexCount);
	}
	else{tmesh->Vertex=NULL;}
	tmesh->MatFlags=GL_DIFFUSE;
	tmesh->Material=DEFMATERIAL;
	return MM_OK;
}
/***************************************************************/
/**************************mD3DFrame****************************/
class ImD3DFrame{
   public:
	M3DVECTOR Position;			//poloha
	M3DVECTOR Orientation;			//orientacia (os-z v obj)
	M3DVECTOR Up;				//(os-y v obj)
	M3DMATRIX world;				//matica vytvorena z Pos, Or, Up
	int Frames;					//pocet poli pointrov na dalsie frame-y
	int NFrames[MAXFRAMES];			//pocet framov v 1 takom poli			
	ImD3DFrame* Frame[MAXFRAMES];		//pointre na taketo polia 
	int Meshs;					//
	int NMeshs[MAXMESHS];			//pozri mD3DMesh
	mD3DMesh Mesh[MAXMESHS];		//
	HRESULT SetWorld(void){
		world=CreateWorldMatrix(Position,Orientation,Up);
		return MM_OK;}
	HRESULT AddFrame(int NN, ImD3DFrame* Fr){
		if(Fr==NULL){return MM_INVALID_PARAMS;}
		Frame[Frames]=Fr;
		NFrames[Frames]=NN;
		Frames++;
		return MM_OK;}
	HRESULT RemoveFrame(ImD3DFrame* Fr);
	HRESULT AddMesh(int NN, ImD3DMesh *msh){
		if(msh==NULL){return MM_INVALID_PARAMS;}
		Mesh[Meshs]=msh;
		NMeshs[Meshs]=NN;
		Meshs++;
		return MM_OK;}
	HRESULT RemoveMesh(ImD3DMesh *msh);
};
HRESULT ImD3DFrame::RemoveFrame(ImD3DFrame* Fr){
	if(Fr==NULL){return MM_INVALID_PARAMS;}
	for(int A=Frames-1;A>=0;A--){
		if(Fr==Frame[A]){
			for(int B=A;B<Frames-1;B++){
				Frame[B]=Frame[B+1];
				NFrames[B]=NFrames[B+1];
			}
			Frames--;
			return MM_OK;
		}
	}
	return MM_UNSUCCESFUL;
}
HRESULT ImD3DFrame::RemoveMesh(ImD3DMesh *msh){
	if(msh==NULL){return MM_INVALID_PARAMS;}
	for(int A=Meshs-1;A>=0;A--){
		if(msh==Mesh[A]){
			for(int B=A;B<Meshs-1;B++){
				Mesh[B]=Mesh[B+1];
				NMeshs[B]=NMeshs[B+1];
			}
			Meshs--;
			return MM_OK;
		}
	}
	return MM_UNSUCCESFUL;
}
class ImD3DFrame;
typedef ImD3DFrame *mD3DFrame;

HRESULT _ReleasemD3DFrame(mD3DFrame frame, BOOL DeleteSubMF, BOOL ReleaseOBJ){
	int i,j;
	if(frame==NULL){return MM_INVALID_PARAMS;}
	if(ReleaseOBJ==1){
		j=0;
		for(i=0;i<ReleasedFrame_N;i++){
			if(ReleasedFrame[i]==(void*)frame){j++;}
		}
		if(j!=0){return MM_OK;}
		ReleasedFrame[ReleasedFrame_N]=(void*)frame;
		ReleasedFrame_N++;
	}
	if(DeleteSubMF==1){
		for(i=0;i<frame->Frames;i++){
			ImD3DFrame *FF=(ImD3DFrame*)frame->Frame[i];
			for(j=0;j<frame->NFrames[i];j++){
				ATTEMPT(_ReleasemD3DFrame(&FF[j],1,ReleaseOBJ));
			}
		}
		for(i=0;i<frame->Meshs;i++){
			ImD3DMesh *MM=(ImD3DMesh*)frame->Mesh[i];
			for(j=0;j<frame->NMeshs[i];j++){
				ATTEMPT(_ReleasemD3DMesh(&MM[j],1));
			}
		}
	}
	return MM_OK;
}
HRESULT ReleasemD3DFrame(mD3DFrame &frame, BOOL DeleteSubMF){
	if(frame==NULL){return MM_INVALID_PARAMS;}
	ReleasedMesh_N=0;	ReleasedFrame_N=0;
	ReleasedFrame[ReleasedFrame_N]=(void*)frame;
	ReleasedFrame_N++;
	
	int i,j;
	if(DeleteSubMF==1){
		for(i=0;i<frame->Frames;i++){
			ImD3DFrame *FF=(ImD3DFrame*)frame->Frame[i];
			for(j=0;j<frame->NFrames[i];j++){
				if(j==0){ATTEMPT(_ReleasemD3DFrame(&FF[j],1,1));}
				else{ATTEMPT(_ReleasemD3DFrame(&FF[j],1,0));}
			}
		}
		for(i=0;i<frame->Meshs;i++){
			ImD3DMesh *MM=(ImD3DMesh*)frame->Mesh[i];
			for(j=0;j<frame->NMeshs[i];j++){
				ATTEMPT(_ReleasemD3DMesh(&MM[j],1));
				ATTEMPT(_ReleasemD3DMesh(&MM[j],1));
			}
		}
	}
	for(i=0;i<ReleasedFrame_N;i++){
		free(mD3DFrame(ReleasedFrame[i]));
	}
	ReleasedFrame_N=0;
	for(i=0;i<ReleasedMesh_N;i++){
		free(mD3DMesh(ReleasedMesh[i])->Data);
		free(mD3DMesh(ReleasedMesh[i])->Vertex);
		free(mD3DMesh(ReleasedMesh[i]));
	}
	ReleasedMesh_N=0;
	frame=NULL;
	return MM_OK;
}
HRESULT CreatemD3DFrame(mD3DFrame Parent, mD3DFrame* Child){
	if(Child==NULL){return MM_INVALID_PARAMS;}
	mD3DFrame C;
	C=(mD3DFrame)malloc(sizeof(ImD3DFrame));
	if(C==NULL){return MM_OUT_OF_MEMORY;}
	ZeroMemory(C, sizeof(ImD3DFrame));
	if(Parent!=NULL){
		Parent->Frame[Parent->Frames]=C;
		Parent->NFrames[Parent->Frames]=1;
		Parent->Frames++;
	}
	C->Frames=0;C->Meshs=0;
	C->Position=M3DVECTOR(0.0f,0.0f,0.0f);
	C->Orientation=M3DVECTOR(0.0f,0.0f,1.0f);
	C->Up=M3DVECTOR(0.0f,1.0f,0.0f);
	C->world=CreateWorldMatrix(C->Position,C->Orientation,C->Up);
	memcpy(Child, &C, sizeof(mD3DFrame));
	return MM_OK;
}
HRESULT SetmD3DFrame(mD3DFrame Parent, mD3DFrame Child){
	if(Child==NULL){return MM_INVALID_PARAMS;}
	mD3DFrame C=Child;
	ZeroMemory(C, sizeof(ImD3DFrame));
	if(Parent!=NULL){
		Parent->Frame[Parent->Frames]=C;
		Parent->NFrames[Parent->Frames]=1;
		Parent->Frames++;
	}
	C->Frames=0;C->Meshs=0;
	C->Position=M3DVECTOR(0.0f,0.0f,0.0f);
	C->Orientation=M3DVECTOR(0.0f,0.0f,1.0f);
	C->Up=M3DVECTOR(0.0f,1.0f,0.0f);
	C->world=CreateWorldMatrix(C->Position,C->Orientation,C->Up);
	return MM_OK;
}
/****************************************************************/
/************Copyruje rect z FRAMEBUFFERU do textury****************/
HRESULT CopyFBtoTX(DWORD N, int x, int y, int Width, int Height, mD3DTexture *Text, BOOL Mip){
	if(Text==NULL){return MM_INVALID_PARAMS;}
	//void PixelStore( enum pname, T param );
	//void PixelTransfer( enum param, T value );
	//void PixelMap( enum map, sizei size, Tvalues );
	glDeleteTextures ( 1, (GLuint*)&Text->Text);
	//uprava na mocninu 2
	int xW, xH;
	for(xW=1;Width>xW;xW<<=1);
	for(xH=1;Height>xH;xH<<=1);
	if(xW>xH){xH = xW;}
	else{xW=xH;}
	Width=xW; Height=xH;

	BYTE *_pixels, *pixels;
	_pixels=(BYTE*)malloc(Width*Height*4);
	if(_pixels==NULL){return MM_OUT_OF_MEMORY;}
	pixels=(BYTE*)malloc(Width*Height*4);
	if(pixels==NULL){return MM_OUT_OF_MEMORY;}
	glReadPixels(x, y, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, _pixels);
	int yy;
	for(yy=0;yy<Height;yy++){
		memcpy(&pixels[yy*Width*4], &_pixels[(Height-yy)*Width*4], Width*4); 
	}

	Text->Text=N;
	glBindTexture(GL_TEXTURE_2D, Text->Text);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, TEXT_MAG_FILTER);
	if(Mip==1){
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, TEXT_MIN_FILTER_MIP);
		gluBuild2DMipmaps(GL_TEXTURE_2D, TEXTF, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}
	else{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, TEXT_MIN_FILTER);
		glTexImage2D(GL_TEXTURE_2D, 0, TEXTF, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}
	strcpy(&Text->Name[0], "SphereMap\n");

	free(pixels);
	free(_pixels);
	return MM_OK;
}


/***************************************************************/
/***************************RENDER******************************/
/***************************************************************/

mD3DFrame Lastscene;
BOOL _noculling=0;	//aby netrebalo stale volat GL funkcie
M3DMATERIAL _LMat={-1.0f, -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, ""};
HRESULT _RenderMesh(mD3DMesh mesh, M3DMATRIX world, BOOL rendersubmesh){
	if((mesh->Flags&MD3DMESHF_DISABLED)==MD3DMESHF_DISABLED){return MM_OK;}

/**********************EFFECTY ktore nevyzaduju ZSORT*********************/
	M3DVECTOR normal,N, Nn,R,E,P;	//pre sphermap
	M3DVECTOR center=M3DVECTOR(0.0f,0.0f,0.0f);
	long strsize=mesh->VertexSize;
	for(long A=0;A<mesh->VertexCount;A++){
		float *bod=(float*)&mesh->Vertex[A*strsize/sizeof(float)];

		center.x=center.x+bod[0];
		center.y=center.y+bod[1];
		center.z=center.z+bod[2];
		/*SPHERICAL ENVIRONMENTAL MAPPING*/
		if((mesh->VertexStruct&M3DFVF_NORMAL)&&(mesh->Textures>0)&&( (mesh->Flags&MD3DMESHF_SPHERMAP)==MD3DMESHF_SPHERMAP)){
			normal.x=bod[3];normal.y=bod[4];normal.z=bod[5];
			N.x=normal.x*world._11+normal.y*world._21+normal.z*world._31;
			N.y=normal.x*world._12+normal.y*world._22+normal.z*world._32;
			N.z=normal.x*world._13+normal.y*world._23+normal.z*world._33;
			P.x=bod[0]*world._11+bod[1]*world._21+bod[2]*world._31+world._41;
			P.y=bod[0]*world._12+bod[1]*world._22+bod[2]*world._32+world._42;
			P.z=bod[0]*world._13+bod[1]*world._23+bod[2]*world._33+world._43;
			E=CameraPosition-P;
			//normalize
			float TT=E.x*E.x+E.y*E.y+E.z*E.z;
			if(TT==0){E=M3DVECTOR(0.0f,0.0f,0.0f);}
			else{TT=float(sqrt(float(TT))); E=E/TT;}
			
			R=2*DotProduct(E,N)*N-E;
			//preventivne
			if(R.x>1.0f){R.x=1.0f;} if(R.x<-1.0f){R.x=-1.0f;}
			if(R.y>1.0f){R.y=1.0f;} if(R.y<-1.0f){R.y=-1.0f;}
			//U, V
			bod[6]=float(acos(R.x)/PI);
			bod[7]=0.5f-float(asin(R.y)/PI);
		}
		/*SPHERICAL ENVIRONMENTAL MAPPING II*/
		if((mesh->VertexStruct&M3DFVF_NORMAL)&&(mesh->Textures>1)&&( (mesh->Flags&MD3DMESHF_SPHERMAP2)==MD3DMESHF_SPHERMAP2)){
			normal.x=bod[3];normal.y=bod[4];normal.z=bod[5];
			N.x=normal.x*world._11+normal.y*world._21+normal.z*world._31;
			N.y=normal.x*world._12+normal.y*world._22+normal.z*world._32;
			N.z=normal.x*world._13+normal.y*world._23+normal.z*world._33;
			P.x=bod[0]*world._11+bod[1]*world._21+bod[2]*world._31+world._41;
			P.y=bod[0]*world._12+bod[1]*world._22+bod[2]*world._32+world._42;
			P.z=bod[0]*world._13+bod[1]*world._23+bod[2]*world._33+world._43;
			E=CameraPosition-P;
			//normalize
			float TT=E.x*E.x+E.y*E.y+E.z*E.z;
			if(TT==0){E=M3DVECTOR(0.0f,0.0f,0.0f);}
			else{TT=float(sqrt(float(TT))); E=E/TT;}
			R=2*DotProduct(E,N)*N-E;
			//preventivne
			if(R.x>1.0f){R.x=1.0f;} if(R.x<-1.0f){R.x=-1.0f;}
			if(R.y>1.0f){R.y=1.0f;} if(R.y<-1.0f){R.y=-1.0f;}
			//U, V
			bod[8]=float(acos(R.x)/PI);
			bod[9]=0.5f-float(asin(R.y)/PI);
		}
	}
/**********************EFFECTY ktore nevyzaduju ZSORT - KONIEC*********************/

	glPushMatrix();
	glLoadMatrixf(&ViewMatrix._11);
	M4DVECTOR _V4;
	for(int q=0;q<8;q++){
		if(Light[q]==1){_V4=M4DVECTOR(Light_Pos[q]); glLightfv( GL_LIGHT0+q,GL_POSITION, &_V4.x);}
	}
	glMultMatrixf(&world._11);
	
	long i,j;
/*ALPHA*/
/*TEXTURES*/
	/*MANAGMENT*/
	/***********/
/*MATERIAL*/
	glColorMaterial ( GL_FRONT_AND_BACK, mesh->MatFlags);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &mesh->Material.AR);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &mesh->Material.DR);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &mesh->Material.SR);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &mesh->Material.ER);
	if(_LMat.SH!=mesh->Material.SH){glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &mesh->Material.SH);}
	glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, &mesh->Material.am);
        _LMat=mesh->Material;
	glBlendFunc(mesh->SrcBlend, mesh->DestBlend);
	//CULLING
	if((_noculling==0) && ((mesh->Flags&MD3DMESHF_NOCULLING)==MD3DMESHF_NOCULLING)){glDisable(GL_CULL_FACE);_noculling=1;}
	else if((_noculling==1) && ((mesh->Flags&MD3DMESHF_NOCULLING)!=MD3DMESHF_NOCULLING)){glEnable( GL_CULL_FACE );glCullFace(GL_BACK);_noculling=0;}

	if(mesh->MatFlags==GL_AMBIENT_AND_DIFFUSE){glColor4f(mesh->Material.DR, mesh->Material.DG, mesh->Material.DB, mesh->Material.DA);}
	else if(mesh->MatFlags==GL_AMBIENT){glColor4f(mesh->Material.AR, mesh->Material.AG, mesh->Material.AB, mesh->Material.AA);}
	else if(mesh->MatFlags==GL_DIFFUSE){glColor4f(mesh->Material.DR, mesh->Material.DG, mesh->Material.DB, mesh->Material.DA);}
	else if(mesh->MatFlags==GL_SPECULAR){glColor4f(mesh->Material.SR, mesh->Material.SG, mesh->Material.SB, mesh->Material.SA);}
	else if(mesh->MatFlags==GL_EMISSION){glColor4f(mesh->Material.ER, mesh->Material.EG, mesh->Material.EB, mesh->Material.EA);}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mesh->TextureWrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mesh->TextureWrapT);
	if(MULTI_TEXTURE_SUPPORTED==1){
		int i;
		for(i=0;((i<mesh->Textures)&&(i<MaxTexelUnits));i++){
			glActiveTextureARB(GL_TEXTURE0_ARB+i);	
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mesh->TextureEnvMode[i]);
			glBindTexture(GL_TEXTURE_2D, mesh->Texture[i]->Text);		
			glEnable(GL_TEXTURE_2D);
		}
		for(i;i<MaxTexelUnits;i++){
			glActiveTextureARB(GL_TEXTURE0_ARB+i);	
			glDisable(GL_TEXTURE_2D);
		}
	}
	else{
		if(mesh->Textures>0){glBindTexture(GL_TEXTURE_2D, mesh->Texture[0]->Text); glEnable(GL_TEXTURE_2D);}
		else{glDisable(GL_TEXTURE_2D);}
	}
	int posun=0;
	if(MULTI_TEXTURE_SUPPORTED==1){

//pada to!!!!	
/*		if((mesh->VertexCount>0)&&((mesh->VertexStruct&M3DFVF_XYZ)==M3DFVF_XYZ)){
			glEnableClientState(GL_VERTEX_ARRAY_EXT);
			glVertexPointerEXT(3, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
			posun+=3;
			if((mesh->VertexStruct&M3DFVF_NORMAL)==M3DFVF_NORMAL){
				glEnableClientState(GL_NORMAL_ARRAY_EXT);
				glNormalPointerEXT(GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=3;
			}
			else{glDisableClientState(GL_NORMAL_ARRAY_EXT);}
			if((mesh->VertexStruct&M3DFVF_TEX_MASK)==M3DFVF_TEX1){	
				glClientActiveTextureARB(GL_TEXTURE0_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;}
			else if((mesh->VertexStruct&M3DFVF_TEX_MASK)==M3DFVF_TEX2){
				glClientActiveTextureARB(GL_TEXTURE0_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;
				glClientActiveTextureARB(GL_TEXTURE1_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;}	
			else if((mesh->VertexStruct&M3DFVF_TEX_MASK)==M3DFVF_TEX3){
				glClientActiveTextureARB(GL_TEXTURE0_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;
				glClientActiveTextureARB(GL_TEXTURE1_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;
				glClientActiveTextureARB(GL_TEXTURE2_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;}	
			else if((mesh->VertexStruct&M3DFVF_TEX_MASK)==M3DFVF_TEX4){
				glClientActiveTextureARB(GL_TEXTURE0_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;
				glClientActiveTextureARB(GL_TEXTURE1_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;
				glClientActiveTextureARB(GL_TEXTURE2_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;
				glClientActiveTextureARB(GL_TEXTURE3_ARB);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY_EXT);
				glTexCoordPointerEXT(2, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
				posun+=2;}	
			else{glDisableClientState(GL_TEXTURE_COORD_ARRAY_EXT);}
			if((mesh->VertexStruct&M3DFVF_COLOR)==M3DFVF_COLOR){
				glEnableClientState(GL_COLOR_ARRAY_EXT);
				glColorPointerEXT(4, GL_FLOAT, mesh->VertexSize, 0, &mesh->Vertex[posun]);
                                //posun+=4;
			}
			else{glDisableClientState(GL_COLOR_ARRAY_EXT);}
			i=0;
			while(mesh->Data[i]!=0){
				glDrawElements(GL_TRIANGLE_FAN, mesh->Data[i], GL_UNSIGNED_INT, &mesh->Data[i+1]);
				i=i+mesh->Data[i]+1;
			}	
		}else{glDisableClientState(GL_VERTEX_ARRAY_EXT);}

*/
//Nepada	
int ii=0;
while(mesh->Data[ii]!=0){
	float *Vertex=mesh->Vertex;
	int Size=mesh->VertexSize;
	glBegin(GL_TRIANGLE_FAN);
	for(int k=ii+1;k<=ii+long(mesh->Data[ii]);k++){
		posun=3;
		if((mesh->VertexStruct&M3DFVF_NORMAL)==M3DFVF_NORMAL){glNormal3f( Vertex[mesh->Data[k]*(Size/sizeof(float))+posun], Vertex[mesh->Data[k]*(Size/sizeof(float))+1+posun], Vertex[mesh->Data[k]*(Size/sizeof(float))+2+posun]); posun+=3; }
		if((mesh->VertexStruct&M3DFVF_TEX1)==M3DFVF_TEX1){
				glMultiTexCoord2fARB(GL_TEXTURE0_ARB,Vertex[mesh->Data[k]*(Size/sizeof(float))+posun], Vertex[mesh->Data[k]*(Size/sizeof(float))+1+posun]); posun+=2;
		}
		else if((mesh->VertexStruct&M3DFVF_TEX2)==M3DFVF_TEX2){
			glMultiTexCoord2fARB(GL_TEXTURE0_ARB,Vertex[mesh->Data[k]*(Size/sizeof(float))+posun], Vertex[mesh->Data[k]*(Size/sizeof(float))+1+posun]); posun+=2;
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB,Vertex[mesh->Data[k]*(Size/sizeof(float))+posun], Vertex[mesh->Data[k]*(Size/sizeof(float))+1+posun]); posun+=2;
		}
		if((mesh->VertexStruct&M3DFVF_COLOR)==M3DFVF_COLOR){glColor4f( Vertex[mesh->Data[k]*(Size/sizeof(float))+posun], Vertex[mesh->Data[k]*(Size/sizeof(float))+1+posun], Vertex[mesh->Data[k]*(Size/sizeof(float))+2+posun], Vertex[mesh->Data[k]*(Size/sizeof(float))+3+posun]); posun+=4;}
			glVertex3f( Vertex[mesh->Data[k]*(Size/sizeof(float))],Vertex[mesh->Data[k]*(Size/sizeof(float))+1], Vertex[mesh->Data[k]*(Size/sizeof(float))+2]);
		}
	glEnd();
	ii=ii+mesh->Data[ii]+1;
}

	}
	else{
		if((mesh->VertexCount>0)&&((mesh->VertexStruct&M3DFVF_XYZ)==M3DFVF_XYZ)){
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, mesh->VertexSize, &mesh->Vertex[posun]);
			posun+=3;
			if((mesh->VertexStruct&M3DFVF_NORMAL)==M3DFVF_NORMAL){
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT, mesh->VertexSize, &mesh->Vertex[posun]);
				posun+=3;
			}
			else{glDisableClientState(GL_NORMAL_ARRAY);}
			if((mesh->VertexStruct&M3DFVF_TEX_MASK)==M3DFVF_TEX1){	
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, mesh->VertexSize, &mesh->Vertex[posun]);
				posun+=2;}
			else if((mesh->VertexStruct&M3DFVF_TEX_MASK)==M3DFVF_TEX2){
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, mesh->VertexSize, &mesh->Vertex[posun]);
				posun+=2;}	
			else if((mesh->VertexStruct&M3DFVF_TEX_MASK)==M3DFVF_TEX3){
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, mesh->VertexSize, &mesh->Vertex[posun]);
				posun+=2;}	
			else if((mesh->VertexStruct&M3DFVF_TEX_MASK)==M3DFVF_TEX4){
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, mesh->VertexSize, &mesh->Vertex[posun]);
				posun+=2;}	
			else{glDisableClientState(GL_TEXTURE_COORD_ARRAY);}
			if((mesh->VertexStruct&M3DFVF_COLOR)==M3DFVF_COLOR){
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(4, GL_FLOAT, mesh->VertexSize, &mesh->Vertex[posun]);
                                //posun+=4;
			}
			else{glDisableClientState(GL_COLOR_ARRAY);}
			i=0;
			while(mesh->Data[i]!=0){
				glDrawElements(GL_TRIANGLE_FAN, mesh->Data[i], GL_UNSIGNED_INT, &mesh->Data[i+1]);
				i=i+mesh->Data[i]+1;
			}		
		}else{glDisableClientState(GL_VERTEX_ARRAY);}
	}
	//disable textures
	if(MULTI_TEXTURE_SUPPORTED==1){
		for(int i=0;i<MaxTexelUnits;i++){
			glActiveTextureARB(GL_TEXTURE0_ARB+i);	
			glDisable(GL_TEXTURE_2D);
		}
	}
	else{
		for(int i=0;i<MaxTexelUnits;i++){
			glDisable(GL_TEXTURE_2D);
		}
	}
	glPopMatrix();
	glFlush();
/***********************************/
	//ci renderovat dalsie meshe
	if(rendersubmesh==1){
		mD3DMesh tmesh;
		for(i=0;i<mesh->Meshs;i++){
			tmesh=(mD3DMesh)mesh->Mesh[i];
			for(j=0;j<mesh->NMeshs[i];j++){
                                ATTEMPT(_RenderMesh(&tmesh[j], world, 1));
			}
		}	
	}
	return MM_OK;
}

/*******************************************************/
HRESULT RenderFrame(mD3DFrame frame);
/************************ZSORT**************************/

int DSMapx, DSMapy;	//kolko ich bolo renderovanych pre DSPHERICAL MAPPING
/*Zoradene od najvzdialenejsieho*/
HRESULT _RenderMesh_Zsort(mD3DMesh mesh, M3DMATRIX world){
  	if((mesh->Flags&MD3DMESHF_DISABLED)==MD3DMESHF_DISABLED){return MM_OK;}
		
	M3DVECTOR normal,N, Nn,R,E,P;	//pre sphermap
	M3DVECTOR center=M3DVECTOR(0.0f,0.0f,0.0f);
	/*view trans*/
	M3DMATRIX view=ViewMatrix;
	
	long strsize=mesh->VertexSize;
	
	if(mesh->VertexCount>0){
		center=center/float(mesh->VertexCount);
		M3DVECTOR transcenter;//transformed center in to world coor.
		transcenter.x=center.x*world._11+center.y*world._21+center.z*world._31+world._41;
		transcenter.y=center.x*world._12+center.y*world._22+center.z*world._32+world._42;
		transcenter.z=center.x*world._13+center.y*world._23+center.z*world._33+world._43;
		center.x=transcenter.x*view._11+transcenter.y*view._21+transcenter.z*view._31+view._41;
		center.y=transcenter.x*view._12+transcenter.y*view._22+transcenter.z*view._32+view._42;
		center.z=transcenter.x*view._13+transcenter.y*view._23+transcenter.z*view._33+view._43;

		if( (mesh->Flags&MD3DMESHF_RENDEREDFIRST) ==MD3DMESHF_RENDEREDFIRST){
			zsort_first[Count_zsort_first].Z=center.z;
			zsort_first[Count_zsort_first].World=world;
			zsort_first[Count_zsort_first].hMesh=(void*)mesh;
			Count_zsort_first++;
		}
		else{
			zsort[Count_zsort].Z=center.z;
			zsort[Count_zsort].World=world;
			zsort[Count_zsort].hMesh=(void*)mesh;
			Count_zsort++;
		}
	}

	if( ((center.z<=0.0f) && ( ((mesh->Flags&MD3DMESHF_ALWAYSVISIBLE)==MD3DMESHF_ALWAYSVISIBLE) || (VIEWDIST1==0.0f) || (center.z>VIEWDIST1) )) || ((center.z>0.0f) && ( ((mesh->Flags&MD3DMESHF_ALWAYSVISIBLE)==MD3DMESHF_ALWAYSVISIBLE) || (VIEWDIST2==0.0f) || (center.z<VIEWDIST2) )) ){
		for(long A=0;A<mesh->VertexCount;A++){
			float *bod=(float*)&mesh->Vertex[A*strsize/sizeof(float)];	

			center.x=center.x+bod[0];
			center.y=center.y+bod[1];
			center.z=center.z+bod[2];
			/*ENV MAPPING - DYNAMIC*/
			if((mesh->VertexStruct&M3DFVF_NORMAL)&&(mesh->Textures>0)&&( (mesh->Flags&MD3DMESHF_DSPHERMAP)==MD3DMESHF_DSPHERMAP)){
				normal.x=bod[3];normal.y=bod[4];normal.z=bod[5];
				N.x=normal.x*world._11+normal.y*world._21+normal.z*world._31;
				N.y=normal.x*world._12+normal.y*world._22+normal.z*world._32;
				N.z=normal.x*world._13+normal.y*world._23+normal.z*world._33;
			
				Nn.x=N.x*ViewMatrix._11+N.y*ViewMatrix._21+N.z*ViewMatrix._31;
				Nn.y=N.x*ViewMatrix._12+N.y*ViewMatrix._22+N.z*ViewMatrix._32;
				Nn.z=N.x*ViewMatrix._13+N.y*ViewMatrix._23+N.z*ViewMatrix._33;
				//U, V
				bod[6]=0.5f-Nn.x/2.0f;
				bod[7]=0.5f-Nn.y/2.0f;
			}
			/*********************/
		}

		/*DSPHEREMAP*/
		if((mesh->VertexStruct&M3DFVF_NORMAL)&&(mesh->Textures>0)&&( (mesh->Flags&MD3DMESHF_DSPHERMAP)==MD3DMESHF_DSPHERMAP)){
			M3DVECTOR CP=CameraPosition, CO=CameraOrientation, CU=CameraUp;
			CameraPosition=M3DVECTOR(world._41, world._42, world._43);
			CameraOrientation=CP-CameraPosition;
			if(CameraOrientation==M3DVECTOR(0.0f, 0.0f, 0.0f)){CameraOrientation=M3DVECTOR(0.0f, 0.0f, 1.0f);}
			CameraOrientation=Normalize(CameraOrientation);
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(&SMMatrix._11);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();	

			glViewport( DSMapx*SM_WIDTH, DSMapy*SM_HEIGHT, SM_WIDTH, SM_HEIGHT);
			DWORD Temp=mesh->Flags;
			mesh->Flags|=MD3DMESHF_DISABLED;
			int _ZSORT_T=ZSORT;	
			ZSORT=0;
			ATTEMPT(RenderFrame(Lastscene));
			ZSORT=_ZSORT_T;
			mesh->Flags=Temp;
			ATTEMPT(CopyFBtoTX(mesh->Texture[0]->Text, 0, 0, SM_WIDTH, SM_HEIGHT, mesh->Texture[0], 0));
			CameraPosition=CP; CameraOrientation=CO; CameraUp=CU;
			ViewMatrix=CreateCameraMatrix(CameraPosition, CameraOrientation, CameraUp);

			glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(&ProjectionMatrix._11);
			glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
			DSMapx++;	//pre polohu okna
			if(DSMapx*SM_WIDTH+SM_WIDTH>SCREEN_WIDTH){DSMapx=0; DSMapy++;}
		}
	}

	/***************/
	mD3DMesh tmesh;
	for(int i=0;i<mesh->Meshs;i++){
              	tmesh=(mD3DMesh)mesh->Mesh[i];
		for(int j=0;j<mesh->NMeshs[i];j++){
			ATTEMPT(_RenderMesh_Zsort(&tmesh[j], world));
		}
	}
	return MM_OK;
}
_cdecl helpF_zsort(const void* element1, const void* element2) {
	ZSORTstruct *e1=(ZSORTstruct*)element1;
	ZSORTstruct *e2=(ZSORTstruct*)element2;
	if(ZSORT==1){
		if(e1[0].Z>e2[0].Z){
			return 1;
		}
	}
	else if(ZSORT==-1){
		if(e1[0].Z<e2[0].Z){
			return 1;
		}
	}
	return -1;
}
HRESULT _RenderFromBuff_zsort(void){
	qsort(&zsort_first[0], (size_t)Count_zsort_first, sizeof(ZSORTstruct), helpF_zsort);  
	qsort(&zsort[0], (size_t)Count_zsort, sizeof(ZSORTstruct), helpF_zsort);  
	int k;
	for(k=0;k<Count_zsort_first;k++){
		//ohranicenie v predu
		if( (zsort_first[k].Z<=0.0f) && ( ((mD3DMesh(zsort_first[k].hMesh)->Flags&MD3DMESHF_ALWAYSVISIBLE)==MD3DMESHF_ALWAYSVISIBLE) || (VIEWDIST1==0.0f) || (zsort_first[k].Z>VIEWDIST1) )){ATTEMPT(_RenderMesh( (mD3DMesh)zsort_first[k].hMesh, zsort_first[k].World, 0));}
		//zad
		if( (zsort_first[k].Z>0.0f) && ( ((mD3DMesh(zsort_first[k].hMesh)->Flags&MD3DMESHF_ALWAYSVISIBLE)==MD3DMESHF_ALWAYSVISIBLE) || (VIEWDIST2==0.0f) || (zsort_first[k].Z<VIEWDIST2) )){ATTEMPT(_RenderMesh( (mD3DMesh)zsort_first[k].hMesh, zsort_first[k].World, 0));}
	}
	for(k=0;k<Count_zsort;k++){
		//pred
		if( (zsort[k].Z<=0.0f) && ( ((mD3DMesh(zsort[k].hMesh)->Flags&MD3DMESHF_ALWAYSVISIBLE)==MD3DMESHF_ALWAYSVISIBLE) || (VIEWDIST1==0.0f) || (zsort[k].Z>VIEWDIST1) )){ATTEMPT(_RenderMesh( (mD3DMesh)zsort[k].hMesh, zsort[k].World, 0));}
		//zad
		if( (zsort[k].Z>0.0f) && ( ((mD3DMesh(zsort[k].hMesh)->Flags&MD3DMESHF_ALWAYSVISIBLE)==MD3DMESHF_ALWAYSVISIBLE) || (VIEWDIST2==0.0f) || (zsort[k].Z<VIEWDIST2) )){ATTEMPT(_RenderMesh( (mD3DMesh)zsort[k].hMesh, zsort[k].World, 0));}
	}
	return MM_OK;
}
/**************************************************/
/**************************************************/
HRESULT _RenderFrame(mD3DFrame frame, M3DMATRIX world_up){
	frame->SetWorld();
	
	M3DMATRIX actual=M3DMATRIX(frame->world*world_up);
	int i,j;
	for(i=0;i<frame->Meshs;i++){
                ImD3DMesh* Mesh=(mD3DMesh)frame->Mesh[i];
	
		for(j=0;j<frame->NMeshs[i];j++){
			if(ZSORT==1 || ZSORT==-1){ATTEMPT(_RenderMesh_Zsort(&Mesh[j], actual));}
			else{ATTEMPT(_RenderMesh(&Mesh[j], actual, 1));}
		}
	}
	for(i=0;i<frame->Frames;i++){
		for(j=0;j<frame->NFrames[i];j++){
			ImD3DFrame *_Tframe=(ImD3DFrame*)frame->Frame[i];
			ATTEMPT(_RenderFrame(&_Tframe[j], actual));
		}
	}
	return MM_OK;
}
HRESULT RenderFrame(mD3DFrame frame){
	if(frame==NULL){return MM_INVALID_PARAMS;}
	Lastscene=frame;
	//camera
	ViewMatrix=CreateCameraMatrix(CameraPosition, CameraOrientation, CameraUp);

	if(ZSORT==1 || ZSORT==-1){Count_zsort=0; Count_zsort_first=0; DSMapx=0; DSMapy=0;}
	ATTEMPT( _RenderFrame(frame, IdentityMatrix()) );
	if(ZSORT==1 || ZSORT==-1){
		#ifdef CLEARSCENE
			if(DSMapx>0 || DSMapy>0){glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);}
		#endif
		#ifndef CLEARSCENE
			if(DSMapx>0 || DSMapy>0){glClear(GL_DEPTH_BUFFER_BIT);}
		#endif
	
		ATTEMPT(_RenderFromBuff_zsort());
	}
	return MM_OK; 
}

/*************************************************************************/
/********************************end of RENDER****************************/
/*************************************************************************/





/******************************OBJECTS************************************/
/******************SPHERE********************/
HRESULT CreateSphere(float R, long StepRov, long StepPol, float U1, float V1, float U2, float V2, int Ntext, mD3DMesh *xmesh){
	if(xmesh==NULL){return MM_INVALID_PARAMS;}
	mD3DMesh mesh;
	float xU=U2-U1;
	float xV=V2-V1;

	typedef struct __VRX0{float x, y, z;	float nx, ny, nz;}_VRX0;
	typedef struct __VRX1{float x, y, z;	float nx, ny, nz;	float tu, tv;}_VRX1;
	typedef struct __VRX2{float x, y, z;	float nx, ny, nz;	float tu, tv, tu1, tv1;}_VRX2;
	void *vertex;
	long VNum=(1+180/StepRov)*(1+360/StepPol)+1;
	if(Ntext==0){vertex=(_VRX0*)malloc(sizeof(_VRX0)*VNum);if(vertex==NULL){return MM_OUT_OF_MEMORY;}}
	else if(Ntext==1){vertex=(_VRX1*)malloc(sizeof(_VRX1)*VNum);if(vertex==NULL){return MM_OUT_OF_MEMORY;}}
	else if(Ntext==2){vertex=(_VRX2*)malloc(sizeof(_VRX2)*VNum);if(vertex==NULL){return MM_OUT_OF_MEMORY;}}
	DWORD *data;
	data=(DWORD*)malloc(sizeof(DWORD)*VNum*5);if(data==NULL){return MM_OUT_OF_MEMORY;}
	long NRovnobeziek=long(180/StepRov)+1;
	long NPoludnikov=long(360/StepPol)+1;
	float al,be;
	long i,j,count=0;
	M3DVECTOR temp=M3DVECTOR(0.0f,1.0f,0.0f),T,axis,t_axis=M3DVECTOR(1.0f,0.0f,0.0f);
	//POLY
	if(Ntext==0){
		_VRX0 *Vertex=(_VRX0*)(vertex);
		Vertex[count].x=temp.x*R; Vertex[count].y=temp.y*R; Vertex[count].z=temp.z*R;
		Vertex[count].nx=temp.x; Vertex[count].ny=temp.y; Vertex[count].nz=temp.z;
		count++;
		Vertex[count].x=-temp.x*R; Vertex[count].y=-temp.y*R; Vertex[count].z=-temp.z*R;
		Vertex[count].nx=-temp.x; Vertex[count].ny=-temp.y; Vertex[count].nz=-temp.z;
		count++;
	}
	if(Ntext==1){
		_VRX1 *Vertex=(_VRX1*)vertex;
		Vertex[count].x=temp.x*R; Vertex[count].y=temp.y*R; Vertex[count].z=temp.z*R;
		Vertex[count].nx=temp.x; Vertex[count].ny=temp.y; Vertex[count].nz=temp.z;
		Vertex[count].tu=U1+(float(180)/360.0f)*xU;Vertex[count].tv=V1+(float(0)/180.0f)*xV;
		count++;
		Vertex[count].x=-temp.x*R; Vertex[count].y=-temp.y*R; Vertex[count].z=-temp.z*R;
		Vertex[count].nx=-temp.x; Vertex[count].ny=-temp.y; Vertex[count].nz=-temp.z;
		Vertex[count].tu=U1+(float(180)/360.0f)*xU;Vertex[count].tv=V1+(float(180)/180.0f)*xV;
		count++;
	}
	else if(Ntext==2){
		_VRX2 *Vertex=(_VRX2*)vertex;
		Vertex[count].x=temp.x*R; Vertex[count].y=temp.y*R; Vertex[count].z=temp.z*R;
		Vertex[count].nx=temp.x; Vertex[count].ny=temp.y; Vertex[count].nz=temp.z;
		Vertex[count].tu=U1+(float(180)/360.0f)*xU;Vertex[count].tv=V1+(float(0)/180.0f)*xV;
		Vertex[count].tu1=U1+(float(180)/360.0f)*xU;Vertex[count].tv1=V1+(float(0)/180.0f)*xV;
		count++;
		Vertex[count].x=-temp.x*R; Vertex[count].y=-temp.y*R; Vertex[count].z=-temp.z*R;
		Vertex[count].nx=-temp.x; Vertex[count].ny=-temp.y; Vertex[count].nz=-temp.z;
		Vertex[count].tu=U1+(float(180)/360.0f)*xU;Vertex[count].tv=V1+(float(180)/180.0f)*xV;
		Vertex[count].tu1=U1+(float(180)/360.0f)*xU;Vertex[count].tv1=V1+(float(180)/180.0f)*xV;
		count++;
	}
	for(al=0;al<=360;al+=StepPol){
		axis=POINTROTATE(t_axis,M3DVECTOR(0.0f,0.0f,0.0f),M3DVECTOR(0.0f,-1.0f,0.0f),float(-al*(PI/180.0f)) );
		for(be=float(StepRov);be<=180.0-float(StepRov);be+=float(StepRov)){
			T=POINTROTATE(temp,M3DVECTOR(0.0f,0.0f,0.0f),axis,float(-be*(PI/180.0f)) );
			if(Ntext==0){
				_VRX0 *Vertex=(_VRX0*)(vertex);
				Vertex[count].x=T.x*R; Vertex[count].y=T.y*R; Vertex[count].z=T.z*R;
				Vertex[count].nx=T.x; Vertex[count].ny=T.y; Vertex[count].nz=T.z;
			}
			if(Ntext==1){
				_VRX1 *Vertex=(_VRX1*)vertex;
				Vertex[count].x=T.x*R; Vertex[count].y=T.y*R; Vertex[count].z=T.z*R;
				Vertex[count].nx=T.x; Vertex[count].ny=T.y; Vertex[count].nz=T.z;
				Vertex[count].tu=U1+(float(al)/360.0f)*xU;Vertex[count].tv=V1+(float(be)/180.0f)*xV;
			}
			else if(Ntext==2){
				_VRX2 *Vertex=(_VRX2*)vertex;
				Vertex[count].x=T.x*R; Vertex[count].y=T.y*R; Vertex[count].z=T.z*R;
				Vertex[count].nx=T.x; Vertex[count].ny=T.y; Vertex[count].nz=T.z;
				Vertex[count].tu=U1+(float(al)/360.0f)*xU;Vertex[count].tv=V1+(float(be)/180.0f)*xV;
				Vertex[count].tu1=U1+(float(al)/360.0f)*xU;Vertex[count].tv1=V1+(float(be)/180.0f)*xV;
			}
			count++;
		}
	}
	count=0;
	for(j=0;j<NPoludnikov-1;j++){
		for(i=0;i<NRovnobeziek-1;i++){
				if(i==0){
					data[count  ]=3;
					data[count+1]=(2+i+(j+1)*(NRovnobeziek-2));
					data[count+2]=0;
					data[count+3]=(2+i+j*(NRovnobeziek-2));
					count+=4;
				}
				else if(i>=NRovnobeziek-2){
					data[count  ]=3;
					data[count+1]=1;
					data[count+2]=(2+(i-1)+(j+1)*(NRovnobeziek-2));
					data[count+3]=(2+(i-1)+(j)*(NRovnobeziek-2));
					count+=4;
				}
				else{
					data[count  ]=4;
					data[count+1]=(2+i+(j+1)*(NRovnobeziek-2));
					data[count+2]=(2+(i-1)+(j+1)*(NRovnobeziek-2));
					data[count+3]=(2+(i-1)+j*(NRovnobeziek-2));
					data[count+4]=(2+i+j*(NRovnobeziek-2));
					count+=5;
				}
		}
	}
	data[count]=0;
	if(Ntext==0){ATTEMPT(CreatemD3DMesh(M3DFVF_XYZ | M3DFVF_NORMAL, vertex, &data[0], &mesh));}
	else if(Ntext==1){ATTEMPT(CreatemD3DMesh(M3DFVF_XYZ | M3DFVF_NORMAL | M3DFVF_TEX1, vertex, &data[0], &mesh));}
	else if(Ntext==2){ATTEMPT(CreatemD3DMesh(M3DFVF_XYZ | M3DFVF_NORMAL | M3DFVF_TEX2, vertex, &data[0], &mesh));}
	free(vertex);
	free(data);
	memcpy(xmesh, &mesh, sizeof(mD3DMesh));
	return MM_OK;
}
/*******************************************************************************************************/
// prepocita bod na obr. do priamky v priestore
HRESULT GetLinefPixel(M3DVECTOR *POINT, M3DVECTOR *VECT,long Xx, long Yy){
	if(POINT==NULL){return MM_INVALID_PARAMS;}
	if(VECT==NULL){return MM_INVALID_PARAMS;}
	M3DVECTOR right=CrossProduct(CameraOrientation, CameraUp);
	right=Normalize(right);
	M3DVECTOR up=CrossProduct(right, CameraOrientation);
	//vectory kamery musia byt normalizovane
	//up=Normalize(up);
	M3DVECTOR or=CameraOrientation;
	//M3DVECTOR or=Normalize(CameraOrientation);
	float WIDTH,HEIGHT;
	WIDTH=P_NPlane*float(tan((P_fov_horiz/2.0f)*(float(PI)/180.0f)));
	HEIGHT=P_NPlane*float(tan((P_fov_vert/2.0f)*(float(PI)/180.0f)));
	WIDTH=WIDTH*float((float(Xx)/float(SCREEN_WIDTH))*2.0f-1.0f);
	HEIGHT=HEIGHT*float((float(SCREEN_HEIGHT-Yy)/float(SCREEN_HEIGHT))*2.0f-1.0f);
	right=M3DVECTOR(right*WIDTH);
	up=M3DVECTOR(up*HEIGHT);
	or=M3DVECTOR(or*P_NPlane);
	M3DVECTOR final,dir;
	final=CameraPosition+or+up+right;
	dir=final-CameraPosition;
	memcpy(POINT, &final, sizeof(M3DVECTOR));
	memcpy(VECT, &dir, sizeof(M3DVECTOR));
	return MM_OK;
}
/****************************************************************************/
/*********************************TEXTURES**********************************/
/*********************************/
/*	N - index textury
	fileRGB - nazov suboru pre RGB
	X1, Y1, X2, Y2 - rect RGB, ktory sa ma loadnut (je roztiahnuty na celu text - bilinearny filtering)
	fileA - nazov suboru pre Alpha
	XX1, YY1, XX2, YY2 - rect Alpha
	Width, Height - rozmery vyslednej textury, ale su upravene na mocninu 2 a bude stvorcova. 
				ak Width/Height=0 pokusi sa zobrat rozmer z RGB textury (pripadne z Alpha)
	Text - pointer na texturu
	Mip - pouzit mipmapping
	Funkcia nevracia chybu ak nenajde dane subory, vytvori bielu texturu
*/
HRESULT LoadBMP_to_RGBA(DWORD N, LPCSTR fileRGB, float X1, float Y1,float X2, float Y2, LPCSTR fileA, float XX1, float YY1, float XX2, float YY2, int Width, int Height, mD3DTexture *Text, BOOL Mip){
	if(Text==NULL){return MM_INVALID_PARAMS;}

	BOOL RGB=1, ALPHA=1;			//ci existuju BMP
	DWORD RGBBPP=0, ABPP=0;		//hlbka farieb bitmap
	HBITMAP hbitmap=NULL, hbitmapA=NULL;
	BITMAP bitmapinfo, bitmapinfoA;
	BYTE *DATA;
	float fWidthRGB=0, fHeightRGB=0, fWidthA=0, fHeightA=0;
	RGBQUAD table[256];
	HDC dc;

	if(fileRGB!=NULL){
		hbitmap=(HBITMAP)LoadImage(NULL, fileRGB, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
		if(hbitmap==NULL){RGB=0;}
	}else{RGB=0;}
	if(fileA!=NULL){
		hbitmapA=(HBITMAP)LoadImage(NULL, fileA, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
		if(hbitmapA==NULL){ALPHA=0;}
	}else{ALPHA=0;}

	//ak X2<X1 alebo Y2<Y1 ich vymena
	if(X2<X1){X2=X2+X1; X1=X2-X1; X2=X2-X1;}
	if(Y2<Y1){Y2=Y2+Y1; Y1=Y2-Y1; Y2=Y2-Y1;}
	//ak XX2<XX1 alebo YY2<YY1 ich vymena
	if(XX2<XX1){XX2=XX2+XX1; XX1=XX2-XX1; XX2=XX2-XX1;}
	if(YY2<YY1){YY2=YY2+YY1; YY1=YY2-YY1; YY2=YY2-YY1;}

	if(RGB==1){
		GetObject(hbitmap, sizeof(BITMAP), (void*)&bitmapinfo );
		fWidthRGB=float(bitmapinfo.bmWidth)*(X2-X1);
		fHeightRGB=float(bitmapinfo.bmHeight)*(Y2-Y1);
		RGBBPP=bitmapinfo.bmBitsPixel;
	}
	if(ALPHA==1){
		GetObject(hbitmapA, sizeof(BITMAP), (void*)&bitmapinfoA );
		fWidthA=float(bitmapinfoA.bmWidth)*(XX2-XX1);
		fHeightA=float(bitmapinfoA.bmHeight)*(YY2-YY1);
		ABPP=bitmapinfoA.bmBitsPixel;
	}
	if(Width==0){Width=(int)fWidthRGB;} if(Width==0){Width=(int)fWidthA;}
	if(Height==0){Height=(int)fHeightRGB;} if(Height==0){Height=(int)fHeightA;}
	//uprava na mocninu dvoch
	int xW, xH;
	for(xW=1;Width>xW;xW<<=1);
	for(xH=1;Height>xH;xH<<=1);
	if(xW>xH){xH = xW;}
	else{xW=xH;}
	Width=xW; Height=xH;

	DATA=(BYTE*)malloc(Width*Height*4); if(DATA==NULL){return MM_OUT_OF_MEMORY;}
	memset(DATA, 255, Width*Height*4);
	
	BYTE *BitmapByte, *BitmapByteA;
	if(RGB==1){BitmapByte=(BYTE *)bitmapinfo.bmBits;}
	if(ALPHA==1){BitmapByteA=(BYTE *)bitmapinfoA.bmBits;}
	BYTE R, G, B;
	int x, y;
	float X, Y, zX, zY;
	long CLH, CPH, CLD, CPD;

	if(RGBBPP==24){
		for(y=0;y<Height;y++){
			for(x=0;x<Width;x++){
				X=(float(x)/float(Width))*fWidthRGB+X1*float(bitmapinfo.bmWidth);
				Y=(float(y)/float(Height))*fHeightRGB+Y1*float(bitmapinfo.bmHeight);
				zX=X-float(int(X)); zY=Y-float(int(Y));
				CLH=long(X)*3+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
				CPH=(long(X)+1)*3+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
				CLD=long(X)*3+((bitmapinfo.bmHeight-1)-long(Y)-1)*bitmapinfo.bmWidthBytes;
				CPD=(long(X)+1)*3+((bitmapinfo.bmHeight-1)-long(Y)-1)*bitmapinfo.bmWidthBytes;
				if(X>=float(bitmapinfo.bmWidth-1)){CPH=CLH; CPD=CLD;}
				if(Y>=float(bitmapinfo.bmHeight-1)){CLD=CLH; CPD=CPH;}
				B=BYTE((float(BitmapByte[CLH])*(1.0f-zX)+float(BitmapByte[CPH])*(zX))*(1.0f-zY) + (float(BitmapByte[CLD])*(1.0f-zX)+float(BitmapByte[CPD])*(zX))*(zY));
				G=BYTE((float(BitmapByte[CLH+1])*(1.0f-zX)+float(BitmapByte[CPH+1])*(zX))*(1.0f-zY) + (float(BitmapByte[CLD+1])*(1.0f-zX)+float(BitmapByte[CPD+1])*(zX))*(zY));
				R=BYTE((float(BitmapByte[CLH+2])*(1.0f-zX)+float(BitmapByte[CPH+2])*(zX))*(1.0f-zY) + (float(BitmapByte[CLD+2])*(1.0f-zX)+float(BitmapByte[CPD+2])*(zX))*(zY));
				DATA[(x+y*Width)*4]=R; DATA[(x+y*Width)*4+1]=G; DATA[(x+y*Width)*4+2]=B;
			}
		}
	}
	else if(RGBBPP==8){
		dc=CreateCompatibleDC(NULL);
		SelectObject(dc,hbitmap);
		GetDIBColorTable(dc,0,256,&table[0]);
		DeleteDC(dc);
		for(y=0;y<Height;y++){
			for(x=0;x<Width;x++){
				X=(float(x)/float(Width))*fWidthRGB+X1*float(bitmapinfo.bmWidth);
				Y=(float(y)/float(Height))*fHeightRGB+Y1*float(bitmapinfo.bmHeight);
				zX=X-float(int(X)); zY=Y-float(int(Y));
				CLH=long(X)+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
				CPH=long(X)+1+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
				CLD=long(X)+((bitmapinfo.bmHeight-1)-long(Y)-1)*bitmapinfo.bmWidthBytes;
				CPD=long(X)+1+((bitmapinfo.bmHeight-1)-long(Y)-1)*bitmapinfo.bmWidthBytes;
				if(X>=float(bitmapinfo.bmWidth-1)){CPH=CLH; CPD=CLD;}
				if(Y>=float(bitmapinfo.bmHeight-1)){CLD=CLH; CPD=CPH;}
				B=BYTE((float(table[BitmapByte[CLH]].rgbBlue)*(1.0f-zX)+float(table[BitmapByte[CPH]].rgbBlue)*(zX))*(1.0f-zY) + (float(table[BitmapByte[CLD]].rgbBlue)*(1.0f-zX)+float(table[BitmapByte[CPD]].rgbBlue)*(zX))*(zY));
				G=BYTE((float(table[BitmapByte[CLH]].rgbGreen)*(1.0f-zX)+float(table[BitmapByte[CPH]].rgbGreen)*(zX))*(1.0f-zY) + (float(table[BitmapByte[CLD]].rgbGreen)*(1.0f-zX)+float(table[BitmapByte[CPD]].rgbGreen)*(zX))*(zY));
				R=BYTE((float(table[BitmapByte[CLH]].rgbRed)*(1.0f-zX)+float(table[BitmapByte[CPH]].rgbRed)*(zX))*(1.0f-zY) + (float(table[BitmapByte[CLD]].rgbRed)*(1.0f-zX)+float(table[BitmapByte[CPD]].rgbRed)*(zX))*(zY));
				DATA[(x+y*Width)*4]=R; DATA[(x+y*Width)*4+1]=G; DATA[(x+y*Width)*4+2]=B;
			}
		}
	}
	if(ABPP==24){
		for(y=0;y<Height;y++){
			for(x=0;x<Width;x++){
				X=(float(x)/float(Width))*fWidthA+XX1*float(bitmapinfoA.bmWidth);
				Y=(float(y)/float(Height))*fHeightA+YY1*float(bitmapinfoA.bmHeight);
				zX=X-float(int(X)); zY=Y-float(int(Y));
				CLH=long(X)*3+((bitmapinfoA.bmHeight-1)-long(Y))*bitmapinfoA.bmWidthBytes;
				CPH=(long(X)+1)*3+((bitmapinfoA.bmHeight-1)-long(Y))*bitmapinfoA.bmWidthBytes;
				CLD=long(X)*3+((bitmapinfoA.bmHeight-1)-long(Y)-1)*bitmapinfoA.bmWidthBytes;
				CPD=(long(X)+1)*3+((bitmapinfoA.bmHeight-1)-long(Y)-1)*bitmapinfoA.bmWidthBytes;
				if(X>=float(bitmapinfoA.bmWidth-1)){CPH=CLH; CPD=CLD;}
				if(Y>=float(bitmapinfoA.bmHeight-1)){CLD=CLH; CPD=CPH;}
				B=BYTE((float(BitmapByteA[CLH])*(1.0f-zX)+float(BitmapByteA[CPH])*(zX))*(1.0f-zY) + (float(BitmapByteA[CLD])*(1.0f-zX)+float(BitmapByteA[CPD])*(zX))*(zY));
				G=BYTE((float(BitmapByteA[CLH+1])*(1.0f-zX)+float(BitmapByteA[CPH+1])*(zX))*(1.0f-zY) + (float(BitmapByteA[CLD+1])*(1.0f-zX)+float(BitmapByteA[CPD+1])*(zX))*(zY));
				R=BYTE((float(BitmapByteA[CLH+2])*(1.0f-zX)+float(BitmapByteA[CPH+2])*(zX))*(1.0f-zY) + (float(BitmapByteA[CLD+2])*(1.0f-zX)+float(BitmapByteA[CPD+2])*(zX))*(zY));
				DATA[(x+y*Width)*4+3]=BYTE(float(R+G+B)/3.0f);
			}
		}
	}
	else if(ABPP==8){
		dc=CreateCompatibleDC(NULL);
		SelectObject(dc,hbitmapA);
		GetDIBColorTable(dc,0,256,&table[0]);
		DeleteDC(dc);
		for(y=0;y<Height;y++){
			for(x=0;x<Width;x++){
				X=(float(x)/float(Width))*fWidthA+X1*float(bitmapinfoA.bmWidth);
				Y=(float(y)/float(Height))*fHeightA+Y1*float(bitmapinfoA.bmHeight);
				zX=X-float(int(X)); zY=Y-float(int(Y));
				CLH=long(X)+((bitmapinfoA.bmHeight-1)-long(Y))*bitmapinfoA.bmWidthBytes;
				CPH=long(X)+1+((bitmapinfoA.bmHeight-1)-long(Y))*bitmapinfoA.bmWidthBytes;
				CLD=long(X)+((bitmapinfoA.bmHeight-1)-long(Y)-1)*bitmapinfoA.bmWidthBytes;
				CPD=long(X)+1+((bitmapinfoA.bmHeight-1)-long(Y)-1)*bitmapinfoA.bmWidthBytes;
				if(X>=float(bitmapinfoA.bmWidth-1)){CPH=CLH; CPD=CLD;}
				if(Y>=float(bitmapinfoA.bmHeight-1)){CLD=CLH; CPD=CPH;}
				B=BYTE((float(table[BitmapByteA[CLH]].rgbBlue)*(1.0f-zX)+float(table[BitmapByteA[CPH]].rgbBlue)*(zX))*(1.0f-zY) + (float(table[BitmapByteA[CLD]].rgbBlue)*(1.0f-zX)+float(table[BitmapByteA[CPD]].rgbBlue)*(zX))*(zY));
				G=BYTE((float(table[BitmapByteA[CLH]].rgbGreen)*(1.0f-zX)+float(table[BitmapByteA[CPH]].rgbGreen)*(zX))*(1.0f-zY) + (float(table[BitmapByteA[CLD]].rgbGreen)*(1.0f-zX)+float(table[BitmapByteA[CPD]].rgbGreen)*(zX))*(zY));
				R=BYTE((float(table[BitmapByteA[CLH]].rgbRed)*(1.0f-zX)+float(table[BitmapByteA[CPH]].rgbRed)*(zX))*(1.0f-zY) + (float(table[BitmapByteA[CLD]].rgbRed)*(1.0f-zX)+float(table[BitmapByteA[CPD]].rgbRed)*(zX))*(zY));
				DATA[(x+y*Width)*4+3]=BYTE(float(R+G+B)/3.0f);
			}
		}
	}

	if(RGB==1){DeleteObject(hbitmap);} if(ALPHA==1){DeleteObject(hbitmapA);}
	Text->Text=N;
	Text->Width=Width; Text->Height=Height;
	glBindTexture(GL_TEXTURE_2D, Text->Text);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, TEXT_MAG_FILTER);
	if(Mip==1){
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, TEXT_MIN_FILTER_MIP);
	}
	else{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, TEXT_MIN_FILTER);
	}
	if(Mip==1){
		gluBuild2DMipmaps(GL_TEXTURE_2D, TEXTF, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, &DATA[0]);
	}
	else{
		glTexImage2D(GL_TEXTURE_2D, 0, TEXTF, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &DATA[0]);
	}
	free(DATA);

	//Name
	int i=0, last=-1;
	if(RGB==1){
		while(fileRGB[i]!='.'){
			if(fileRGB[i]=='\\'){last=i;}
			i++;
		}
		memcpy(Text->Name, &fileRGB[last+1], i-last-1);
		Text->Name[i-last-1]='\n';
	}
	else if(ALPHA==1){
		while(fileA[i]!='.'){
			if(fileA[i]=='\\'){last=i;}
			i++;
		}
		memcpy(Text->Name, &fileA[last+1], i-last-1);
		Text->Name[i-last-1]='\n';
	}
	return MM_OK;
}
/*********************************/
/*	ako LoadBMP_to_RGBA, az na: 
	Transp - zapnut priehladnost
	rr, gg, bb - hodnoty uplne priehladnej farby
	TOL - tolerancia (0, priehladna je iba farba s rr, gg, bb)
				(a, priehladne su body, ktore splnaju (abs(R-rr)<=TOL)&&(abs(G-gg)<=TOL)&&(abs(B-bb)<=TOL)
				(255, priehladne su vsetky body bitmapy, ALPHA=(abs(R-rr)+abs(G-gg)+abs(B-bb))/3.0f
*/
HRESULT LoadBMP(DWORD N, LPCSTR fileRGB, float X1, float Y1,float X2, float Y2, BOOL Transp, BYTE rr, BYTE gg, BYTE bb, BYTE TOL,  int Width, int Height, mD3DTexture *Text, BOOL Mip){
	if(Text==NULL){return MM_INVALID_PARAMS;}

	BOOL RGB=1;			//ci existuju BMP
	DWORD RGBBPP=0;		//hlbka farieb bitmap
	HBITMAP hbitmap=NULL;
	BITMAP bitmapinfo;
	BYTE *DATA;
	float fWidthRGB=0, fHeightRGB=0;
	RGBQUAD table[256];
	HDC dc;

	if(fileRGB!=NULL){
		hbitmap=(HBITMAP)LoadImage(NULL, fileRGB, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
		if(hbitmap==NULL){RGB=0;}
	}else{RGB=0;}
	
	//ak X2<X1 alebo Y2<Y1 ich vymena
	if(X2<X1){X2=X2+X1; X1=X2-X1; X2=X2-X1;}
	if(Y2<Y1){Y2=Y2+Y1; Y1=Y2-Y1; Y2=Y2-Y1;}
	
	if(RGB==1){
		GetObject(hbitmap, sizeof(BITMAP), (void*)&bitmapinfo );
		fWidthRGB=float(bitmapinfo.bmWidth)*(X2-X1);
		fHeightRGB=float(bitmapinfo.bmHeight)*(Y2-Y1);
		RGBBPP=bitmapinfo.bmBitsPixel;
	}
	if(Width==0){Width=(int)fWidthRGB;}
	if(Height==0){Height=(int)fHeightRGB;}
	//uprava na mocninu dvoch
	int xW, xH;
	for(xW=1;Width>xW;xW<<=1);
	for(xH=1;Height>xH;xH<<=1);
	if(xW>xH){xH = xW;}
	else{xW=xH;}
	Width=xW; Height=xH;

	DATA=(BYTE*)malloc(Width*Height*4); if(DATA==NULL){return MM_OUT_OF_MEMORY;}
	memset(DATA, 255, Width*Height*4);

	BYTE *BitmapByte;
	if(RGB==1){BitmapByte=(BYTE *)bitmapinfo.bmBits;}
	BYTE R, G, B;
	int x, y;
	float X, Y, zX, zY;
	long CLH, CPH, CLD, CPD;

	if(RGBBPP==24){
		for(y=0;y<Height;y++){
			for(x=0;x<Width;x++){
				X=(float(x)/float(Width))*fWidthRGB+X1*float(bitmapinfo.bmWidth);
				Y=(float(y)/float(Height))*fHeightRGB+Y1*float(bitmapinfo.bmHeight);
				zX=X-float(int(X)); zY=Y-float(int(Y));
				CLH=long(X)*3+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
				CPH=(long(X)+1)*3+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
				CLD=long(X)*3+((bitmapinfo.bmHeight-1)-long(Y)-1)*bitmapinfo.bmWidthBytes;
				CPD=(long(X)+1)*3+((bitmapinfo.bmHeight-1)-long(Y)-1)*bitmapinfo.bmWidthBytes;
				if(X>=float(bitmapinfo.bmWidth-1)){CPH=CLH; CPD=CLD;}
				if(Y>=float(bitmapinfo.bmHeight-1)){CLD=CLH; CPD=CPH;}
				B=BYTE((float(BitmapByte[CLH])*(1.0f-zX)+float(BitmapByte[CPH])*(zX))*(1.0f-zY) + (float(BitmapByte[CLD])*(1.0f-zX)+float(BitmapByte[CPD])*(zX))*(zY));
				G=BYTE((float(BitmapByte[CLH+1])*(1.0f-zX)+float(BitmapByte[CPH+1])*(zX))*(1.0f-zY) + (float(BitmapByte[CLD+1])*(1.0f-zX)+float(BitmapByte[CPD+1])*(zX))*(zY));
				R=BYTE((float(BitmapByte[CLH+2])*(1.0f-zX)+float(BitmapByte[CPH+2])*(zX))*(1.0f-zY) + (float(BitmapByte[CLD+2])*(1.0f-zX)+float(BitmapByte[CPD+2])*(zX))*(zY));
				DATA[(x+y*Width)*4]=R; DATA[(x+y*Width)*4+1]=G; DATA[(x+y*Width)*4+2]=B;
				if(Transp==1){
					if( (abs(R-rr)<=TOL)&&(abs(G-gg)<=TOL)&&(abs(B-bb)<=TOL)){DATA[(x+y*Width)*4+3]=BYTE(float( abs(R-rr)+abs(G-gg)+abs(B-bb) )/3.0f);}
				}
			}
		}
	}
	else if(RGBBPP==8){
		dc=CreateCompatibleDC(NULL);
		SelectObject(dc,hbitmap);
		GetDIBColorTable(dc,0,256,&table[0]);
		DeleteDC(dc);
		for(y=0;y<Height;y++){
			for(x=0;x<Width;x++){
				X=(float(x)/float(Width))*fWidthRGB+X1*float(bitmapinfo.bmWidth);
				Y=(float(y)/float(Height))*fHeightRGB+Y1*float(bitmapinfo.bmHeight);
				zX=X-float(int(X)); zY=Y-float(int(Y));
				CLH=long(X)+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
				CPH=long(X)+1+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
				CLD=long(X)+((bitmapinfo.bmHeight-1)-long(Y)-1)*bitmapinfo.bmWidthBytes;
				CPD=long(X)+1+((bitmapinfo.bmHeight-1)-long(Y)-1)*bitmapinfo.bmWidthBytes;
				if(X>=float(bitmapinfo.bmWidth-1)){CPH=CLH; CPD=CLD;}
				if(Y>=float(bitmapinfo.bmHeight-1)){CLD=CLH; CPD=CPH;}
				B=BYTE((float(table[BitmapByte[CLH]].rgbBlue)*(1.0f-zX)+float(table[BitmapByte[CPH]].rgbBlue)*(zX))*(1.0f-zY) + (float(table[BitmapByte[CLD]].rgbBlue)*(1.0f-zX)+float(table[BitmapByte[CPD]].rgbBlue)*(zX))*(zY));
				G=BYTE((float(table[BitmapByte[CLH]].rgbGreen)*(1.0f-zX)+float(table[BitmapByte[CPH]].rgbGreen)*(zX))*(1.0f-zY) + (float(table[BitmapByte[CLD]].rgbGreen)*(1.0f-zX)+float(table[BitmapByte[CPD]].rgbGreen)*(zX))*(zY));
				R=BYTE((float(table[BitmapByte[CLH]].rgbRed)*(1.0f-zX)+float(table[BitmapByte[CPH]].rgbRed)*(zX))*(1.0f-zY) + (float(table[BitmapByte[CLD]].rgbRed)*(1.0f-zX)+float(table[BitmapByte[CPD]].rgbRed)*(zX))*(zY));
				DATA[(x+y*Width)*4]=R; DATA[(x+y*Width)*4+1]=G; DATA[(x+y*Width)*4+2]=B;
				if(Transp==1){
					if( (abs(R-rr)<=TOL)&&(abs(G-gg)<=TOL)&&(abs(B-bb)<=TOL)){DATA[(x+y*Width)*4+3]=BYTE(float( abs(R-rr)+abs(G-gg)+abs(B-bb) )/3.0f);}
				}
			}
		}
	}
	if(RGB==1){DeleteObject(hbitmap);}
	Text->Text=N;
	Text->Width=Width; Text->Height=Height;
	glBindTexture(GL_TEXTURE_2D, Text->Text);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, TEXT_MAG_FILTER);
	if(Mip==1){
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, TEXT_MIN_FILTER_MIP);
	}
	else{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, TEXT_MIN_FILTER);
	}
	if(Mip==1){
		gluBuild2DMipmaps(GL_TEXTURE_2D, TEXTF, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, &DATA[0]);
	}
	else{
		glTexImage2D(GL_TEXTURE_2D, 0, TEXTF, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &DATA[0]);
	}
	free(DATA);

	//Name
	int i=0, last=-1;
	if(RGB==1){
		while(fileRGB[i]!='.'){
			if(fileRGB[i]=='\\'){last=i;}
			i++;
		}
		memcpy(Text->Name, &fileRGB[last+1], i-last-1);
		Text->Name[i-last-1]='\n';
	}
	return MM_OK;
}
/************************************/
/*	bez interpolacie, vracia chybu ak nenajde
	ak je textura vacsia ako obr. precnievajuce
	casti su cierne
	Ostatne ako LoadBMP
*/
HRESULT LoadBMP_Fast(DWORD N, LPCSTR fileRGB, float X1, float Y1,float X2, float Y2, BOOL Transp, BYTE rr, BYTE gg, BYTE bb, BYTE TOL, mD3DTexture *Text, BOOL Mip){
	if(Text==NULL){return MM_INVALID_PARAMS;}

	BOOL RGB=1;			//ci existuju BMP
	HBITMAP hbitmap=NULL;
	DWORD RGBBPP=0;		//hlbka farieb bitmap
	BITMAP bitmapinfo;
	BYTE *DATA;
	float fWidthRGB=0, fHeightRGB=0;
	RGBQUAD table[256];
	HDC dc;

	if(fileRGB!=NULL){
		hbitmap=(HBITMAP)LoadImage(NULL, fileRGB, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
		if(hbitmap==NULL){RGB=0;}
	}else{RGB=0;}

	//ak X2<X1 alebo Y2<Y1 ich vymena
	if(X2<X1){X2=X2+X1; X1=X2-X1; X2=X2-X1;}
	if(Y2<Y1){Y2=Y2+Y1; Y1=Y2-Y1; Y2=Y2-Y1;}
	
	if(RGB==1){
		GetObject(hbitmap, sizeof(BITMAP), (void*)&bitmapinfo );
		fWidthRGB=float(bitmapinfo.bmWidth)*(X2-X1);
		fHeightRGB=float(bitmapinfo.bmHeight)*(Y2-Y1);
		RGBBPP=bitmapinfo.bmBitsPixel;
	}
	long Width=(long)fWidthRGB;
	long Height=(long)fHeightRGB;
	//uprava na mocninu dvoch
	long xW, xH;
	for(xW=1;Width>xW;xW<<=1);
	for(xH=1;Height>xH;xH<<=1);
	if(xW>xH){xH = xW;}
	else{xW=xH;}
	Width=xW; Height=xH;

	DATA=(BYTE*)malloc(Width*Height*4); if(DATA==NULL){return MM_OUT_OF_MEMORY;}
	memset(DATA, 255, Width*Height*4);

	
	BYTE *BitmapByte;
	if(RGB==1){BitmapByte=(BYTE *)bitmapinfo.bmBits;}
	BYTE R, G, B;
	long x, y;
	long X,Y, Xzz=long(X1*float(bitmapinfo.bmWidth)), Yzz=long(Y1*float(bitmapinfo.bmHeight));
	long CLH;
	if(RGBBPP==24){
		for(y=0;y<Height;y++){
			for(x=0;x<Width;x++){
				X=x+Xzz;
				Y=y+Yzz;
				if(X>=0 && X<bitmapinfo.bmWidth && Y>=0 && Y<bitmapinfo.bmHeight){
					CLH=long(X)*3+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
					B=BitmapByte[CLH];
					G=BitmapByte[CLH+1];
					R=BitmapByte[CLH+2];
				}else{R=0;G=0;B=0;}
				DATA[(x+y*Width)*4]=R; DATA[(x+y*Width)*4+1]=G; DATA[(x+y*Width)*4+2]=B;
				if(Transp==1){
					if( (abs(R-rr)<=TOL)&&(abs(G-gg)<=TOL)&&(abs(B-bb)<=TOL)){DATA[(x+y*Width)*4+3]=0;}
				}
			}
		}
	}
	else if(RGBBPP==8){
		dc=CreateCompatibleDC(NULL);
		SelectObject(dc,hbitmap);
		GetDIBColorTable(dc,0,256,&table[0]);
		DeleteDC(dc);
		for(y=0;y<Height;y++){
			for(x=0;x<Width;x++){
				X=x+Xzz;
				Y=y+Yzz;
				if( X>=0 && X<long(bitmapinfo.bmWidth) && Y>=0 && Y<long(bitmapinfo.bmHeight)){
					CLH=long(X)+((bitmapinfo.bmHeight-1)-long(Y))*bitmapinfo.bmWidthBytes;
					B=table[BitmapByte[CLH]].rgbBlue;
					G=table[BitmapByte[CLH]].rgbGreen;
					R=table[BitmapByte[CLH]].rgbRed;
				}else{R=0;G=0;B=0;}
				DATA[(x+y*Width)*4]=R; DATA[(x+y*Width)*4+1]=G; DATA[(x+y*Width)*4+2]=B;
				if(Transp==1){
					if( (abs(R-rr)<=TOL)&&(abs(G-gg)<=TOL)&&(abs(B-bb)<=TOL)){DATA[(x+y*Width)*4+3]=0;}
				}
			}
		}
	}
	if(RGB==1){DeleteObject(hbitmap);}
	Text->Text=N;
	Text->Width=Width; Text->Height=Height;
	glBindTexture(GL_TEXTURE_2D, Text->Text);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, TEXT_MAG_FILTER);
	if(Mip==1){
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, TEXT_MIN_FILTER_MIP);
	}
	else{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, TEXT_MIN_FILTER);
	}
	if(Mip==1){
		gluBuild2DMipmaps(GL_TEXTURE_2D, TEXTF, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, &DATA[0]);
	}
	else{
		glTexImage2D(GL_TEXTURE_2D, 0, TEXTF, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &DATA[0]);
	}
	free(DATA);

	//Name
	int i=0, last=-1;
	if(RGB==1){
		while(fileRGB[i]!='.'){
			if(fileRGB[i]=='\\'){last=i;}
			i++;
		}
		memcpy(Text->Name, &fileRGB[last+1], i-last-1);
		Text->Name[i-last-1]='\n';
	}
	return MM_OK;
}
/**********************************************************************************************************/
/******************************************* LOAD / SAVE ***************************************************/
/***********SAVE***********/
/*****VLASTNY FORMAT*******/
/*
HRESULT _SavemD3DMesh(FILE *file, mD3DMesh mesh){
	if(mesh==NULL){return MM_INVALID_PARAMS;}

	long a, b, c;
	fwrite(&mesh->Name[0], 1, NAMELENGHT, file);
	fwrite(&mesh->Flags, sizeof(mesh->Flags), 1, file);

	//number of Data
	if(mesh->Data!=NULL){
		a=0;
		while(mesh->Data[a]!=0){
			a=a+long(mesh->Data[a])+1;
		}a++;
		fwrite(&a, sizeof(a), 1, file);
		fwrite(&mesh->Data[0], sizeof(mesh->Data[0]), a, file);
	}else{a=0; fwrite(&a, sizeof(a), 1, file);}

	fwrite(&mesh->VertexStruct, sizeof(mesh->VertexStruct), 1, file);
	fwrite(&mesh->VertexSize, sizeof(mesh->VertexSize), 1, file);
	fwrite(&mesh->VertexCount, sizeof(mesh->VertexCount), 1, file);
	for(a=0;a<mesh->VertexCount;a++){
		fwrite(&mesh->Vertex[a*mesh->VertexSize/sizeof(float)], mesh->VertexSize, 1, file);
	}
	fwrite(&mesh->SrcBlend, sizeof(mesh->SrcBlend), 1, file);
	fwrite(&mesh->DestBlend, sizeof(mesh->DestBlend), 1, file);
	fwrite(&mesh->Textures, sizeof(mesh->Textures), 1, file);
	for(a=0;a<mesh->Textures;a++){
		fwrite(&mesh->Texture[a]->Name[0], 1, NAMELENGHT, file);
	}
	fwrite(&mesh->MatFlags, sizeof(mesh->MatFlags), 1, file);
	fwrite(&mesh->Material.Name[0], 1, NAMELENGHT, file);
	
	fwrite(&mesh->Meshs, sizeof(mesh->Meshs), 1, file);
	c=0;
	for(a=0;a<mesh->Meshs;a++){
		fwrite(&mesh->NMeshs[a], sizeof(mesh->NMeshs[a]), 1, file);
		for(b=0;b<mesh->NMeshs[b];b++){
			_SavemD3DMesh(file, mesh->Mesh[c]);
			c++;
		}
	}
	return MM_OK;
}
HRESULT SavemD3DMesh(char *name, mD3DMesh mesh){
	if(mesh==NULL){return MM_INVALID_PARAMS;}

	FILE *file;
	file=fopen(name, "wb");
	if(file==NULL){
		char err[80];
		sprintf(&err[0], "Unable to create file %s\n", name); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;
	}
	fwrite(&FNAME[0], strlen(FNAME)+1, 1, file);
	ATTEMPT(_SavemD3DMesh(file, mesh));

	fclose(file);
	return MM_OK;
}
*/
/***********LOAD***********/
/*HRESULT _LoadmD3DMesh(int nT, mD3DTexture *text, int nM, M3DMATERIAL mat, FILE *file, mD3DMesh *Mesh){
	if(Mesh==NULL){return MM_INVALID_PARAMS;}

	mD3DMesh mesh=(mD3DMesh)malloc(sizeof(ImD3DMesh));
	if(mesh==NULL){return MM_OUT_OF_MEMORY;}
	memset(mesh, 0, sizeof(ImD3DMesh));
	memcpy(Mesh, &mesh, sizeof(mD3DMesh));

	long a, b, c;
	fread(&mesh->Name[0], 1, NAMELENGHT, file);
	fread(&mesh->Flags, sizeof(mesh->Flags), 1, file);
	fread(&c, sizeof(c), 1, file);

	if(c!=0){
		mesh->Data=(DWORD*)malloc(sizeof(DWORD)*c);
		if(mesh->Data==NULL){return MM_OUT_OF_MEMORY;}
		memset(&mesh->Data[0], 0, sizeof(DWORD)*c);
		fread(&mesh->Data[0], sizeof(mesh->Data[0]), c, file);
	}else{mesh->Data=NULL;}
	fread(&mesh->VertexStruct, sizeof(mesh->VertexStruct), 1, file);
	fread(&mesh->VertexSize, sizeof(mesh->VertexSize), 1, file);
	fread(&mesh->VertexCount, sizeof(mesh->VertexCount), 1, file);
	if(mesh->VertexCount!=0 && mesh->VertexSize!=0){
		mesh->Vertex=(float*)malloc(mesh->VertexSize*mesh->VertexCount);
		if(mesh->Vertex==NULL){return MM_OUT_OF_MEMORY;}
		for(a=0;a<mesh->VertexCount;a++){
			fread(&mesh->Vertex[a*mesh->VertexSize/sizeof(float)], mesh->VertexSize, 1, file);
		}
	}
	fread(&mesh->SrcBlend, sizeof(mesh->SrcBlend), 1, file);
	fread(&mesh->DestBlend, sizeof(mesh->DestBlend), 1, file);
	fread(&mesh->Textures, sizeof(mesh->Textures), 1, file);
	char X[NAMELENGHT];
	for(a=0;a<mesh->Textures;a++){
		fread(&X[0], 1, NAMELENGHT, file);
		for(b=0;b<nT;b++){
			if(strcmp(text[b].Name, X)==0){mesh->Texture[a]=&text[b];}
		}
	}
	fread(&mesh->MatFlags, sizeof(mesh->MatFlags), 1, file);
	fread(&X[0], 1, NAMELENGHT, file);
	mesh->Material=DEFMATERIAL;
	for(b=0;b<nM;b++){
		if(strcmp(mat[b].Name, X)==0){mesh->Material=mat[b];}
	}
	fread(&mesh->Meshs, sizeof(mesh->Meshs), 1, file);
	c=0;
	for(a=0;a<mesh->Meshs;a++){
		fread(&mesh->NMeshs[a], sizeof(mesh->NMeshs[a]), 1, file);
		for(b=0;b<mesh->NMeshs[b];b++){
			_LoadmD3DMesh(nT, text, nM, mat, file, &mesh->Mesh[c]);
			c++;
		}
	}
	return MM_OK;
}
HRESULT LoadmD3DMesh(int nT, mD3DTexture *text, int nM, M3DMATERIAL mat, char *name, mD3DMesh *mesh){
	if(mesh==NULL){return MM_INVALID_PARAMS;}

	FILE *file;
	file=fopen(name, "rb");
	if(file==NULL){
		char err[80];
		sprintf(&err[0], "Unable to open file %s\n", name); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;
	}
	char X[20];
	fread(&X[0], strlen(FNAME)+1, 1, file);
	
	if(strcmp(X, FNAME)!=0){
		char err[80];
		sprintf(&err[0], "Invalid file %s\n", name); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;
	} 
	ATTEMPT(_LoadmD3DMesh(nT, text, nM, mat, file, mesh));

	fclose(file);
	return MM_OK;
}
*/

/*****************************FONT**********************************/
/*******************************************************************/

GLubyte space[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
GLubyte ciarka[] = {0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
GLubyte bodka[] = {0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
GLubyte dvojbodka[] = {0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00};
GLubyte pomlcka[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
GLubyte plus[] = {0x00, 0x00, 0x18, 0x18, 0x18, 0x7e, 0x7e, 0x7e, 0x18, 0x18, 0x18, 0x00, 0x00};
GLubyte mocnina[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3, 0xc3, 0xe7, 0xe7, 0x3c};
GLubyte rovnasa[] = {0x00, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00};


GLubyte numbers[][13] = {
{0x00, 0x00, 0x3c, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x3c, 0x00},	//0
{0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03},
{0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x14, 0x0c, 0x06, 0x04, 0xc3, 0x7e},
{0x00, 0x00, 0x3c, 0xc3, 0xc3, 0x03, 0x03, 0x1f, 0x03, 0x03, 0xc3, 0xc3, 0x3c},
{0x00, 0x00, 0x30, 0x30, 0x30, 0xff, 0xc3, 0xc3, 0xc3, 0xc0, 0xc0, 0xc0, 0xc0},
{0x00, 0x00, 0x3c, 0xc3, 0xc3, 0x0e, 0x38, 0xc0, 0xc0, 0xc0, 0xff, 0xff, 0x00},	//5
{0x00, 0x00, 0x3c, 0xc3, 0xc3, 0x3c, 0x3c, 0xc0, 0xc0, 0x38, 0x38, 0x0e, 0x0e},
{0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x0f, 0x04, 0x04, 0x04, 0x04, 0x3f, 0x00},
{0x00, 0x00, 0x3c, 0xc3, 0xc3, 0x3c, 0x38, 0x3c, 0xc3, 0xc3, 0x3c, 0x00, 0x00},
{0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x3c, 0xc3, 0xc3, 0xc3, 0x3c, 0x00, 0x00} 
};


GLubyte letters[][13] = {
{0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x18},	//A
{0x00, 0x00, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
{0x00, 0x00, 0x7e, 0xe7, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
{0x00, 0x00, 0xfc, 0xce, 0xc7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc7, 0xce, 0xfc},
{0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xc0, 0xff},
{0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xff},
{0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xcf, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
{0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
{0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e},
{0x00, 0x00, 0x7c, 0xee, 0xc6, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06},
{0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xe0, 0xf0, 0xd8, 0xcc, 0xc6, 0xc3},
{0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},
{0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xdb, 0xff, 0xff, 0xe7, 0xc3},
{0x00, 0x00, 0xc7, 0xc7, 0xcf, 0xcf, 0xdf, 0xdb, 0xfb, 0xf3, 0xf3, 0xe3, 0xe3},
{0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e},
{0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
{0x00, 0x00, 0x3f, 0x6e, 0xdf, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c},
{0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
{0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0xe0, 0xc0, 0xc0, 0xe7, 0x7e},
{0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff},
{0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
{0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
{0x00, 0x00, 0xc3, 0xe7, 0xff, 0xff, 0xdb, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
{0x00, 0x00, 0xc3, 0x66, 0x66, 0x3c, 0x3c, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3},
{0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3},
{0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x7e, 0x0c, 0x06, 0x03, 0x03, 0xff}	//X
};

GLuint fontOffset;

void makeRasterFont(void){
	GLuint i, j;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	fontOffset = glGenLists (256);
	for (i = 0,j = 'A'; i < 26; i++,j++) {
		glNewList(fontOffset + j, GL_COMPILE);
		glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, letters[i]);
		glEndList();
	}
	for (i = 0,j = 'a'; i < 26; i++,j++) {
		glNewList(fontOffset + j, GL_COMPILE);
		glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, letters[i]);
		glEndList();
	}
	glNewList(fontOffset + ',', GL_COMPILE);
	glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, ciarka);
	glEndList();
	glNewList(fontOffset + '.', GL_COMPILE);
	glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, ciarka);
	glEndList();
	glNewList(fontOffset + ':', GL_COMPILE);
	glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, dvojbodka);
	glEndList();
	glNewList(fontOffset + ' ', GL_COMPILE);
	glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, space);
	glEndList();
	glNewList(fontOffset + '-', GL_COMPILE);
	glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, pomlcka);
	glEndList();
	glNewList(fontOffset + '+', GL_COMPILE);
	glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, plus);
	glEndList();
	glNewList(fontOffset + '^', GL_COMPILE);
	glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, mocnina);
	glEndList();
	glNewList(fontOffset + '=', GL_COMPILE);
	glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, rovnasa);
	glEndList();
	for (i = 0,j = '0'; i < 10; i++,j++) {
		glNewList(fontOffset + j, GL_COMPILE);
		glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, numbers[i]);
		glEndList();
	}
}
void InitTX(void){
	glPushAttrib (GL_ALL_ATTRIB_BITS);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glDisable(GL_LOGIC_OP);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
	glPixelTransferi(GL_RED_SCALE, 1);
	glPixelTransferi(GL_RED_BIAS, 0);
	glPixelTransferi(GL_GREEN_SCALE, 1);
	glPixelTransferi(GL_GREEN_BIAS, 0);
	glPixelTransferi(GL_BLUE_SCALE, 1);
	glPixelTransferi(GL_BLUE_BIAS, 0);
	glPixelTransferi(GL_ALPHA_SCALE, 1);
	glPixelTransferi(GL_ALPHA_BIAS, 0);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, float(SCREEN_WIDTH), float(SCREEN_HEIGHT), 0.0f, -1.0, 1.0);
}
void EndTX(void){
	glPopMatrix();
	glPopAttrib();
	glMatrixMode(GL_MODELVIEW);
}

void Printf(int X, int Y, char *s, float R, float G, float B, float A){
	float C[]={R,G,B,A};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, C);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, C);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, C);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, C);
	glColor4fv(C);

	glRasterPos2i(X, Y);
	glListBase(fontOffset);
	glCallLists(strlen(s), GL_UNSIGNED_BYTE, (GLubyte *) s);
}
