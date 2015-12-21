#include "../../common/opengl.h"
#include "../../common/sfml.h"
#include "../../common/glm.h"
#include "../../common/assimp.h"

#include "../../common/glsl.h"
#include "../../common/glm_additional.h"

#include <iostream>
#include <string>

int main( int argc, char* argv[] )
{
	unsigned int windowWidth = 800;
	unsigned int windowHeight = 800;

	sf::Window window( sf::VideoMode( windowWidth, windowHeight, 32 ), "asset loading", sf::Style::Close | sf::Style::Titlebar, sf::ContextSettings( 16, 0, 8, 3, 3 ) );
	if ( glewInit() != GLEW_OK ) return 0;

	// A) write & load simple shader

	std::vector< cg2::ShaderInfo > shaders; shaders.reserve(2);
	shaders.push_back(cg2::ShaderInfo(cg2::ShaderType::VERTEX_SHADER, cg2::loadShaderSource("shaders/1b.vert"))); ///../../bin/shaders/1b.vert")));
	shaders.push_back(cg2::ShaderInfo(cg2::ShaderType::FRAGMENT_SHADER, cg2::loadShaderSource("shaders/1b.frag")));	//"shaders//1b.frag")));
	std::shared_ptr< cg2::GlslProgram > prog = cg2::GlslProgram::create(shaders, true);
	
	// B) load asset

	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1);
	const aiScene *scene = importer.ReadFile(argv[1], aiProcess_PreTransformVertices |
		aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals);
	if (!scene)
	{
		std::cout << "Not loaded" << std::endl;
		return -1;
	}

	// C) upload @GPU

	std::vector<GLuint> vertexHandles;
	std::vector<GLuint> normalHandles;
	std::vector<GLuint> indexHandles;
	std::vector<GLuint> vaoHandles;
	std::vector<int> indexArrSizes;
	/*GLuint vertHandle;
	GLuint idxHandle;
	GLuint vaoHandle;
*/
	for (int m = 0; m < scene->mNumMeshes; m++) {
		aiMesh* mesh = scene->mMeshes[m];
	
		vertexHandles.push_back(GLuint());
		normalHandles.push_back(GLuint());
		indexHandles.push_back(GLuint());
		vaoHandles.push_back(GLuint());
		normalHandles.push_back(GLuint());

		std::vector<GLfloat> vertArr;
		for (int i = 0; i < scene->mMeshes[m]->mNumVertices; i++)
		{
			vertArr.push_back(scene->mMeshes[m]->mVertices[i].x);
			vertArr.push_back(scene->mMeshes[m]->mVertices[i].y);
			vertArr.push_back(scene->mMeshes[m]->mVertices[i].z);
		}
		//Gen Vertex VBO
		glGenBuffers(1, &vertexHandles[m]);
		glBindBuffer(GL_ARRAY_BUFFER, vertexHandles[m]);
		glBufferData(GL_ARRAY_BUFFER, (vertArr.size() * sizeof(GLfloat)), vertArr.data(), GL_STATIC_DRAW);

		
		std::vector<GLfloat> normalArr;
		for (int i = 0; i < scene->mMeshes[m]->mNumVertices; i++) 
		{
			normalArr.push_back(scene->mMeshes[m]->mNormals[i].x);
			normalArr.push_back(scene->mMeshes[m]->mNormals[i].y);
			normalArr.push_back(scene->mMeshes[m]->mNormals[i].z);
		}
		//Gen Normal VBO
		glGenBuffers(1, &normalHandles[m]);
		glBindBuffer(GL_ARRAY_BUFFER, normalHandles[m]);
		glBufferData(GL_ARRAY_BUFFER, (normalArr.size() * sizeof(GLfloat)), normalArr.data(), GL_STATIC_DRAW);

		//Gen VAO
		glGenVertexArrays(1, &vaoHandles[m]);
		glBindVertexArray(vaoHandles[m]);

		//Upload Vertex Data
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexHandles[m]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3 * sizeof(GLfloat)), NULL);

		//Upload Normal Data
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, normalHandles[m]);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (3 * sizeof(GLfloat)), NULL);

		std::vector<GLuint> indexArr;
		for (int i = 0; i < scene->mMeshes[m]->mNumFaces; i++)
		{
			indexArr.push_back(scene->mMeshes[m]->mFaces[i].mIndices[0]);
			indexArr.push_back(scene->mMeshes[m]->mFaces[i].mIndices[1]);
			indexArr.push_back(scene->mMeshes[m]->mFaces[i].mIndices[2]);
		}

		indexArrSizes.push_back(indexArr.size());
		//Register Index Data
		glGenBuffers(1, &indexHandles[m]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexHandles[m]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (indexArr.size() * sizeof(GLuint)), indexArr.data(), GL_STATIC_DRAW); 
	}

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
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cg2::GlslProgram::setActiveProgram(prog);

		for (int i = 0; i < vaoHandles.size(); i++) {

			glBindVertexArray(vaoHandles[i]);

			glBindBuffer(GL_ARRAY_BUFFER, vertexHandles[i]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexHandles[i]);
			glDrawElements(GL_TRIANGLES, indexArrSizes[i], GL_UNSIGNED_INT, nullptr);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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