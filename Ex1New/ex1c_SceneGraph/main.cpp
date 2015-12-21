#include "../../common/opengl.h"
#include "../../common/sfml.h"
#include "../../common/glm.h"
#include "../../common/assimp.h"
#include "../../common/assimp_and_glm.h"

#include "../../common/glsl.h"
#include "../../common/glm_additional.h"

#include <iostream>
#include <string>

void iterateScenegraph(glm::mat4 parentMat, aiNode* currentNode, std::vector<glm::mat4>& modelMatricesRef) {
	parentMat *= toGlm::mat4(currentNode->mTransformation);

	for (int i = 0; i < currentNode->mNumMeshes; i++)
		modelMatricesRef[currentNode->mMeshes[i]] = parentMat;

	for (int i = 0; i < currentNode->mNumChildren; i++)
		iterateScenegraph(parentMat, currentNode->mChildren[i], modelMatricesRef);
}

struct Scene {
public:
	std::vector<GLuint> vertexHandles = std::vector<GLuint>();
	std::vector<GLuint> normalHandles = std::vector<GLuint>();
	std::vector<GLuint> indexHandles = std::vector<GLuint>();
	std::vector<GLuint> vaoHandles = std::vector<GLuint>();
	std::vector<int> indexArrSizes = std::vector<int>();
	std::vector<glm::mat4> modelMatrices = std::vector<glm::mat4>();
};

Scene loadScene(std::string filePath) {
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1);
	const aiScene *scene = importer.ReadFile(filePath, aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals); // aiProcess_PreTransformVertices
	if (!scene)
	{
		std::cout << "Not loaded" << std::endl;
		//return -1;
	}

	// C) upload @GPU

	Scene currentScene{};
	
	for (int m = 0; m < scene->mNumMeshes; m++) {
		aiMesh* mesh = scene->mMeshes[m];

		currentScene.vertexHandles.push_back(GLuint());
		currentScene.normalHandles.push_back(GLuint());
		currentScene.indexHandles.push_back(GLuint());
		currentScene.vaoHandles.push_back(GLuint());
		currentScene.normalHandles.push_back(GLuint());

		std::vector<GLfloat> vertArr;
		for (int i = 0; i < scene->mMeshes[m]->mNumVertices; i++)
		{
			vertArr.push_back(scene->mMeshes[m]->mVertices[i].x);
			vertArr.push_back(scene->mMeshes[m]->mVertices[i].y);
			vertArr.push_back(scene->mMeshes[m]->mVertices[i].z);
		}
		//Gen Vertex VBO
		glGenBuffers(1, &currentScene.vertexHandles[m]);
		glBindBuffer(GL_ARRAY_BUFFER, currentScene.vertexHandles[m]);
		glBufferData(GL_ARRAY_BUFFER, (vertArr.size() * sizeof(GLfloat)), vertArr.data(), GL_STATIC_DRAW);


		std::vector<GLfloat> normalArr;
		for (int i = 0; i < scene->mMeshes[m]->mNumVertices; i++)
		{
			normalArr.push_back(scene->mMeshes[m]->mNormals[i].x);
			normalArr.push_back(scene->mMeshes[m]->mNormals[i].y);
			normalArr.push_back(scene->mMeshes[m]->mNormals[i].z);
		}
		//Gen Normal VBO
		glGenBuffers(1, &currentScene.normalHandles[m]);
		glBindBuffer(GL_ARRAY_BUFFER, currentScene.normalHandles[m]);
		glBufferData(GL_ARRAY_BUFFER, (normalArr.size() * sizeof(GLfloat)), normalArr.data(), GL_STATIC_DRAW);

		//Gen VBA
		glGenVertexArrays(1, &currentScene.vaoHandles[m]);
		glBindVertexArray(currentScene.vaoHandles[m]);

		//Upload Vertex Data
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, currentScene.vertexHandles[m]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3 * sizeof(GLfloat)), NULL);

		//Upload Normal Data
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, currentScene.normalHandles[m]);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (3 * sizeof(GLfloat)), NULL);

		std::vector<GLuint> indexArr;
		for (int i = 0; i < scene->mMeshes[m]->mNumFaces; i++)
		{
			indexArr.push_back(scene->mMeshes[m]->mFaces[i].mIndices[0]);
			indexArr.push_back(scene->mMeshes[m]->mFaces[i].mIndices[1]);
			indexArr.push_back(scene->mMeshes[m]->mFaces[i].mIndices[2]);	
		}

		currentScene.indexArrSizes.push_back(indexArr.size());
		//Register Index Data
		glGenBuffers(1, &currentScene.indexHandles[m]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currentScene.indexHandles[m]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (indexArr.size() * sizeof(GLuint)), indexArr.data(), GL_STATIC_DRAW);
	}

	//std::vector<glm::mat4> modelMatrices(scene->mNumMeshes);
	currentScene.modelMatrices = std::vector<glm::mat4>(scene->mNumMeshes);
	iterateScenegraph(glm::mat4(), scene->mRootNode, currentScene.modelMatrices);

	return currentScene;
}

