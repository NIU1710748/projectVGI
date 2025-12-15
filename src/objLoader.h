//******** PRACTICA VISUALITZACIÓ GRÀFICA INTERACTIVA (Escola Enginyeria - UAB)
//******** Entorn bàsic VS2022 MONOFINESTRA amb OpenGL 4.6, interfície GLFW 3.4, ImGui i llibreries GLM
//******** Enric Martí (Setembre 2025)
// objLoader.h: Inteface of the class COBJModel.
//
//	  Versió 2.0:	- Adaptació funcions a crear un VAO per a cada material del fitxer
//////////////////////////////////////////////////////////////////////////////////////

#ifndef OBJLOADER_H
#define OBJLOADER_H

#define OBJLOADER_CLASS_DECL     __declspec(dllexport)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "OBB.h"

// Màxima mida vector VAOList
#define MAX_SIZE_VAOLIST 125

// Needed structures
struct Vector3D
{
	float fX;
	float fY;
	float fZ;
};

struct Vector2D
{
	float fX;
	float fY;
};

struct OBJFileInfo
{
	unsigned int iVertexCount;
	unsigned int iTexCoordCount;
	unsigned int iNormalCount;
	unsigned int iFaceCount;
	unsigned int iMaterialCount;
};

struct Face
{
	unsigned int	iNumVertices;
	unsigned int	iMaterialIndex;
	Vector3D* pVertices;
	Vector3D* pNormals;
	Vector2D* pTexCoords;
};

struct Material
{
	char	szName[1024];		 // Name of the material
	char	szTexture[_MAX_PATH];// Filename of the texture
	uint	iTextureID;			 // OpenGL name of the texture
	float fDiffuse[3];		 // Diffuse component
	float fAmbient[3];		 // Ambient component
	float fSpecular[3];		 // Specular component
	float fEmmissive[3];		 // Emmissive component
	float fShininess;			 // Specular exponent
};

class OBJLOADER_CLASS_DECL COBJModel
{
public:
	bool isHitboxActive() const { return hitboxActive; };
	bool isHitbox() const { return hitbox; };
	void setHitboxActive(bool hitboxState) { hitboxActive = hitboxState; };
	void setAsHitbox(bool hit = true) { hitbox = hit; };
	AABB* getAABB() const { return AABBhitbox; };
	OBB* getOBB() const { return OBBhitbox; };
	bool isRendering() const { return render; };
	void changeRendering(bool renderValue) { render = renderValue; };
	void setName(std::string newName) { name = newName; };
	std::string getName() { return name; };

	void drawOBB(GLuint shader_programID, const glm::mat4& modelMatrix);
	void updateOBBWorld();

	void _stdcall DrawModel(int prim_Id);
	int _stdcall LoadModel(char* szFileName);
	_stdcall COBJModel();
	virtual _stdcall ~COBJModel();
	void _stdcall EliminaLlista(int prim_Id);

	// Funcions CVAO
	void _stdcall netejaVAOList_OBJ();
	void _stdcall netejaTextures_OBJ();
	void _stdcall draw_TriVAO_OBJ(GLuint sh_programID);


	// Get transformation matrix for the object
	glm::mat4 modelMatrix() const {
		glm::mat4 M(1.0f);
		M = glm::translate(M, position);
		//M = glm::rotate(M, glm::radians(rotation.x), glm::vec3(1, 0, 0));
		//M = glm::rotate(M, glm::radians(rotation.y), glm::vec3(0, 1, 0));
		//M = glm::rotate(M, glm::radians(rotation.z), glm::vec3(0, 0, 1));
		M = M * glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
		M = glm::scale(M, scale);
		return M;
	}

	void rotateX(float deg) { rotation.x += glm::radians(deg); };
	void rotateY(float deg) { rotation.y += glm::radians(deg); };
	void rotateZ(float deg) { rotation.z += glm::radians(deg); };
	void rescale(float factor) { scale = glm::vec3(factor); };
	void move(float Xf, float Yf, float Zf) { position += glm::vec3(Xf, Yf, Zf); };

private:
	void _stdcall ReadNextString(char szString[], FILE* hStream);
	//	  int _stdcall LoadTexture(const char szFileName[_MAX_PATH]);
	int _stdcall LoadTexture2(const char szFileName[_MAX_PATH]);
	void _stdcall UseMaterial(const Material* pMaterial);
	void _stdcall UseMaterial_ShaderID(GLuint sh_programID, Material pMaterial);
	void _stdcall GetTokenParameter(char szString[], const unsigned int iStrSize, FILE* hFile);
	void _stdcall MakePath(char szFileAndPath[]);
	bool _stdcall LoadMaterialLib(const char szFileName[], Material* pMaterials,
		unsigned int* iCurMaterialIndex, char szBasePath[]);
	CVAO _stdcall RenderToVAOList(const Face* pFaces, const unsigned int iFaceCount,
		const Material* pMaterials);
	void _stdcall loadToVAOList(const Face* pFaces, const unsigned int iFaceCount,
		const Material* pMaterials);
	void _stdcall GetFaceNormal(float fNormalOut[3], const Face* pFace);
	void _stdcall ParseFaceString(char szFaceString[], Face* FaceOut, const Vector3D* pVertices,
		const Vector3D* pNormals, const Vector2D* pTexCoords, const unsigned int iMaterialIndex);
	void _stdcall GetFileInfo(FILE* hStream, OBJFileInfo* Stat, const char szConstBasePath[]);
	void _stdcall GenTexCoords();


	// CVAO
	GLint numMaterials = 0;
	int vector_Materials[MAX_SIZE_VAOLIST];
	CVAO VAOList_OBJ[MAX_SIZE_VAOLIST];
	Material vMaterials[MAX_SIZE_VAOLIST];
	AABB* AABBhitbox;
	OBB* OBBhitbox;
	OBB* worldOBB;
	bool hitbox;
	bool hitboxActive;
	bool render;
	std::string name;

	// Transformation matrix for the object
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f); // Euler angles in degrees
	glm::vec3 scale = glm::vec3(1.0f);

	//glm::vec3 center;



// Funcions CVAO
	void _stdcall initVAOList_OBJ();
	void _stdcall Set_VAOList_OBJ(GLint k, CVAO auxVAO);
	void _stdcall deleteVAOList_OBJ(GLint k);
	void _stdcall draw_TriVAO_Object_OBJ(GLint k);
};

// Loads paths of all obj files
void loadObjPaths(const std::string& folder, std::vector<std::string>& paths);
void loadObjPathsRec(const std::string& folder, std::vector<std::pair<std::string, std::string>>& paths); // Recursive version. Loads subfolders too

// Callback function for comparing two faces with qsort
static int CompareFaceByMaterial(const void* Arg1, const void* Arg2);

//Returns a newly created COBJModel object. 
OBJLOADER_CLASS_DECL COBJModel* _stdcall InitObject();

//Destroys a COBJModel object
OBJLOADER_CLASS_DECL void _stdcall UnInitObject(COBJModel* obj);

#endif // !defined(AFX_OBJMODEL_H__32C5F722_AD3D_11D1_8F4D_E0B57CC10800__INCLUDED_)
