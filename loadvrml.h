#include"eng.h"

#define VRML_LINE_LENGTH 200
#define VRML_WORD_LENGTH 25
#define VRML_MAX_PATH 50

char *Buffer;
char BufferLine[VRML_LINE_LENGTH];
char BufferWord[VRML_WORD_LENGTH];
long flength;
long Pointer;
char err[80];	//error message

BOOL ReadLine(){
	BufferLine[0]=='\0';
	int i=0;
	do
		BufferLine[i++] = Buffer[Pointer++];
	while( (Buffer[Pointer-1] != '\n') && (i < VRML_LINE_LENGTH) && (Pointer < flength) );

	BufferLine[i-1] = '\0';
	return 1;
}
BOOL ReadWord(){
	BufferWord[0] = '\0';
	int i=0;
	// Jump to next valid character
	while( (Buffer[Pointer] == '\n' || Buffer[Pointer] == '\t' || Buffer[Pointer] == '\r' || Buffer[Pointer] == ' ') && Pointer < flength){
		Pointer++;
	}
	// Check eof
	if(Pointer >= flength) return 0;
	do
		BufferWord[i++] = Buffer[Pointer++];
	while(Buffer[Pointer-1] != '\n' && Buffer[Pointer-1] != '\t' && Buffer[Pointer-1] != '\r' && Buffer[Pointer-1] != ' ' && i < VRML_WORD_LENGTH && Pointer < flength);

	BufferWord[i-1] = '\0';
	return 1;
}
BOOL OffsetToStringBeginLine(char *string){
	while(Pointer<flength){
		ReadLine();
		if(strncmp(BufferLine, string, strlen(string))==0)
		{
			Pointer -= strlen(BufferLine)+1;
			return 1;
		}
	}
	return 0;
}
BOOL OffsetToString(char *string){
	while(Pointer<flength){
		ReadLine();
		char *adr = strstr(BufferLine,string);
		if(strstr(BufferLine,string) != NULL){
			Pointer = Pointer - strlen(BufferLine) - 1 + (adr-BufferLine);
			if(Pointer < 0){Pointer=0;}
			return 1;
		}
	}
	return 0;
}
BOOL OffsetToStringBefore(char *string, char *before){
	while(Pointer < flength){
		ReadLine();
		char *adr = strstr(BufferLine,string);
		if(strstr(BufferLine,before) != NULL)	return 0;
		if(strstr(BufferLine,string) != NULL){
			Pointer = Pointer - strlen(BufferLine) - 1 + (adr-BufferLine);
			if(Pointer<0){return 0;}
			return 1;
		}
	}
	return 0;
}
BOOL CheckMesh(){
	OffsetToStringBeginLine("DEF");
	int tmp = Pointer;
	ReadLine();
	if(strstr(BufferLine,"DEF") != NULL && strstr(BufferLine,"Transform") != NULL){
		Pointer = tmp;
		if(OffsetToString("Transform") && OffsetToString("Material") && OffsetToString("IndexedFaceSet")){
			Pointer = tmp;
			return 1;
		}
	}
	return 0;
}

