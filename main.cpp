#define NOMINMAX
#include <limits>

// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.
#include <stdlib.h>

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include "maths_funcs.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/

#define MESH_NAME "./meshes/white_key.dae"
#define MESH_NAME2 "./meshes/black_key.dae"
#define MESH_NAME3 "./meshes/base.dae"
#define MESH_NAME4 "./meshes/floor.dae"
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes
typedef struct ModelData
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
};
#pragma endregion SimpleTypes

using namespace std;
GLuint shader, blackKeyShader;

ModelData mesh_data, mesh_data2, mesh_data3, mesh_data4;
int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_y = 0.0f;

GLfloat rotate_view_x = -90.0f, rotate_view_z = 0.0f;
GLfloat rotate_white_1 = 0.0f, rotate_white_3 = 0.0f, rotate_white_5 = 0.0f, rotate_white_7 = 0.0f;
GLfloat rotate_black_2 = 0.0f, rotate_black_3 = 0.0f, rotate_black_5 = 0.0f;
GLfloat return_white_1 = 0.0f, return_white_3 = 0.0f, return_white_5 = 0.0f, return_white_7 = 0.0f;
GLfloat return_black_2 = 0.0f, return_black_3 = 0.0f, return_black_5 = 0.0f;
GLfloat rotate_white_6 = 0.0f, return_white_6 = 0.0f;
GLfloat rotate_base = 0.0f;
vec3 translate_base = vec3(0.0f, 0.0f, 0.0f);
GLfloat view_x = 0.0f, view_y = 0.0f, view_z = 0.0f;

const int i = 16;
GLuint vp_vbo[i], vao[i], vn_vbo[i], vt_vbo[i];

