#include "../../common/opengl.h"
#include "../../common/sfml.h"
#include "../../common/glm.h"
#include "../../common/assimp.h"
#include "../../common/assimp_and_glm.h"

#include "../../common/arcball.h"
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
	float bias = 0.05;
	float maxBias = 0.9;

	sf::Window window(sf::VideoMode(windowWidth, windowHeight, 32), "Use arrow up / down to change bias", sf::Style::Close | sf::Style::Titlebar, sf::ContextSettings(16, 0, 8, 3, 3));
	if (glewInit() != GLEW_OK) return 0;

	// A) write & load simple shader

	std::vector< cg2::ShaderInfo > shaders; shaders.reserve(2);
	shaders.push_back(cg2::ShaderInfo(cg2::ShaderType::VERTEX_SHADER, cg2::loadShaderSource("shaders//3.vert")));
	shaders.push_back(cg2::ShaderInfo(cg2::ShaderType::FRAGMENT_SHADER, cg2::loadShaderSource("shaders//3.frag")));
	std::shared_ptr< cg2::GlslProgram > prog = cg2::GlslProgram::create(shaders, true);

	std::vector<cg2::ShaderInfo> sphereShaders; sphereShaders.reserve(2);
	sphereShaders.push_back(cg2::ShaderInfo(cg2::ShaderType::VERTEX_SHADER, cg2::loadShaderSource("shaders//solidColor.vert")));
	sphereShaders.push_back(cg2::ShaderInfo(cg2::ShaderType::FRAGMENT_SHADER, cg2::loadShaderSource("shaders//solidColor.frag")));
	std::shared_ptr<cg2::GlslProgram> solidColorProg = cg2::GlslProgram::create(sphereShaders);

	std::vector< cg2::ShaderInfo > renderDepthShaders = { cg2::ShaderInfo(cg2::ShaderType::VERTEX_SHADER, cg2::loadShaderSource("shaders//depthRender.vert")), cg2::ShaderInfo(cg2::ShaderType::FRAGMENT_SHADER, cg2::loadShaderSource("shaders//depthRender.frag")) };
	std::shared_ptr< cg2::GlslProgram > renderDepthProg = cg2::GlslProgram::create(renderDepthShaders, true);

	std::vector< cg2::ShaderInfo > displayDepthShaders = { cg2::ShaderInfo(cg2::ShaderType::VERTEX_SHADER, cg2::loadShaderSource("shaders//depthDisplay.vert")), cg2::ShaderInfo(cg2::ShaderType::FRAGMENT_SHADER, cg2::loadShaderSource("shaders//depthDisplay.frag")) };
	std::shared_ptr< cg2::GlslProgram > displayDepthProg = cg2::GlslProgram::create(displayDepthShaders, true);


	// B) load asset

	std::vector<Scene> scenes = std::vector<Scene>(argc);
	for (int i = 1; i < argc; i++)
		scenes[i] = loadScene(std::string(argv[i]));
	
	std::cout << "break" << std::endl;

	const float fieldOfView = glm::radians(30.f);
	const float zNear = 0.1f;
	const float zFar = 20.f;
	const float windowAspectRatio = 1.f;
	const glm::mat4 projectionTf = glm::perspective(fieldOfView, windowAspectRatio, zNear, zFar);

	float cameraDistanceToOrigin = 6.f;
	glm::vec3 centerOfInterest(0.f, 1.f, 0.f);
	glm::vec3 cameraPos(0.f, 1.f, 6.f);
	glm::vec3 up(0.f, 1.f, 0.f);
	glm::mat4 viewTf = glm::lookAt(cameraPos, centerOfInterest, up);
	glm::mat4 inverseViewTf = glm::inverse(viewTf);
	
	cg2::Arcball cameraController(glm::uvec2(0, 0), glm::uvec2(800, 800));
	cg2::Arcball lightController(glm::uvec2(0, 0), glm::uvec2(800, 800));

	
	//////////// FBO ////////////
	const unsigned int framebufferWidth = 2 * windowWidth;
	const unsigned int framebufferHeight = 2 * windowHeight;
	const glm::vec4 depthTextureBorder(0.f);

	unsigned int handleToFramebufferObject = 0;
	unsigned int handleToDepthTexture = 0;

	glGenTextures(1, std::addressof(handleToDepthTexture));
	glBindTexture(GL_TEXTURE_2D, handleToDepthTexture);
	// https://www.opengl.org/wiki/Image_Format#Required_formats
	// GL_DEPTH_COMPONENT16 or GL_DEPTH_COMPONENT24 or GL_DEPTH_COMPONENT32F are all valid internal formats for this texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, framebufferWidth, framebufferHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, std::addressof(depthTextureBorder[0]));
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, std::addressof(handleToFramebufferObject));
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, handleToFramebufferObject);
	// functions like glFramebufferTexture2D are sometimes used @online tutorials, but they are deprecated
	glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, handleToDepthTexture, 0);

	// this looks like a drawing command, but it isn't
	// instead it tells opengl that all color outputs produced by the fragment shader
	// are useless and should be ignored when this fbo is used
	// (because there's no color attachment anyway)
	glDrawBuffer(GL_NONE);

	// never, ever forget the status check
	// you can absolutely not proceed without a valid fbo
	GLenum statusCode = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (statusCode != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "framebuffer invalid, status code " << statusCode << std::endl;
		return 0;
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	//////////// FBO END ////////////

	glm::mat4 sphereMatrix;
	
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
			else if (sf::Event::MouseMoved == ev.type)
			{
				// update camera controller rotation ( only has an effect when right mouse button is down )
				// please leave this line as it is
				cameraController.updateRotation(glm::uvec2(ev.mouseMove.x, ev.mouseMove.y));
				lightController.updateRotation(glm::uvec2(ev.mouseMove.x, ev.mouseMove.y));
			}
			else if (sf::Event::MouseButtonPressed == ev.type && ev.mouseButton.button == sf::Mouse::Left)
			{
				// start camera controller rotation
				// please leave this line as it is
				cameraController.startRotation(glm::uvec2(ev.mouseButton.x, ev.mouseButton.y));
			}
			else if (sf::Event::MouseButtonReleased == ev.type && ev.mouseButton.button == sf::Mouse::Left)
			{
				// stop camera controller rotation
				// please leave this line as it is
				cameraController.endRotation();
			}
			else if (sf::Event::MouseButtonPressed == ev.type && ev.mouseButton.button == sf::Mouse::Right)
			{
				// start camera controller rotation
				// please leave this line as it is
				lightController.startRotation(glm::uvec2(ev.mouseButton.x, ev.mouseButton.y));
			}
			else if (sf::Event::MouseButtonReleased == ev.type && ev.mouseButton.button == sf::Mouse::Right)
			{
				// stop camera controller rotation
				// please leave this line as it is
				lightController.endRotation();
			}
			else if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Key::Up) {
				if (bias + 0.05 <= maxBias)
					bias += 0.05;
				std::cout << "Bias: " + std::to_string(bias) << std::endl;
			}
			else if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Key::Down) {
				if (bias - 0.05 >= 0)
					bias -= 0.05;
				std::cout << "Bias: " + std::to_string(bias) << std::endl;
			}
		}

		if (exit) break;

		glEnable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, handleToFramebufferObject);	// gl_draw_framebuffer: used for render-to-texture
		glViewport(0, 0, framebufferWidth, framebufferHeight);				// viewport should fit the dimensions of the attached textures
		glClear(GL_DEPTH_BUFFER_BIT);											// this fbo has no color attachments

		
		cg2::GlslProgram::setActiveProgram(renderDepthProg);
		renderDepthProg->setUniformMat4("viewTf", glm::lookAt(glm::vec3(sphereMatrix[3]), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0)));
		renderDepthProg->setUniformMat4("projectionTf", glm::perspective(glm::half_pi<glm::float32>(), windowAspectRatio, zNear, zFar));

		Scene scene = scenes[1]; //Robot
		for (int i = 0; i < scene.vaoHandles.size(); i++) {
			renderDepthProg->setUniformMat4("modelTf", scene.modelMatrices[i]);
			glBindVertexArray(scene.vaoHandles[i]);

			glBindBuffer(GL_ARRAY_BUFFER, scene.vertexHandles[i]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene.indexHandles[i]);
			glDrawElements(GL_TRIANGLES, scene.indexArrSizes[i], GL_UNSIGNED_INT, nullptr);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		cg2::GlslProgram::setActiveProgram(nullptr);
	
		//// End depth map render ///



		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);							// 0 = default framebuffer, i.e. the window
		glViewport(0, 0, windowWidth, windowHeight);

		glEnable(GL_MULTISAMPLE);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		
		cameraPos = glm::mat3_cast(cameraController.getCurrentRotation()) * glm::vec3(0.f, 0.f, cameraDistanceToOrigin);
		up = glm::mat3_cast(cameraController.getCurrentRotation()) * glm::vec3(0.f, 1.f, 0.f);
		viewTf = glm::lookAt(cameraPos, centerOfInterest, up);
		//viewTfInverse = glm::inverse(viewTf);

		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 cubeScaleMatrix = glm::mat4{ 
			{7,0,0,0},
			{0,7,0,0},
			{0,0,7,0},
			{0,0,0,1}
		};


		//Each of these vectors is a column

		glm::vec3 lightColor = glm::vec3(1.f, 1.f, 1.f);
		glActiveTexture(GL_TEXTURE0);

		for (int sceneIndex = 0; sceneIndex < scenes.size(); sceneIndex++) {
			Scene scene = scenes[sceneIndex];
			for (int i = 0; i < scene.vaoHandles.size(); i++) {	

				sphereMatrix = glm::mat4{
					{ 0.2, 0, 0, 0 },
					{ 0, 0.2, 0, 0 },
					{ 0, 0, 0.2, 0 },
					{ 0, 0, 0, 1 }
				};

				glBindTexture(GL_TEXTURE_2D, handleToDepthTexture);

				sphereMatrix = glm::translate(sphereMatrix, glm::vec3(0, 8, 0));
				glm::mat4 rotMat = glm::mat4_cast(lightController.getCurrentRotation());
				sphereMatrix = sphereMatrix*rotMat;
				sphereMatrix = glm::translate(sphereMatrix, glm::vec3(0, 0, 5));

				if (sceneIndex == 3) {
					cg2::GlslProgram::setActiveProgram(solidColorProg);
					glm::mat4 mvp = projectionTf * viewTf * (scene.modelMatrices[i] * sphereMatrix);
					solidColorProg->setUniformMat4("mvp", mvp);
					solidColorProg->setUniformVec4("solidColor", lightColor.x, lightColor.y, lightColor.z, 1.f);
				}

				else if (sceneIndex == 4) {

					glViewport(0, 0, windowWidth / 4, windowHeight / 4);
					glDisable(GL_DEPTH_TEST);

					cg2::GlslProgram::setActiveProgram(displayDepthProg);
					displayDepthProg->setUniformTexVal("depthTex", 0);				// val = 0 because we bound to texture unit 0 via glActiveTexture
					displayDepthProg->setUniformVal("zNear", zNear);
					displayDepthProg->setUniformVal("zFar", zFar);
					glm::mat4 mvp = projectionTf * viewTf * scene.modelMatrices[i];
					displayDepthProg->setUniformMat4("mvp", glm::mat4(1));
				}

				else {
					cg2::GlslProgram::setActiveProgram(prog);
					prog->setUniformMat4("viewTf", viewTf);
					prog->setUniformMat4("projectionTf", projectionTf);
					prog->setUniformMat4("inverseViewTf", inverseViewTf);
					prog->setUniformVec3("cameraPosition", cameraPos[0], cameraPos[1], cameraPos[2]);
					prog->setUniformVal("shininess", 50.f);
					prog->setUniformVal("apertureAngle", glm::quarter_pi<glm::float32>());
					prog->setUniformVec3("lightColor", lightColor.x, lightColor.y, lightColor.z);
					prog->setUniformVal("zNear", zNear);
					prog->setUniformVal("zFar", zFar);
					prog->setUniformMat4("viewTf_Light", glm::lookAt(glm::vec3(sphereMatrix[3]), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0)));
					prog->setUniformMat4("projectionTf_Light", glm::perspective(glm::half_pi<glm::float32>(), windowAspectRatio, zNear, zFar));
					prog->setUniformVal("windowDim", windowHeight);
					prog->setUniformVal("biasScale", bias);
					prog->setUniformIVal("maxBias", maxBias);

					if (sceneIndex == 2) {
						prog->setUniformMat4("modelTf", scene.modelMatrices[i] * cubeScaleMatrix);
						prog->setUniformMat4("inverseModelTf", glm::inverse(scene.modelMatrices[i] * cubeScaleMatrix));
					}

					else {
						prog->setUniformMat4("modelTf", scene.modelMatrices[i]);
						prog->setUniformMat4("inverseModelTf", glm::inverse(scene.modelMatrices[i]));
					}
					prog->setUniformTexVal("depthTex", 0);
					glm::vec3 L = glm::vec3(sphereMatrix[3]);
					prog->setUniformVec3("light", L[0], L[1], L[2]);
					glm::vec3 dir = L - glm::vec3(0.f, 1.f, 0.f);
					prog->setUniformVec3("lightDirection", dir.x, dir.y, dir.z);
				}

				glBindVertexArray(scene.vaoHandles[i]);

				glBindBuffer(GL_ARRAY_BUFFER, scene.vertexHandles[i]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene.indexHandles[i]);
				glDrawElements(GL_TRIANGLES, scene.indexArrSizes[i], GL_UNSIGNED_INT, nullptr);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				glBindTexture(GL_TEXTURE_2D, 0);
				glViewport(0, 0, windowWidth, windowHeight);
				glEnable(GL_DEPTH_TEST);


				cg2::GlslProgram::setActiveProgram(nullptr);
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