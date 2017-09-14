#include"init.h"

typedef struct _OBJ{
	M3DMATRIX Rotations;
	M3DVECTOR Velocity;
	M3DVECTOR New_Velocity;
	M3DVECTOR Position;
	mD3DFrame Frame;
	float R, M;	//hmotnost v mega tonach
	float _E;
}mD3DObject;


BOOL INSPHERE=1;
float SPHERE_R=100.0f;

BOOL OVERLAPTEST=1;
BOOL Gravity=1;
BOOL VGravity=0;
M3DVECTOR Gt=M3DVECTOR(0.0f, -9.806f, 0.0f);	//zrychlenie pre VGravity m/(s*s)
float Restitution=0.4f;
float Kapa=6.7f/100.0f;

#define N_BALLS 60		//bacha na MAXNFRAMES !!!
mD3DObject Ball[N_BALLS];
mD3DMesh mBall[N_BALLS];

HRESULT BallsCollision(mD3DObject *B1, mD3DObject *B2){
	M3DVECTOR N;
	float SQM_N;
	float M1=B1->M, M2=B2->M;
	N=M3DVECTOR(B2->Position-B1->Position);
	SQM_N=SquareMagnitude(N);	

	M3DVECTOR V1_1, V1_2, V2_1, V2_2;		//1. zlozka rychlosti 1. obj ...2. ... 
	
	V1_1=(DotProduct(B1->New_Velocity, N)/SQM_N)*N;
	V1_2=B1->New_Velocity+(-DotProduct(N, B1->New_Velocity)/SQM_N)*N;
	V2_1=(DotProduct(B2->Velocity, N)/SQM_N)*N;
	V2_2=B2->Velocity+(-DotProduct(N, B2->Velocity)/SQM_N)*N;
	
	M3DVECTOR x_V1_1, x_V2_1;
	x_V1_1=( (M1-M2)*V1_1+2*M2*V2_1 )/(M1+M2);
	x_V2_1=( (M2-M1)*V2_1+2*M1*V1_1 )/(M1+M2);
		
	B1->New_Velocity=x_V1_1*Restitution+V1_2;

	//tzv rotacia
	M3DVECTOR F1, v1, v2, X;
	//miesto stretu
	X=(N*B1->R)/(B1->R+B2->R)+B1->Position;
	F1=M1*(x_V1_1-V1_1)/Tc;
	v1=(VxM(X-B1->Position, B1->Rotations)+B1->Position)-X;
	v2=(VxM(X-B2->Position, B2->Rotations)+B2->Position)-X;
	
/*	M3DVECTOR vel=(v1+V1_2)-(v2+V2_2);
	float mi=0.01f;
	float Mv=Magnitude(vel);
	M3DVECTOR move, os, F;
	if(Mv==0.0f){F=M3DVECTOR(0.0f, 0.0f, 0.0f);}
	else{
		F=-(vel/Mv)*mi*Magnitude(F1);
	}
	float uhl;
	F_naTeleso(F, X, M1, B1->R, B1->Position, &move, &os, &uhl);
	B1->Rotations=B1->Rotations*POINTROTATE_MATRIX(M3DVECTOR(0.0f, 0.0f, 0.0f), os, uhl*Tc);
*/
	//trenie 100%

	M3DVECTOR vel=(v1+V1_2)-(v2+V2_2);
	M3DVECTOR os;
	float uhl=( Magnitude(vel)*(M2/(M1+M2)) )/(B1->R);
	os=CrossProduct(-vel, -(X-B1->Position));
	
	B1->Rotations=POINTROTATE_MATRIX(M3DVECTOR(0.0f, 0.0f, 0.0f), os, uhl*Tc);

	return MM_OK;
}

