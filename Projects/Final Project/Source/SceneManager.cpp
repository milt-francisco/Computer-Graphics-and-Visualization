///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials() {

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	metalMaterial.ambientStrength = 0.1f;
	metalMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	metalMaterial.specularColor = glm::vec3(0.6f, .6f, .6f);
	metalMaterial.shininess = 24.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial.ambientStrength = 0.5f;
	woodMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	woodMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	woodMaterial.shininess = 15.0;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	backdropMaterial.ambientStrength = 0.5f;
	backdropMaterial.diffuseColor = glm::vec3(0.06f, 0.05f, 0.01f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 0.0;
	backdropMaterial.tag = "backdrop";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL cubeMaterial;
	cubeMaterial.ambientColor = glm::vec3(.5f, .5f, .5f);
	cubeMaterial.ambientStrength = 0.1;
	cubeMaterial.diffuseColor = glm::vec3(.1f, .1f, .1f);
	cubeMaterial.specularColor = glm::vec3(.2f, .2f, .2f);
	cubeMaterial.shininess = 22.0f;
	cubeMaterial.tag = "cube";

	m_objectMaterials.push_back(cubeMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";
}


/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights() {
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("lightSources[0].position", 50.0f, 4.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.08f,0.08f, 0.08f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.08f, 0.07f, 0.06f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, .9f, 0.8f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", .8f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 10.0f, 4.0f, 20.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.75f, 0.75f, 0.75f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.6f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, .4f, 0.4f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", .4f);

}

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	// Tumbler Texture
	bReturn = CreateGLTexture("../../Utilities/textures/tumbler.jpg", "tumbler");

	// Table texture
	bReturn = CreateGLTexture("../../Utilities/textures/woodTable.jpg", "table");

	// Wall and Floor textures
	bReturn = CreateGLTexture("../../Utilities/textures/wall.jpg", "wall");
	bReturn = CreateGLTexture("../../Utilities/textures/carpet.jpg", "floor");

	// Placeholder 5 textures for rubiks cube
	bReturn = CreateGLTexture("../../Utilities/textures/yellowFace.jpg", "yellowFront");
	bReturn = CreateGLTexture("../../Utilities/textures/blueFace.jpg", "blueRight");
	bReturn = CreateGLTexture("../../Utilities/textures/redFace.jpg", "redTop");
	bReturn = CreateGLTexture("../../Utilities/textures/greenFace.jpg", "greenLeft");
	bReturn = CreateGLTexture("../../Utilities/textures/whiteFace.jpg", "whiteBack");

	// Placeholder 2 textures for book
	bReturn = CreateGLTexture("../../Utilities/textures/bookCover.png", "Cover");
	bReturn = CreateGLTexture("../../Utilities/textures/BookCoverSide.png", "CoverSide");
	bReturn = CreateGLTexture("../../Utilities/textures/bookPages.png", "bookPages");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load the texture image files for the textures applied to the objects
	LoadSceneTextures();

	// Define the materials that will be used for the objects
	DefineObjectMaterials();

	// Add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	CreateBackground();
	CreateTable();
	CreateTumbler();
	CreateRubiksCube();
	CreateBook();
	CreateDiffuser();
	/*************** Used during development only *******************/
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	/****************************************************************/
}

/***********************************************************
 *  CreateBackground()
 *
 *  This method is used for rendering the wall and floor
 *  of the 3D Scene
 ***********************************************************/
void SceneManager::CreateBackground() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// LOWER Plane (Floor)
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -3.25f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(((float)180 / 255), ((float)153 / 255), ((float)135 / 255), 1);
	SetShaderTexture("floor");
	SetTextureUVScale(4.0, 4.0);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// UPPER Plane (Wall)
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 5.5f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(((float)224 / 255), ((float)213 / 255), ((float)193 / 255), 1);

	// Set texture material
	//SetShaderTexture("wall");
	SetTextureUVScale(4.0, 4.0);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
}

/***********************************************************
 *  CreateTable()
 *
 *  This method creates the table object that all of the
 *  main objects are placed on.
 ***********************************************************/
void SceneManager::CreateTable() {

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// TABLE
	// Leg Front Left
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.75f, 5.0f, .75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.6f, -0.8f, 6.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// TABLE
	// Leg Front Right
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.75f, 5.0f, .75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.6f, -.8f, 6.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// TABLE
	// Leg Back Left
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.75f, 5.0f, .75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.6f, -.8f, -.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// TABLE
	// Leg Back Right
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.75f, 5.0f, .75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.6f, -.8f, -.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// TABLE
	// Table top
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.1f, .8f, 8.1f); // Square and slightly larger than legs

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 2.0f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}

