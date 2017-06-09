#include "Object.hpp"

//#include "Libs\glm-0.9.8.4\glm\glm\gtc\type_ptr.hpp"
#include <gtc\type_ptr.hpp>

//#include "Libs\Simple OpenGL Image Library\src\SOIL.h"
#include <SOIL.h>

#include <iostream>

using namespace std;

// ************* GameObject *********************

void VisibleObject::sendObjectToGPU()
{
	// set up buffers to hold mesh data
	// Error handling missing
	glGenBuffers(1, &mVertexBufferID);
	glGenBuffers(1, &mNormalBufferID);
	glGenBuffers(1, &mElementBufferID);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferID);
	size_t dataSize = mMesh->vertices.size() * sizeof(mMesh->vertices[0]);
	glBufferData(GL_ARRAY_BUFFER, dataSize, mMesh->vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, mNormalBufferID);
	size_t ndataSize = mMesh->normals.size() * sizeof(mMesh->normals[0]);
	glBufferData(GL_ARRAY_BUFFER, ndataSize, mMesh->normals.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBufferID);
	size_t indicesSize = mMesh->elements.size() * sizeof(mMesh->elements[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, mMesh->elements.data(), GL_STATIC_DRAW);

	// If texture coords present set up texture coord buffer
	if (mMesh->texcoords.size() > 0) {
		glGenBuffers(1, &mTexCoordBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, mTexCoordBufferID);
		size_t tcdataSize = mMesh->texcoords.size() * sizeof(mMesh->texcoords[0]);
		glBufferData(GL_ARRAY_BUFFER, tcdataSize, mMesh->texcoords.data(), GL_STATIC_DRAW);
	}

	// If texture file present set up texture buffers etc
	if (mMaterial->textureFile != "") // assume that therefore we have a texture file!
	{
		// Create a texture buffer ID
		glGenTextures(1, &mTextureBufferID);

		// Activate that texture ID
		glBindTexture(GL_TEXTURE_2D, mTextureBufferID);

		// Texture image data
		int imgWidth, imgHeight;

		// Load the texture image.
		unsigned char* image = SOIL_load_image(mMaterial->textureFile.c_str(),
			&imgWidth,
			&imgHeight,
			0,
			SOIL_LOAD_RGB);

		// Some debug info
		std::cout << "null: " << !image << std::endl;
		std::cout << "Max size: " << GL_MAX_TEXTURE_SIZE << std::endl;
		std::cout << "Width: " << imgWidth << std::endl;
		std::cout << "Height: " << imgHeight << std::endl;
		std::cout << "Texture ID: " << mTextureBufferID << std::endl;

		// Allocate storage for the texture and load the data.
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGB,
			imgWidth,
			imgHeight,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			image);

		// Set appropriate parameters for texture sampler
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Unbind the texture
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void VisibleObject::render()
{
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, mNormalBufferID);
	if (mMesh->texcoords.size() > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, mTexCoordBufferID);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBufferID);

	if (mMaterial->textureFile != "") {
		// Activate texture
		glBindTexture(GL_TEXTURE_2D, mTextureBufferID);
	}

	// Draw the object
	glDrawElements(GL_TRIANGLES, mMesh->elements.size(), GL_UNSIGNED_SHORT, 0);
}

void VisibleObject::useShader(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& correction)
{
	mShader->use();
	glEnableVertexAttribArray(mShader->attribute("vPosition"));
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferID);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(mShader->attribute("vTexCoord"));
	glBindBuffer(GL_ARRAY_BUFFER, mTexCoordBufferID);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Associate view matrix with shader uniform V
	glUniformMatrix4fv(mShader->uniform("V"), 1, GL_FALSE, glm::value_ptr(view));
	// Associate projection matrix with shader uniform P
	glUniformMatrix4fv(mShader->uniform("P"), 1, GL_FALSE, glm::value_ptr(proj));

	// Object model transform
	glm::mat4 mt = mMovement * correction * mModelTransform;
	glUniformMatrix4fv(mShader->uniform("M"), 1, GL_FALSE, glm::value_ptr(mt));
}