HRESULT ComputeScene(void){
	int a,b;
	for(a=0;a<N_BALLS;a++){
		Ball[a].Velocity=Ball[a].New_Velocity;
		Ball[a].Position=Ball[a].Position+Ball[a].Velocity*Tc;	Ball[a].Frame->Position=Ball[a].Position;

		Ball[a].Frame->Orientation=VxM(Ball[a].Frame->Orientation, Ball[a].Rotations);
		Ball[a].Frame->Up=VxM(Ball[a].Frame->Up, Ball[a].Rotations);
	}
	M3DVECTOR temp_V=M3DVECTOR(0.0f, 0.0f, 0.0f);
	for(a=0;a<N_BALLS;a++){
		for(b=0;b<N_BALLS;b++){
			if(a!=b){
				if( SquareMagnitude(M3DVECTOR(Ball[b].Position-Ball[a].Position))<=(Ball[a].R+Ball[b].R)*(Ball[a].R+Ball[b].R) ){
					ATTEMPT(BallsCollision( &Ball[a], &Ball[b]));
				}
				else if(Gravity==1){
					if(Gravity==1){
						float r=Magnitude(Ball[a].Position-Ball[b].Position);
						if(r!=0.0f){
							temp_V+=Tc*((Ball[b]._E)/(r*r*r))*(Ball[a].Position-Ball[b].Position);
						}
					}
				}
			}
		}
		Ball[a].New_Velocity+=temp_V;
		temp_V=M3DVECTOR(0.0f, 0.0f, 0.0f);
	}
	if(VGravity==1){
		for(a=0;a<N_BALLS;a++){Ball[a].New_Velocity+=Tc*Gt;}
	}
	if(OVERLAPTEST==1){
		for(a=0;a<N_BALLS;a++){
			for(b=a+1;b<N_BALLS;b++){
				if( SquareMagnitude(M3DVECTOR(Ball[b].Position-Ball[a].Position))<=(Ball[a].R+Ball[b].R)*(Ball[a].R+Ball[b].R) ){
					//test prekryvania
					M3DVECTOR X=M3DVECTOR(Ball[b].Position-Ball[a].Position);
					float M=Magnitude(X);
					if(M!=0.0f){
						M3DVECTOR P=((Ball[a].R+Ball[b].R-M)*M3DVECTOR(X/M)), P1, P2;
						P1=P*(Ball[a].M/(Ball[a].M+Ball[b].M));
						P2=P*(Ball[b].M/(Ball[a].M+Ball[b].M));	
						Ball[b].Position+=P1;
						Ball[a].Position-=P2;
					}
				}
			}
		}
	}
	if(INSPHERE==1){
		for(a=0;a<N_BALLS;a++){
			if( (Magnitude(Ball[a].Position)+Ball[a].R)>SPHERE_R ){
					//test prekryvania
					M3DVECTOR NormalaS=-Ball[a].Position;
					float Nmagn=Magnitude(NormalaS);
					if(Nmagn!=0.0f){NormalaS/=Nmagn;}

					Ball[a].New_Velocity=(1.0f+Restitution)*DotProduct(-Ball[a].New_Velocity, NormalaS)*NormalaS+Ball[a].New_Velocity;
					Ball[a].Position=-NormalaS*(SPHERE_R-Ball[a].R);
			
					//tzv rotacia
					M3DVECTOR v1, v2, X;
					//miesto stretu
					X=(-NormalaS*Ball[a].R)+Ball[a].Position;
					v1=(VxM(X-Ball[a].Position, Ball[a].Rotations)+Ball[a].Position)-X;
					
					//trenie 100%
					M3DVECTOR N=-NormalaS;
					M3DVECTOR V1_2=Ball[a].New_Velocity+(-DotProduct(N, Ball[a].New_Velocity))*N;
					M3DVECTOR vel=(v1+V1_2);
					M3DVECTOR os;
					float uhl=( Magnitude(vel) )/(Ball[a].R);
					os=CrossProduct(-vel, -(X-Ball[a].Position));
			
					Ball[a].Rotations=POINTROTATE_MATRIX(M3DVECTOR(0.0f, 0.0f, 0.0f), os, uhl*Tc);
			}
		}
	}
	
	return MM_OK;
}