GLuint texture[i];


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(const char* vertexShader, const char* fragmentShader)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	GLuint shaderProgramID;
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, vertexShader, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, fragmentShader, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh() {

	mesh_data = load_mesh(MESH_NAME);
	mesh_data2 = load_mesh(MESH_NAME2);
	mesh_data3 = load_mesh(MESH_NAME3);
	mesh_data4 = load_mesh(MESH_NAME4);
	
	loc1 = glGetAttribLocation(shader, "vertex_position");
	loc2 = glGetAttribLocation(shader, "vertex_normal");
	loc3 = glGetAttribLocation(shader, "vertex_texture");

	//white key
	glGenBuffers(1, &vp_vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mVertices[0], GL_STATIC_DRAW);
	
	glGenBuffers(1, &vn_vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mNormals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &vt_vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec2), &mesh_data.mTextureCoords[0], GL_STATIC_DRAW);

	glGenTextures(1, &texture[0]);
	//glActiveTexture(texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load and generate the texture
	int width, height, nrChannels;
	unsigned char* data = stbi_load("./textures/ivory.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	
	glGenVertexArrays(1, &vao[0]);
	glBindVertexArray(vao[0]);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[0]);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[0]);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray (loc3);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo[0]);
	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	

	//black key
	glGenBuffers(1, &vp_vbo[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data2.mPointCount * sizeof(vec3), &mesh_data2.mVertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &vn_vbo[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data2.mPointCount * sizeof(vec3), &mesh_data2.mNormals[0], GL_STATIC_DRAW);
	
	glGenBuffers(1, &vt_vbo[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data2.mPointCount * sizeof(vec2), &mesh_data2.mTextureCoords[0], GL_STATIC_DRAW);
	
	glGenTextures(1, &texture[1]);
	//glActiveTexture(texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	// load and generate the texture
	data = stbi_load("./textures/black_gloss.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	
	glGenVertexArrays(1, &vao[1]);
	glBindVertexArray(vao[1]);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[1]);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[1]);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	glEnableVertexAttribArray(loc3);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo[1]);
	glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	

	//base
	glGenBuffers(1, &vp_vbo[2]);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data3.mPointCount * sizeof(vec3), &mesh_data3.mVertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &vn_vbo[2]);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data3.mPointCount * sizeof(vec3), &mesh_data3.mNormals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &vt_vbo[2]);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data3.mPointCount * sizeof(vec2), &mesh_data3.mTextureCoords[0], GL_STATIC_DRAW);
	
	glGenTextures(1, &texture[2]);
	//glActiveTexture(texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	// load and generate the texture
	data = stbi_load("./textures/wood.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	
	glGenVertexArrays(1, &vao[2]);
	glBindVertexArray(vao[2]);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[2]);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[2]);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	glEnableVertexAttribArray(loc3);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo[2]);
	glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);


	//floor
	glGenBuffers(1, &vp_vbo[3]);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data4.mPointCount * sizeof(vec3), &mesh_data4.mVertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &vn_vbo[3]);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data4.mPointCount * sizeof(vec3), &mesh_data4.mNormals[0], GL_STATIC_DRAW);

	glGenVertexArrays(1, &vao[3]);
	glBindVertexArray(vao[3]);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[3]);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[3]);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
}
#pragma endregion VBO_FUNCTIONS


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shader);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shader, "model");
	int view_mat_location = glGetUniformLocation(shader, "view");
	int proj_mat_location = glGetUniformLocation(shader, "proj");

	// Root of the Hierarchy
	mat4 view = identity_mat4();
	mat4 persp_proj = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);

	view = translate(view, vec3(5.0f, -5.0f, 90.0f));
	view = rotate_z_deg(view, 180.f);
	view = rotate_x_deg(view, -90.f);
	view = translate(view, vec3(view_x, view_y, 0.0f));

	view = rotate_z_deg(view, rotate_view_z);
	view = rotate_x_deg(view, rotate_view_x);

	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);

	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glBindVertexArray(vao[2]);


	//base
	mat4 base = identity_mat4();
	base = rotate_y_deg(base, -90.0f);
	base = rotate_x_deg(base, -20.0f);
	base = translate(base, translate_base);

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, base.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data3.mPointCount);


	//white keys
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glBindVertexArray(vao[0]);

	mat4 white_key = identity_mat4();
	white_key = rotate_z_deg(white_key, rotate_white_1);
	white_key = rotate_z_deg(white_key, return_white_1);
	white_key = base * white_key;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, white_key.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	mat4 white_key2 = identity_mat4();
	white_key2 = translate(white_key2, vec3(0.0f, 0.0f, 1.6f));
	white_key2 = base * white_key2;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, white_key2.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	mat4 white_key3 = identity_mat4();
	white_key3 = translate(white_key3, vec3(0.0f, 0.0f, 3.2f));
	white_key3 = rotate_z_deg(white_key3, rotate_white_3);
	white_key3 = rotate_z_deg(white_key3, return_white_3);
	white_key3 = base * white_key3;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, white_key3.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	mat4 white_key4 = identity_mat4();
	white_key4 = translate(white_key4, vec3(0.0f, 0.0f, 4.8f));
	white_key4 = base * white_key4;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, white_key4.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	mat4 white_key5 = identity_mat4();
	white_key5 = translate(white_key5, vec3(0.0f, 0.0f, 6.4f));
	white_key5 = rotate_z_deg(white_key5, rotate_white_5);
	white_key5 = rotate_z_deg(white_key5, return_white_5);
	white_key5 = base * white_key5;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, white_key5.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	mat4 white_key6 = identity_mat4();
	white_key6 = translate(white_key6, vec3(0.0f, 0.0f, 8.0f));
	white_key6 = rotate_z_deg(white_key6, rotate_white_6);
	white_key6 = rotate_z_deg(white_key6, return_white_6);
	white_key6 = base * white_key6;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, white_key6.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	mat4 white_key7 = identity_mat4();
	white_key7 = translate(white_key7, vec3(0.0f, 0.0f, 9.6f));
	white_key7 = rotate_z_deg(white_key7, rotate_white_7);
	white_key7 = rotate_z_deg(white_key7, return_white_7);
	white_key7 = base * white_key7;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, white_key7.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	mat4 white_key8 = identity_mat4();
	white_key8 = translate(white_key8, vec3(0.0f, 0.0f, 11.2f));
	white_key8 = base * white_key8;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, white_key8.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	
	//black keys
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glBindVertexArray(vao[1]);

	mat4 black_key = identity_mat4();
	black_key = base * black_key;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, black_key.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);

	mat4 black_key2 = identity_mat4();
	black_key2 = translate(black_key2, vec3(0.0f, 0.0f, 1.6f));
	black_key2 = rotate_z_deg(black_key2, rotate_black_2);
	black_key2 = rotate_z_deg(black_key2, return_black_2);
	black_key2 = base * black_key2;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, black_key2.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);

	mat4 black_key3 = identity_mat4();
	black_key3 = translate(black_key3, vec3(0.0f, 0.0f, 4.8f));
	black_key3 = rotate_z_deg(black_key3, rotate_black_3);
	black_key3 = rotate_z_deg(black_key3, return_black_3);
	black_key3 = base * black_key3;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, black_key3.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);

	mat4 black_key4 = identity_mat4();
	black_key4 = translate(black_key4, vec3(0.0f, 0.0f, 6.4f));
	black_key4 = base * black_key4;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, black_key4.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);

	mat4 black_key5 = identity_mat4();
	black_key5 = translate(black_key5, vec3(0.0f, 0.0f, 8.0f));
	black_key5 = rotate_z_deg(black_key5, rotate_black_5);
	black_key5 = rotate_z_deg(black_key5, return_black_5);
	black_key5 = base * black_key5;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, black_key5.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data2.mPointCount);


	//floor
	glBindVertexArray(vao[3]);

	mat4 floor = identity_mat4();
	floor = translate(floor, vec3(-3.5f, -5.0f, 0.0f));

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, floor.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data4.mPointCount);

	glutSwapBuffers();
}