int main(int argc, char* argv[])
{
	unsigned int windowWidth = 1000;
	unsigned int windowHeight = 1000;

	sf::Window window(sf::VideoMode(windowWidth, windowHeight, 32), "asset loading", sf::Style::Close | sf::Style::Titlebar, sf::ContextSettings(16, 0, 8, 3, 3));
	if (glewInit() != GLEW_OK) return 0;

	// A) write & load simple shader

	std::vector< cg2::ShaderInfo > shaders; shaders.reserve(2);
	shaders.push_back(cg2::ShaderInfo(cg2::ShaderType::VERTEX_SHADER, cg2::loadShaderSource("shaders//1c.vert")));
	shaders.push_back(cg2::ShaderInfo(cg2::ShaderType::FRAGMENT_SHADER, cg2::loadShaderSource("shaders//1c.frag")));
	std::shared_ptr< cg2::GlslProgram > prog = cg2::GlslProgram::create(shaders, true);

	// B) load asset

	std::vector<Scene> scenes = std::vector<Scene>(argc);
	for (int i = 1; i < argc; i++)
		scenes[i] = loadScene(std::string(argv[i]));
	
	std::cout << "break" << std::endl;

	const float fieldOfView = glm::radians(30.f);
	const float zNear = 0.1f;
	const float zFar = 15.f;
	const float windowAspectRatio = 1.f;
	const glm::mat4 projectionTf = glm::perspective(fieldOfView, windowAspectRatio, zNear, zFar);

	glm::vec3 centerOfInterest(0.f, 1.f, 0.f);
	glm::vec3 cameraPos(0.f, 1.f, 6.f);
	glm::vec3 up(0.f, 1.f, 0.f);
	glm::mat4 viewTf = glm::lookAt(cameraPos, centerOfInterest, up);

	
	while (true)
	{
		bool exit = false;
		sf::Event ev;
		while (window.pollEvent(ev))
		{
			if (sf::Event::Closed == ev.type)
			{
				exit = true;
				break;
			}
		}
		if (exit) break;

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 cubeScaleMatrix = glm::mat4{ 
			{7,0,0,0},
			{0,7,0,0},
			{0,0,7,0},
			{0,0,0,1}
		};

		for (int sceneIndex = 0; sceneIndex < scenes.size(); sceneIndex++) {
			Scene scene = scenes[sceneIndex];
			for (int i = 0; i < scene.vaoHandles.size(); i++) {	

				cg2::GlslProgram::setActiveProgram(prog);
				prog->setUniformMat4("viewTf", viewTf);
				prog->setUniformMat4("projectionTf", projectionTf);
				
				if (sceneIndex == 2)
					prog->setUniformMat4("modelTf", scene.modelMatrices[i]*cubeScaleMatrix);
				else
					prog->setUniformMat4("modelTf", scene.modelMatrices[i]);

				glBindVertexArray(scene.vaoHandles[i]);


				glBindBuffer(GL_ARRAY_BUFFER, scene.vertexHandles[i]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene.indexHandles[i]);
				glDrawElements(GL_TRIANGLES, scene.indexArrSizes[i], GL_UNSIGNED_INT, nullptr);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
		}

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);
		glEnd();
		cg2::GlslProgram::setActiveProgram(nullptr);

		window.display();
	}

	// G) cleanup resources

	return 0;
}