HRESULT InitSys(void){
	int a, r;
	randomize();

	for(a=0;a<N_BALLS;a++){
		Ball[a].New_Velocity=M3DVECTOR(0.0f, 0.0f, 0.0f);
		Ball[a].M=float(rand()%100)*10.0f+1.0f;
		Ball[a].R=float(rand()%200)/100.0f+1.0f;
		Ball[a].Position=M3DVECTOR(float(rand()%6000-3000)/100.0f, float(rand()%6000-3000)/100.0f, float(rand()%2000-1000)/100.0f-30.0f);
		ATTEMPT(CreatemD3DFrame(scene, &(Ball[a].Frame) ));
		ATTEMPT(CreateSphere(Ball[a].R, 15, 15, 0.0f, 0.0f, 1.0f, 1.0f, 2, &mBall[a]));
		r=rand()%3;
		M3DMATERIAL mt;
		if(r==0){ mt=Mat1; }
		else if(r==1){ mt=Mat2; }
		else if(r==2){ mt=Mat3; }
		ATTEMPT(mBall[a]->SetMaterial(mt));
		//r=rand()%3;
		if(r==0){ ATTEMPT(mBall[a]->SetFlags(MD3DMESHF_SPHERMAP)); }
			

		Ball[a].Frame->AddMesh(1, mBall[a]);
		Ball[a].Frame->Position=Ball[a].Position;
		Ball[a]._E=-Kapa*(Ball[a].M);

		if(a==0){
			//ATTEMPT(mBall[a]->SetTexture(0, &SphereMap[0] ));
			//ATTEMPT(mBall[a]->SetFlags(MD3DMESHF_DSPHERMAP));
			mBall[a]->Textures=0;
		}
		else{
			//r=rand()%3;
			mD3DTexture *tx;
			if(r==0){ tx=&text; }
			else if(r==1){ tx=&text1; }
			else if(r==2){ tx=&text2; }
			ATTEMPT(mBall[a]->SetTexture(0, tx));

			r=rand()%3;
			if(r==0){ tx=&text; }
			else if(r==1){ tx=&text1; }
			else if(r==2){ tx=&text2; }
			ATTEMPT(mBall[a]->SetTexture(1, tx));
			
			r=rand()%2;
			mBall[a]->Textures=1+r;
		}
		mBall[a]->TextureEnvMode[0]=GL_MODULATE;
		mBall[a]->TextureEnvMode[1]=GL_MODULATE;

		Ball[a].Rotations=IdentityMatrix();
	}

	/*Ball[1].Velocity=M3DVECTOR(-0.5f, 0.0f, 0.0f);
	Ball[1].M=10000000.0f;
	Ball[1].R=5.0f;
	Ball[1].Position=M3DVECTOR(35.0f, 0.0f, -50.0f);
	ATTEMPT(CreatemD3DFrame(scene, &Ball[1].Frame));
	CreateSphere(Ball[1].R, 20, 20, 0.0f, 0.0f, 1.0f, 1.0f, &mBall[1]);
	Ball[1].Frame->AddMesh(1, mBall[1]);
	Ball[1].Frame->Position=Ball[1].Position;
	Ball[1]._E=-Kapa*(Ball[1].M);
	ATTEMPT(mBall[1]->SetTexture(0,text1));
	mBall[1]->Textures=1;
	*/

	return MM_OK;
}

M3DVECTOR LinePlane(M3DVECTOR P, M3DVECTOR n, M3DVECTOR A, M3DVECTOR u){
	float D=-n.x*P.x-n.y*P.y-n.z*P.z;
	float Dot_nu=DotProduct(n, u);
	if(Dot_nu==0.0f){return M3DVECTOR(99999999.0f, 99999999.0f, 99999999.0f);}
	float t;
	t=-(DotProduct(n, A)+D)/Dot_nu;
	return M3DVECTOR(A+t*u);
}

float LineSphere(M3DVECTOR A, M3DVECTOR u, mD3DObject obj){
	if(u==M3DVECTOR(0.0f, 0.0f, 0.0f)){return -9999999.0f;}
	float D, a, b,c;
	a=SquareMagnitude(u);
	b=2*DotProduct(A, u)-2*DotProduct(obj.Position, u);
	c=SquareMagnitude(A)+SquareMagnitude(obj.Position)-obj.R*obj.R-2*DotProduct(A, obj.Position);
	D=b*b-4*a*c;
	return D;
}
mD3DObject* ComputeIntersection(M3DVECTOR A, M3DVECTOR u){
	int a;
	mD3DObject *Obj=NULL;
	float Dist=P_FPlane, NDist;
	for(a=0;a<N_BALLS;a++){
		if(LineSphere(A, u, Ball[a])>=0){
			NDist=SquareMagnitude(Ball[a].Position-A);
			if(NDist<Dist){
				Obj=&Ball[a];
				Dist=NDist;
			}
		}
	}
	return Obj;
}