HRESULT SizeMesh(int *pNbVertex, int *pNbFace, int HasTexture, int *pNbTextureCoordinate /* = NULL */){
	int tmp = Pointer;

	if(pNbVertex == NULL){return MM_INVALID_PARAMS;}
	if(pNbFace == NULL){return MM_INVALID_PARAMS;}

	if(!OffsetToString("IndexedFaceSet")){
		sprintf(&err[0], "Invalid mesh <IdexedFaceSet> not found\n"); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;}
	// Count points
	//***********************************************
	
	if(!OffsetToString("Coordinate { point [")){
		sprintf(&err[0], "Invalid mesh <Coordinate { point [> not found\n"); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;}

	Pointer += strlen("Coordinate { point [") + 1;

	// Cur : x y z,
	// End : x y z]
	int NbVertex = 0;
	int success;
	do{	float x,y,z;
		ReadWord();
		success = sscanf(BufferWord,"%f",&x);
		ReadWord();
		success &= sscanf(BufferWord,"%f",&y);
		ReadWord();
		success &= sscanf(BufferWord,"%f",&z);
		NbVertex += success;
	}
	while(success);
	if(NbVertex <= 0){return MM_ERROR;}

	// Count texture coordinates, if needed 
	//***********************************************
	int NbTextureCoordinate = 0;
	if(HasTexture){
		if(!OffsetToString("TextureCoordinate { point [")){return MM_ERROR;}

		Pointer += strlen("TextureCoordinate { point [") + 1;

		// Cur : x y,
		// End : x y]
		int success;
		do{	float x,y;
			ReadWord();
			success = sscanf(BufferWord,"%f",&x);
			ReadWord();
			success &= sscanf(BufferWord,"%f",&y);
			NbTextureCoordinate += success;
		}
		while(success);
		if(NbTextureCoordinate <= 0)return MM_ERROR;
	}

	// Count faces, accept only triangles
	//***********************************************
	Pointer = tmp;
	if(!OffsetToString("coordIndex [")){
		sprintf(&err[0], "Invalid mesh <coordIndex[> not found\n"); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;}
	Pointer += strlen("coordIndex [") + 1;

	// Cur : int, int, int, -1,
	// End : int, int, int, -1]
	int NbFace = 0;
	do
	{
		int v1,v2,v3;
		ReadWord();
		success  = sscanf(BufferWord,"%d,",&v1);
		ReadWord();
		success &= sscanf(BufferWord,"%d,",&v2);
		ReadWord();
		success &= sscanf(BufferWord,"%d,",&v3);
		NbFace += success;

		if(v1 < 0){return MM_ERROR;}
		if(v2 < 0){return MM_ERROR;}
		if(v3 < 0){return MM_ERROR;}
			
		int test;
		ReadWord();
		sscanf(BufferWord,"%d",&test);
		if(strstr(BufferWord,"]") != NULL)	success = 0;
	}
	while(success);
	
	if(NbFace <= 0){return MM_ERROR;}

	// Count texture coordinate index 
	//***********************************************
	if(HasTexture)
	{
		Pointer = tmp;
		if(!OffsetToString("texCoordIndex [")){
			sprintf(&err[0], "Invalid mesh <texCoordIndex [> not found\n"); 
			ShowWindow(hWnd, 0); UpdateWindow(hWnd);
			MessageBox(NULL, err,"Error",MB_OK);
			return MM_HIDDEN;}
		Pointer += strlen("texCoordIndex [") + 1;

		// Cur : int, int, int, -1,
		// End : int, int, int, -1]
		int NbCoordIndex = 0;
		do
		{	int v1,v2,v3;
			ReadWord();
			success  = sscanf(BufferWord,"%d,",&v1);
			ReadWord();
			success &= sscanf(BufferWord,"%d,",&v2);
			ReadWord();
			success &= sscanf(BufferWord,"%d,",&v3);
			NbCoordIndex += success;

			if(v1 < 0){return MM_ERROR;}
			if(v2 < 0){return MM_ERROR;}
			if(v3 < 0){return MM_ERROR;}
			
			int test;
			ReadWord();
			sscanf(BufferWord,"%d",&test);
			if(strstr(BufferWord,"]") != NULL)
				success = 0;
		}
		while(success);
		if(NbFace != NbCoordIndex)
		{	return MM_ERROR;		}
	}

	// Store result
	*pNbVertex = NbVertex;
	*pNbFace = NbFace;
	if(HasTexture)	*pNbTextureCoordinate = NbTextureCoordinate;
	
	Pointer = tmp;
	return MM_OK;
}
typedef struct _VRX{
	M3DVECTOR pos;
	M3DVECTOR nor;
	float u,v;
}VRX;
HRESULT StoreMesh(VRX *vertex, DWORD *data, int HasTexture){
	int tmp = Pointer;
	if(!OffsetToString("IndexedFaceSet")){
		sprintf(&err[0], "Invalid mesh <IndexedFaceSet> not found\n"); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;}
	// Store vertices
	//***********************************************
	if(!OffsetToString("Coordinate { point [")){
		sprintf(&err[0], "Invalid mesh <Coordinate { point [> not found\n"); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;}
	Pointer += strlen("Coordinate { point [") + 1;
	// Cur : x y z,
	// End : x y z]
	int success;
	int NbVertex = 0;
	do{	float x,y,z;
		ReadWord();
		success = sscanf(BufferWord,"%f",&x);
		ReadWord();
		success &= sscanf(BufferWord,"%f",&y);
		ReadWord();
		success &= sscanf(BufferWord,"%f",&z);
		if(success){vertex[NbVertex].pos=M3DVECTOR(x,y,z); NbVertex++;}
	}
	while(success);

	// Store texture coordinates (if needed)
	//***********************************************
/*	if(HasTexture)
	{
		if(!OffsetToString("TextureCoordinate { point [")){
			sprintf(&err[0], "Invalid mesh <TextureCoordinate { point [> not found\n"); 
			ShowWindow(hWnd, 0); UpdateWindow(hWnd);
			MessageBox(NULL, err,"Error",MB_OK);
			return MM_HIDDEN;}
	
		Pointer += strlen("TextureCoordinate { point [") + 1;
		// Cur : x y,
		// End : x y
		int success;
		int NbTextureCoordinate = 0;
		do{	float x,y;
			ReadWord();
			success = sscanf(BufferWord,"%f",&x);
			ReadWord();
			success &= sscanf(BufferWord,"%f",&y);
			if(success){
				vertex[NbTextureCoordinate].u = x;
				vertex[NbTextureCoordinate].v = y;
				NbTextureCoordinate++;
			}
		}
		while(success);
	}
*/	
	// Store faces, accept only triangles
	//***********************************************
	Pointer = tmp;
	if(!OffsetToString("coordIndex [")){
		sprintf(&err[0], "Invalid mesh <coordIndex [> not found\n"); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;}
	Pointer += strlen("coordIndex [") + 1;

	// Cur : int, int, int, -1,
	// End : int, int, int, -1]
	int NbFace = 0;
	do{	int v1,v2,v3;
		ReadWord();
		success  = sscanf(BufferWord,"%d,",&v1);
		ReadWord();
		success &= sscanf(BufferWord,"%d,",&v2);
		ReadWord();
		success &= sscanf(BufferWord,"%d,",&v3);

		if(v1 < 0){v1=0;}
		if(v2 < 0){v2=0;}
		if(v3 < 0){v3=0;}

		if(success && v1 >= 0 && v2 >= 0 && v3 >= 0)	{
			data[NbFace*4]=3;
			data[NbFace*4+1]=DWORD(v1);
			data[NbFace*4+2]=DWORD(v2);
			data[NbFace*4+3]=DWORD(v3);
			NbFace++;
		}
		int test;
		ReadWord();
		sscanf(BufferWord,"%d",&test);
		if(strstr(BufferWord,"]") != NULL)
			success = 0;
	}
	while(success);
	data[(NbFace)*4]=0;

	// Store texture coord index
	//***********************************************
	if(HasTexture)
	{
		Pointer = tmp;
		if(!OffsetToString("texCoordIndex ["))
		{	sprintf(&err[0], "Invalid mesh <texCoordIndex [> not found\n"); 
			ShowWindow(hWnd, 0); UpdateWindow(hWnd);
			MessageBox(NULL, err,"Error",MB_OK);
			return MM_HIDDEN;}
		Pointer += strlen("texCoordIndex [") + 1;

		// Cur : int, int, int, -1,
		// End : int, int, int, -1]
//                int NbTexCoordIndex = 0;
		do
		{
			int v1,v2,v3;
			ReadWord();
			success  = sscanf(BufferWord,"%d,",&v1);
			ReadWord();
			success &= sscanf(BufferWord,"%d,",&v2);
			ReadWord();
			success &= sscanf(BufferWord,"%d,",&v3);

			if(v1 < 0){v1=0;}
			if(v2 < 0){v2=0;}
			if(v3 < 0){v3=0;}

			if(success && v1 >= 0 && v2 >= 0 && v3 >= 0)	{
			//	pTextureCoordinateIndex[3*NbTexCoordIndex] = v1;
			//	pTextureCoordinateIndex[3*NbTexCoordIndex+1] = v2;
			//	pTextureCoordinateIndex[3*NbTexCoordIndex+2] = v3;
//                              NbTexCoordIndex++;
			}

			int test;
			ReadWord();
			sscanf(BufferWord,"%d",&test);
			if(strstr(BufferWord,"]") != NULL)
				success = 0;
		}
		while(success);
	}
	return MM_OK;
}
HRESULT ReadMesh(mD3DFrame frame){
	// Check
	if(!CheckMesh()){return MM_ERROR;}
	int tmp = Pointer;
	ReadLine();
	if(strstr(BufferLine,"DEF") == NULL){return MM_ERROR;}
	if(strstr(BufferLine,"Transform") == NULL){return MM_ERROR;}
	// DEF [name] Transform {
	if(sscanf(BufferLine,"DEF %s Transform", BufferWord) != 1){
		sprintf(&err[0], "Invalid syntax in VRML file (Line: %s)\n", BufferLine); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;
	}
	Pointer = tmp;

//        int IndexTexture = -1;
	
	mD3DFrame FR=NULL;
	ATTEMPT(CreatemD3DFrame(frame, &FR));

	
	// Transform
	//********************************************
	// Syntax :
	// Transform {
		// translation -360.7 1370 3471
		// rotation 0.3236 -0.3236 -0.8891 -1.688
		// scale -49.36 -49.36 -49.36
		// scaleOrientation -0.689 0.4766 -0.546 -0.6007

	OffsetToString("Transform");
	ReadLine(); // Transform
	
	// Translation
	tmp = Pointer;
	ReadLine();
	if(strstr(BufferLine,"translation") != NULL){
		// Come back
		Pointer = tmp;
		// Jump after "translation"
		ReadWord(); 

		float x,y,z;
		ReadWord();
		int success = sscanf(BufferWord,"%f",&x);
		ReadWord();
		success &= sscanf(BufferWord,"%f",&y);
		ReadWord();
		success &= sscanf(BufferWord,"%f",&z);
		if(success){FR->Position=M3DVECTOR(x,y,z);}
		ReadLine();
		tmp = Pointer;
		ReadLine();
	}
	// Rotation
	if(strstr(BufferLine,"rotation") != NULL)	{
		// Come back
		Pointer = tmp;
		// Jump after "rotation"
		ReadWord(); 

//                float x,y,z,value;
		ReadWord();
//                int success = sscanf(BufferWord,"%f",&x);
		ReadWord();
//                success &= sscanf(BufferWord,"%f",&y);
		ReadWord();
//                success &= sscanf(BufferWord,"%f",&z);
		ReadWord();
//                success &= sscanf(BufferWord,"%f",&value);
		//if(success){transform.SetRotation(CVector3d(x,y,z));
		//	transform.SetValueRotation(value/3.1415926f*180.0f);
		//	TRACE("    rotation : %g %g %g %g\n",x,y,z,value);
		//}
		ReadLine();
		tmp = Pointer;
		ReadLine();
	}
	// Scale
	if(strstr(BufferLine,"scale") != NULL){
		// Come back
		Pointer = tmp;
		// Jump after "scale"
		ReadWord(); 

//                float x,y,z;
		ReadWord();
//                int success = sscanf(BufferWord,"%f",&x);
		ReadWord();
//                success &= sscanf(BufferWord,"%f",&y);
		ReadWord();
//                success &= sscanf(BufferWord,"%f",&z);
		//if(success){
		//	transform.SetScale(CVector3d(x,y,z));
		//	TRACE("    scale : %g %g %g\n",x,y,z);
		//}
		ReadLine();
		tmp = Pointer;
		ReadLine();
	}
	// ScaleOrientation
	if(strstr(BufferLine,"scaleOrientation") != NULL){
		// Come back
		Pointer = tmp;
		// Jump after "scaleOrientation"
		ReadWord(); 

//                float x,y,z,value;
		ReadWord();
//                int success = sscanf(BufferWord,"%f",&x);
		ReadWord();
//                success &= sscanf(BufferWord,"%f",&y);
		ReadWord();
//                success &= sscanf(BufferWord,"%f",&z);
		ReadWord();
//                success &= sscanf(BufferWord,"%f",&value);
		//if(success){
		//	transform.SetScale(CVector3d(x,y,z));
		//	TRACE("    scaleOrientation : %g %g %g %g\n",x,y,z,value);
		//}
		ReadLine();
	}

	// Material
	// appearance Appearance {
		// material Material {
		// diffuseColor 0.5686 0.1098 0.6941
	if(OffsetToString("Material")){
		ReadLine();
		tmp = Pointer;

		// Diffuse color
		ReadLine(); 
		if(strstr(BufferLine,"diffuseColor") != NULL){
			// Come back
			Pointer = tmp;
			// Jump
			ReadWord();
//                        float r,g,b;
			ReadWord();
//                        int success = sscanf(BufferWord,"%f",&r);
			ReadWord();
//                        success &= sscanf(BufferWord,"%f",&g);
			ReadWord();
//                        success &= sscanf(BufferWord,"%f",&b);
			//if(success){
			//	material.SetDiffuse(r,g,b,1.0f);
			//	TRACE("    diffuseColor : %g %g %g\n",r,g,b);
			//}
		}
	}
	// Texture
	int texture = 0;
	if(OffsetToStringBefore("texture ImageTexture","geometry")){
		texture = 1;
		ReadLine();
		tmp = Pointer;

		ReadLine(); 
		if(strstr(BufferLine,"url") != NULL){
			// Come back
			Pointer = tmp;
			// Jump
			ReadWord();
  //                      char string[VRML_MAX_PATH];
			ReadWord();
        //                int success = sscanf(BufferWord,"%s",string);

			// Remove ""
			//CString TextureName = string;
			//TextureName = TextureName.Mid(1,TextureName.GetLength()-2);
			//TRACE("    texture : %s\n",TextureName);

			// Ask SceneGraph to add texture, if needed
			//char *name = TextureName.GetBuffer(MAX_PATH);
			//if(!pSceneGraph->HasTexture(name,&IndexTexture))
			//{
			//	CTexture *pTexture = new CTexture;
			//	pTexture->ReadFile(name);
			//	IndexTexture = pSceneGraph->AddTexture(pTexture);
			//}
			//TextureName.ReleaseBuffer();
		}
	}
	else{ // come back
		Pointer = tmp;
	}
	// Mesh

	int NbVertex,NbFace,NbTextureCoordinate;
	// Count size (do not offset in file)
	ATTEMPT(SizeMesh(&NbVertex,&NbFace,texture,&NbTextureCoordinate));
	VRX *vertex;
	vertex=(VRX*)malloc(NbVertex*sizeof(VRX));
	if(vertex==NULL){return MM_OUT_OF_MEMORY;}
	DWORD *data;
	data=(DWORD*)malloc(sizeof(DWORD)*(NbFace*4+1));	//3 vrcholy + terminal
	if(data==NULL){return MM_OUT_OF_MEMORY;}


//	if(texture)
//	{
//		pMesh->m_pTextureCoordinate = new float[NbTextureCoordinate*2]; // x y 
//		pMesh->m_pTextureCoordinateIndex = new int[NbFace*3];           // triangular faces
//		pMesh->m_IndexTexture = IndexTexture;
//	}
	// Store mesh (offset in file)
	ATTEMPT(StoreMesh(vertex, data, texture));

	mD3DMesh mesh=NULL;
	ATTEMPT(CreatemD3DMesh(M3DFVF_XYZ|M3DFVF_NORMAL|M3DFVF_TEX1, &vertex[0], &data[0], &mesh));
	ATTEMPT(mesh->GenerateNormals());
	ATTEMPT(FR->AddMesh(1, mesh));	

	free(vertex);
	free(data);
	// Transform & material
	//pMesh->SetTransform(transform);
	//pMesh->SetMaterial(&material);
	return MM_OK;
}