/***********************************************************
 *  CreateTumbler()
 *
 *  This method creates the tumbler cup object in the
 *  3D Scene
 ***********************************************************/
void SceneManager::CreateTumbler() {

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Base
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.75f, 2.4f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("tumbler");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, false, true);
	/****************************************************************/

	// Mid-section
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.66f, 1.0f, .66f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.75f, 3.75f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetShaderTexture("tumbler");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);
	/****************************************************************/

	// Upper Section
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.66f, 1.0f, .66f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = .0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.75f, 3.75f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	SetShaderTexture("tumbler");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/****************************************************************/

	// Cap
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.66f, 0.1f, .66f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.75f, 4.75f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, .2, .2, .7);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	///****************************************************************/

	// Top Tab (Lip) 
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.15f, .1f, .15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.4f, 4.85f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 0, 1);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// Top Tab (Lower Part)
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.33f, .15f, .15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.17f, 4.8f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 0, 1);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}

/***********************************************************
 *  CreateRubiksCube()
 *
 *  This method creates the Rubik's Cube object in the
 *  3D Scene
 ***********************************************************/
void SceneManager::CreateRubiksCube() {

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Cube
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.8f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 2.8f, 6.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("cube");

	/*
	* These allow each face's texture to be set separately.
	* Then the specific face is drawn maintaining the original size of the cube.
	*/
	// White Face (Back)
	SetShaderTexture("whiteBack");
	m_basicMeshes->DrawBoxFacesMesh(true);

	// Green Face (Left)
	SetShaderTexture("greenLeft");
	m_basicMeshes->DrawBoxFacesMesh(0, 0, true);

	// Blue Face (Right)
	SetShaderTexture("blueRight");
	m_basicMeshes->DrawBoxFacesMesh(0, 0, 0, true);

	// Red Face (Top)
	SetShaderTexture("redTop");
	m_basicMeshes->DrawBoxFacesMesh(0, 0, 0, 0, true);

	// Yellow Face (Front)
	SetShaderTexture("yellowFront");
	m_basicMeshes->DrawBoxFacesMesh(0, 0, 0, 0, 0, true);
	/****************************************************************/
}

/***********************************************************
 *  CreateBook()
 *
 *  This method creates the book object in the 3D Scene
 ***********************************************************/
void SceneManager::CreateBook() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Cube
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 0.5f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 2.65f, 3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Cover");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawBoxFacesMesh(0, 0, 0, 0, true);

	SetShaderTexture("CoverSide");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawBoxFacesMesh(0, 0, true);

	SetShaderTexture("bookPages");
	SetShaderMaterial("backdrop");
	m_basicMeshes->DrawBoxFacesMesh(true, 0, 0, true, 0, true);

}

/***********************************************************
 *  CreateDiffuser()
 *
 *  This method creates the reed diffuser in the 3D Scene
 ***********************************************************/
void SceneManager::CreateDiffuser() {

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Reed #1
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.01f, 3.0f, .01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 6.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 6.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.70f, 2.42f, -0.05f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.722, .525, .043, 1);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Reed #2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.01f, 3.0f, .01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -3.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 6.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.68f, 2.42f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.622, .425, .033, 1);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Reed #3
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.01f, 3.0f, .01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees =-6.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 3.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.70f, 2.42f, 0.05f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.722, .525, .043, 1);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Reed #4
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.01f, 3.0f, .01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -9.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -6.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.815f, 2.42f, 0.12f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.622, .425, .033, 1);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Reed #5
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.01f, 3.0f, .01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -12.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.92f, 2.42f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.722, .525, .043, 1);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Reed #6
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.01f, 3.0f, .01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 6.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -12.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.975f, 2.42f, -0.075f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.622, .425, .033, 1);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/


	/* CONTAINER*/
	// BASE Water
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.33f, 0.2f, 0.33f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.75f, 2.42f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.863, .863, .863, .7);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// BASE
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 1.0f, 0.35f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.75f, 2.42f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.867, .627, .867, .4);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	//Top Half Sphere (To add depth/ round edges)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.354f, 0.08f, 0.354f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.75f, 3.41f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.867, .627, .867, .4);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/

	//TOP
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.15f, .75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.75f, 3.55f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.847, .749, .847, .4);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

	
}