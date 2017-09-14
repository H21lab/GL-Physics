#include "loadvrml.h"

/***********ROZKLAD VECTORA na 2 vektory - v rovine *********/
HRESULT rozkladV2(M3DVECTOR S, M3DVECTOR V1, M3DVECTOR V2, M3DVECTOR * Roz1, M3DVECTOR * Roz2){
	float p,q;
	p=DotProduct(V1,S)/(V1.x*V1.x+V1.y*V1.y+V1.z*V1.z);
	q=DotProduct(V2,S)/(V2.x*V2.x+V2.y*V2.y+V2.z*V2.z);
	Roz1[0]=V1*p;
	Roz2[0]=V2*q;
	return MM_OK;
}
/************************************************************/
HRESULT F_naTeleso(M3DVECTOR F, M3DVECTOR miestoPosobenia, float hmotnost, float ppolomer, M3DVECTOR Tazisko, M3DVECTOR * MV, M3DVECTOR * os, float * uhlzrychl){
	M3DVECTOR odstredivyV,otocnyV,osOtoc;
	M3DVECTOR MoveV,RotV;
	float uzrychl,ModstredivyV;
	//kontrola udajov
	if(hmotnost<=0.0f || ppolomer<=0.0f){
		MoveV=M3DVECTOR(0.0f,0.0f,0.0f);
		uhlzrychl[0]=0.0f;
		os[0]=M3DVECTOR(0.0f,0.0f,0.0f);
		return MM_OK;
	}
	//	P=(D-S)				 
	odstredivyV=M3DVECTOR(miestoPosobenia-Tazisko);
	osOtoc=CrossProduct(F, odstredivyV);
	if(osOtoc.x==0.0f&&osOtoc.y==0.0f&&osOtoc.z==0.0f){
		MoveV=F/hmotnost;
		os[0]=osOtoc;
		uhlzrychl[0]=0.0f;
		return MM_OK;
	}
	ModstredivyV=Magnitude(odstredivyV);
	osOtoc=Normalize(osOtoc);	
	os[0]=osOtoc;
	otocnyV=CrossProduct(osOtoc, M3DVECTOR(odstredivyV));
	rozkladV2(F,M3DVECTOR(odstredivyV),otocnyV,&MoveV,&RotV);
	
	uzrychl=5*Magnitude(RotV)*ModstredivyV;
	uzrychl=-uzrychl/(2.0f*hmotnost*ppolomer*ppolomer);
	uhlzrychl[0]=uzrychl;
	
	MoveV=MoveV/hmotnost;
	MV[0]=MoveV;
	return MM_OK;
}