HRESULT LoadVRML(LPCSTR name, mD3DFrame frame){
	if(frame==NULL){return MM_INVALID_PARAMS;}
	FILE *file;
	file=fopen(name, "rb");
	if(file==NULL){
		sprintf(&err[0], "Unable to open file %s\n", name); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;
	}
	Pointer=0; flength=0;
	memset(BufferLine, 0, VRML_LINE_LENGTH);
	memset(BufferWord, 0, VRML_WORD_LENGTH);

	fseek(file, 0l, SEEK_END);
	flength=ftell(file);
	fseek(file, 0l, SEEK_SET);
	if(flength==0){flength=1;}
	
	Buffer=NULL;
	Buffer=(char*)malloc(flength);
	if(Buffer==NULL){
		sprintf(&err[0], "Insuffisant memory, unable to load %s\n", name); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;
	}
	if(fread(Buffer, 1, flength, file)<_SIZE_T(flength)){
		sprintf(&err[0], "Unable to read from %s\n", name); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;
	}
	fclose(file);

	//Check Version
	ReadLine();
	if(strstr(BufferLine,"#VRML V2.0") == NULL){
		sprintf(&err[0], "Invalid VRML version ( %s )\n", name); 
		ShowWindow(hWnd, 0); UpdateWindow(hWnd);
		MessageBox(NULL, err,"Error",MB_OK);
		return MM_HIDDEN;
	}
	//
	while(OffsetToStringBeginLine("DEF")){
		ATTEMPT(ReadMesh(frame));
	}

 	free(Buffer);
	return MM_OK;
}