void updateScene() {

	static int count = 0;
	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	if (rotate_white_1 == 5.0f) {
		if (count < 2400) {
			return_white_1 -= 5.0f * delta;
			count++;
		}
		else {
			rotate_white_1 = 0.0f;
			return_white_1 = 0.0f;
			count = 0;
		}
	}
	if (rotate_white_3 == 5.0f) {
		if (count < 2400) {
			return_white_3 -= 5.0f * delta;
			count++;
		}
		else {
			rotate_white_3 = 0.0f;
			return_white_3 = 0.0f;
			count = 0;
		}
	}
	if (rotate_white_5 == 5.0f) {
		if (count < 2400) {
			return_white_5 -= 5.0f * delta;
			count++;
		}
		else {
			rotate_white_5 = 0.0f;
			return_white_5 = 0.0f;
			count = 0;
		}
	}
	if (rotate_white_6 == 5.0f) {
		if (count < 2400) {
			return_white_6 -= 5.0f * delta;
			count++;
		}
		else {
			rotate_white_6 = 0.0f;
			return_white_6 = 0.0f;
			count = 0;
		}
	}
	if (rotate_white_7 == 5.0f) {
		if (count < 2400) {
			return_white_7 -= 5.0f * delta;
			count++;
		}
		else {
			rotate_white_7 = 0.0f;
			return_white_7 = 0.0f;
			count = 0;
		}
	}
	if (rotate_black_2 == 5.0f) {
		if (count < 2400) {
			return_black_2 -= 5.0f * delta;
			count++;
		}
		else {
			rotate_black_2 = 0.0f;
			return_black_2 = 0.0f;
			count = 0;
		}
	}
	if (rotate_black_3 == 5.0f) {
		if (count < 2400) {
			return_black_3 -= 5.0f * delta;
			count++;
		}
		else {
			rotate_black_3 = 0.0f;
			return_black_3 = 0.0f;
			count = 0;
		}
	}
	if (rotate_black_5 == 5.0f) {
		if (count < 2400) {
			return_black_5 -= 5.0f * delta;
			count++;
		}
		else {
			rotate_black_5 = 0.0f;
			return_black_5 = 0.0f;
			count = 0;
		}
	}
	
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	shader = CompileShaders("./shaders/simpleVertexShader.txt", "./shaders/simpleFragmentShader.txt");
	blackKeyShader = CompileShaders("./shaders/blackKeyVertexShader.txt", "./shaders/blackKeyFragmentShader.txt");
	
	// load mesh into a vertex buffer array
	generateObjectBufferMesh();

	std::cout << "\nHARMONY GUIDE:\n\n";
	std::cout << "NOTE: Keystroke numbers do not correspond to scale degrees due to inability to input multiple chars at once e.g. b3.\n\n";
	std::cout << "TRIADS:\n";
	std::cout << "MAJOR TRIAD - Press keys: 1, 3, 5. Corresponds to notes: C, E, G.\n";
	std::cout << "MINOR TRIAD - Press keys: 1, 2, 5. Corresponds to notes: C, Eb, G.\n";
	std::cout << "DIMINISHED TRIAD - Press keys: 1, 2, 4. Corresponds to notes: C, Eb, Gb.\n\n";
	std::cout << "SEVENTHS:\n";
	std::cout << "MAJOR SEVENTH - Press keys: 1, 3, 5, 7. Corresponds to notes: C, E, G, B.\n";
	std::cout << "DOMINANT SEVENTH - Press keys: 1, 3, 5, 6. Corresponds to notes: C, E, G, Bb.\n";
	std::cout << "MINOR SEVENTH - Press keys: 1, 2, 5, 6. Corresponds to notes: C, Eb, G, Bb.\n";
	std::cout << "HALF-DIMINISHED SEVENTH - Press keys: 1, 2, 4, 6. Corresponds to notes: C, Eb, Gb, Bb.\n";
	std::cout << "DIMINISHED SEVENTH - Press keys: 1, 2, 4, 9. Corresponds to notes: C, Eb, Gb, Bbb.\n\n";
	std::cout << "SIXTHS:\n";
	std::cout << "MAJOR SIXTH - Press keys: 1, 3, 5, 9. Corresponds to notes: C, E, G, A.\n";
	std::cout << "MINOR SIXTH - Press keys: 1, 2, 5, 9. Corresponds to notes: C, Eb, G, A.\n\n";
	
}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	switch (key) {
	case 'd':
		rotate_view_z += 5.0f;
		break;
	case 'a':
		rotate_view_z -= 5.0f;
		break;
	case 'w':
		view_x -= sin(glm::radians(rotate_view_z)) * 0.4f;
		view_y -= cos(glm::radians(rotate_view_z)) * 0.4f;
		break;
	case 's':
		view_x += sin(glm::radians(rotate_view_z)) * 0.4f;
		view_y += cos(glm::radians(rotate_view_z)) * 0.4f;
		break;
	case '1':
		PlaySound(TEXT("./audio/C.wav"), NULL, SND_ASYNC);
		rotate_white_1 = 5.0f;
		translate_base = vec3(2.0f, 0.0f, 0.0f);
		break;
	case '3':
		PlaySound(TEXT("./audio/E.wav"), NULL, SND_ASYNC);
		rotate_white_3 = 5.0f;
		translate_base = vec3(0.0f, 0.0f, -2.0f);
		break;
	case '5':
		PlaySound(TEXT("./audio/G.wav"), NULL, SND_ASYNC);
		rotate_white_5 = 5.0f;
		translate_base = vec3(-2.0f, 0.0f, 0.0f);
		break;
	case '7':
		PlaySound(TEXT("./audio/B.wav"), NULL, SND_ASYNC);
		rotate_white_7 = 5.0f;
		translate_base = vec3(0.0f, 0.0f, 2.0f);
		break;
	case '2':
		PlaySound(TEXT("./audio/Eb.wav"), NULL, SND_ASYNC);
		rotate_black_2 = 5.0f;
		translate_base = vec3(-2.0f, 0.0f, 0.0f);
		break;
	case '4':
		PlaySound(TEXT("./audio/Gb.wav"), NULL, SND_ASYNC);
		rotate_black_3 = 5.0f;
		translate_base = vec3(0.0f, 0.0f, -2.0f);
		break;
	case '6':
		PlaySound(TEXT("./audio/Bb.wav"), NULL, SND_ASYNC);
		rotate_black_5 = 5.0f;
		translate_base = vec3(2.0f, 0.0f, 0.0f);
		break;
	case '9':
		PlaySound(TEXT("./audio/A.wav"), NULL, SND_ASYNC);
		rotate_white_6 = 5.0f;
		translate_base = vec3(0.0f, 0.0f, 2.0f);
		break;
	}	
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Piano");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}