glm::vec3 VisibleObject::getMeshCentroid()
{
	glm::vec3 c(0.0f, 0.0f, 0.0f);
	size_t nVertices = mMesh->vertices.size();
	for (size_t i = 0; i < nVertices; ++i) {
		c += glm::vec3(mMesh->vertices[i]);
	}
	return c / (float)nVertices;
}

float VisibleObject::getBoundingSphereRadius()
{
	glm::vec3 cd = getMeshCentroid();
	float maxR = 0.0;
	size_t nVertices = mMesh->vertices.size();
	for (size_t i = 0; i < nVertices; ++i) {
		glm::vec4 v = mMesh->vertices[i] - glm::vec4(cd, 0.0f);
		float dx = v.x;
		float dy = v.y;
		float dz = v.z;
		if (dx > maxR) maxR = dx;
		if (dy > maxR) maxR = dy;
		if (dz > maxR) maxR = dz;
	}
	return maxR;
}

AABB VisibleObject::getBoundingBox()
{
	glm::vec3 cd = getMeshCentroid();
	float maxX = 0.0;
	float maxY = 0.0;
	float maxZ = 0.0;
	size_t nVertices = mMesh->vertices.size();
	for (size_t i = 0; i < nVertices; ++i) {
		float dx = abs(mMesh->vertices[i].x - cd.x);
		float dy = abs(mMesh->vertices[i].y - cd.y);
		float dz = abs(mMesh->vertices[i].z - cd.z);
		if (dx > maxX) maxX = dx;
		if (dy > maxY) maxY = dy;
		if (dz > maxZ) maxZ = dz;
	}

	return AABB(cd, maxX, maxY, maxZ);
}

void VisibleObject::translateMesh(glm::vec3 disp)
{
	// Change mesh vertices by amount given
	size_t nVertices = mMesh->vertices.size();
	for (size_t i = 0; i < nVertices; ++i) {
		glm::vec4 v = mMesh->vertices[i];
		mMesh->vertices[i] = v + glm::vec4(disp, 0.0f);
	}
}

void VisibleObject::move(glm::vec3 d)
{
	glm::mat4 transMat = glm::translate(glm::mat4(), d);
	// had mModelTransform = transMat * mModelTransform;
	// which had effect of concatenating all movements to the model transform matrix!
	// Changed logic to store current required movement. This will be retrieved
	// by the render code and concatenated with mModelTransform  at that point.
	// Idea is that mMovement will hold movement required in that frame to move object
	// from original position to new position.
	mMovement = transMat;
}

void VisibleObject::compileShaders()
{
	if ((mMaterial->vertexShader != "") | (mMaterial->fragmentShader != "")) {
		try {
			mShader = new ShaderProgram();
			mShader->initFromFiles(mMaterial->vertexShader, mMaterial->fragmentShader);

			mShader->addAttribute("vPosition");
			//defaultShader->addAttribute("vNormal");
			mShader->addAttribute("vTexCoord");
			mShader->addUniform("P");
			mShader->addUniform("M");
			mShader->addUniform("V");
			if (glGetUniformLocation(mShader->getProgramID(), "texture1") != -1) {
				mShader->addUniform("texture1");
			}
			//defaultShader->addUniform("lightPosition");
			//defaultShader->addUniform("ambientContrib");
			//defaultShader->addUniform("diffuseContrib");
			//defaultShader->addUniform("specularContrib");
			//defaultShader->addUniform("shininess");
		}
		catch (const runtime_error& error) {
			cerr << "Error in shader processing for object " << mName << endl;
			cerr << error.what();
			// Program now continues but will abort when it hits the code to associate 
			// attribute/uniforms with appropriate program data as that code
			// has not been wrapped in a try..catch block.
			// Might be better to terminate program at this point.
		}
	}
}