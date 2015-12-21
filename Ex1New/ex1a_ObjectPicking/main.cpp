#include "../../common/opengl.h"
#include "../../common/sfml.h"
#include "../../common/glm.h"

#include "../../common/glsl.h"
#include "../../common/glm_additional.h"
#include "../../common/arcball.h"

#include "randomVars.h"
#include "sphereGeometry.h"

#include <iostream>
#include <string>
#include <vector>

struct Sphere
{
	glm::vec3	center;
	float		radius;
	glm::vec3	color;
};

//Subtract SFML yCoordinates from windowHeight to get to GL Coordsystem
int main( int argc, char** argv )
{
	unsigned int windowWidth = 1200;
	unsigned int windowHeight = 1200;

	sf::Window window( sf::VideoMode( windowWidth, windowHeight, 32 ), "pick me!", sf::Style::Close | sf::Style::Titlebar, sf::ContextSettings( 16, 0, 8, 3, 3 ) );
	if ( glewInit() != GLEW_OK ) return 0;

	// load shaders & create sphere mesh
	std::vector< cg2::ShaderInfo > shaders; shaders.reserve( 2 );
	shaders.push_back( cg2::ShaderInfo( cg2::ShaderType::VERTEX_SHADER, cg2::loadShaderSource( "shaders//ex1a_spheres.vert" ) ) );
	shaders.push_back( cg2::ShaderInfo( cg2::ShaderType::FRAGMENT_SHADER, cg2::loadShaderSource( "shaders//ex1a_spheres.frag" ) ) );
	std::shared_ptr< cg2::GlslProgram > prog = cg2::GlslProgram::create( shaders, true );
	std::shared_ptr< Mesh > sphereMesh = createSphereMesh();

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// HOMEWORK STARTS HERE

	// we start by defining a set of 1000 spheres to be drawn
	// and assigning them a random center & color.
	// for a definition of 'Sphere', go to the top of this file
	std::vector< Sphere > spheres; spheres.resize( 1000 );
	for ( Sphere &sphere : spheres )	// @C++: observe the use of a reference ( &sphere ),
										// if we don't do this we will get copies of the elements of the vector
										// which means the assignments below will be meaningless, as the vector elements won't be changed
	{
		sphere.center = randVec3( glm::vec3( -1.f, -1.f, -1.f ), glm::vec3( 1.f, 1.f, 1.f ) );
		sphere.color = randRGBColor( 0.66f, 1.f );
		sphere.radius = 0.025f;
	}

	// projection matrix & its inverse are totally constant: the window can't be resized anyway
	const float fieldOfView = glm::radians( 30.f );
	const float zNear = 0.1f;
	const float zFar = 10.f;
	const float windowAspectRatio = 1.f;
	const glm::mat4 projectionTf = glm::perspective( fieldOfView, windowAspectRatio, zNear, zFar );
	const glm::mat4 projectionTfInverse = glm::inverse( projectionTf );

	// view matrix is not constant ( right mouse drag -> rotates view dir, distance to origin/center of interest stays the same )
	// see @code at UPDATE part
	const float cameraDistanceToOrigin = 2.5;
	const glm::vec3 centerOfInterest( 0.f, 0.f, 0.f );
	glm::vec3 cameraPos( 0.f, 0.f, cameraDistanceToOrigin );
	glm::vec3 up( 0.f, 1.f, 0.f );
	glm::mat4 viewTf = glm::lookAt( cameraPos, centerOfInterest, up );
	glm::mat4 viewTfInverse = glm::inverse( viewTf );

	// variables for the viewport transformation
	// viewport upperBound &  viewport lowerBound - also constant since the window can't be resized
	// depth range is also constant
	const glm::vec2 viewportUpperBound(windowWidth, windowHeight);
	const glm::vec2 viewportLowerBound(0, 0);
	const glm::vec2 depthRange(0.f, 1.f);
	const glm::mat4 viewportTf = glm::mat4((viewportUpperBound.x - viewportLowerBound.x) / 2, 0, 0, 
		(viewportUpperBound.x - viewportLowerBound.x) / 2, 0, 
		(viewportUpperBound.y - viewportLowerBound.y) / 2, 0, 
		(viewportUpperBound.y - viewportLowerBound.y) / 2, 0, 0, 
		depthRange.y - depthRange.x, depthRange.x, 0, 0, 0, 1);

	const glm::mat4 viewportTfInverse = glm::inverse(viewportTf);

	Sphere* currentSphere = nullptr; 
	glm::vec3 prevColor;

	// controller for camera rotation
	cg2::Arcball cameraController( glm::uvec2( 0, 0 ), glm::uvec2( 800, 800 ) );
	while ( true )
	{
		bool exit = false;
		sf::Event ev;
		while ( window.pollEvent( ev ) )
		{
			if ( sf::Event::Closed == ev.type )
			{
				exit = true;
				break;
			}
			else if ( sf::Event::MouseMoved == ev.type )
			{
				// TODO: move sphere, if one is selected

				// observe how we get the mouse position: mouseMove.x & mouseMove.y, in contrast to the mouse button events below!
				//std::cout << "mouse moved to  ( " << ev.mouseMove.x << " | " <<  ev.mouseMove.y << " ) " << std::endl;

				// update camera controller rotation ( only has an effect when right mouse button is down )
				// please leave this line as it is

				cameraController.updateRotation(glm::uvec2(ev.mouseMove.x, ev.mouseMove.y));

				if (currentSphere != nullptr) {

					glm::vec4 sphereCenterCopy = glm::vec4(currentSphere->center, 1);
					sphereCenterCopy = projectionTf * viewTf * sphereCenterCopy;
					sphereCenterCopy /= sphereCenterCopy.w;
					sphereCenterCopy = sphereCenterCopy * viewportTf;

					glm::vec4 sphereWorld(ev.mouseMove.x, windowHeight - ev.mouseMove.y, sphereCenterCopy.z, 1.f);
					sphereWorld = sphereWorld* glm::inverse(viewportTf);
					sphereWorld = projectionTfInverse*sphereWorld;
					sphereWorld /= sphereWorld.w;
					sphereWorld = viewTfInverse*sphereWorld;

					currentSphere->center = glm::vec3(sphereWorld.x, sphereWorld.y, sphereWorld.z);

				}		

			}
			else if ( sf::Event::MouseButtonPressed == ev.type && ev.mouseButton.button == sf::Mouse::Left )
			{
				// TODO: select sphere
				// observe how we get the mouse position: mouseButton.x & mouseButton.y, in contrast to the mouse move event above!
				//std::cout << "left mouse button pressed at ( " << ev.mouseButton.x << " | " << ev.mouseButton.y << " ) " << std::endl;
			
				if (!currentSphere) {
					Sphere* newSphere = nullptr;
					float lastT = FLT_MAX;

					float normX = (2.f * ev.mouseButton.x / (float)windowWidth) - 1.f;
					float normY = 1.f - (2.f * ev.mouseButton.y / (float)windowHeight);

					glm::vec4 nds = glm::vec4(normX, normY, 0.f, 1.0f); // -1.f z so it points inwards to screen
					glm::vec4 clipSpace = projectionTfInverse * nds;
					clipSpace = glm::vec4(clipSpace.x, clipSpace.y, -1.f, 0.f);
					glm::vec3 worldSpace = glm::normalize(glm::vec3(viewTfInverse * clipSpace));

					//std::cout << "Worldspace: " << worldSpace << std::endl;

					for (auto& sphere : spheres) {
						float b = (glm::dot(worldSpace, (cameraPos - sphere.center)));
						float c = glm::dot((cameraPos - sphere.center), (cameraPos - sphere.center)) - sphere.radius*sphere.radius;

						float tRoot = ((b*b) - c); // check part under root of quadratic 

						if (tRoot >= 0) { // if larger or equal 0 intersection found
							std::cout << "Intersect found" << std::endl;
							newSphere = &sphere;

							float t1 = (-b + glm::sqrt(tRoot));
							float t2 = (-b - glm::sqrt(tRoot));
							float closer = t1 < t2 ? t1 : t2;
							
							std::cout << closer << std::endl;

							if (closer < lastT) {
								lastT = closer;
								currentSphere = newSphere;
							}
						}
					}
					if (newSphere != nullptr) { //intersection was found
						//currentSphere = newSphere;
						prevColor = currentSphere->color;
						currentSphere->color = glm::vec3(0.5f, 0.5f, 0.5f);
					}
					std::cout << "\n-----\n" << std::endl;
				}
			}
			else if ( sf::Event::MouseButtonReleased == ev.type && ev.mouseButton.button == sf::Mouse::Left )
			{
				// TODO: delete selection
				if (currentSphere)
					currentSphere->color = prevColor;
				currentSphere = nullptr;
				// observe how we get the mouse position: mouseButton.x & mouseButton.y, in contrast to the mouse move event above!
				std::cout << "left mouse button released at ( " << ev.mouseButton.x << " | " << ev.mouseButton.y << " ) " << std::endl;
			}
			else if ( sf::Event::MouseButtonPressed == ev.type && ev.mouseButton.button == sf::Mouse::Right )
			{
				// start camera controller rotation
				// please leave this line as it is
				cameraController.startRotation( glm::uvec2( ev.mouseButton.x, ev.mouseButton.y ) );
			}
			else if ( sf::Event::MouseButtonReleased == ev.type && ev.mouseButton.button == sf::Mouse::Right )
			{
				// stop camera controller rotation
				// please leave this line as it is
				cameraController.endRotation();
			}
		}
		if ( exit ) break;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// HOMEWORK ENDS HERE

	// RENDER:
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_MULTISAMPLE );

		glViewport( (int)viewportLowerBound.x, (int)viewportLowerBound.y, (int)viewportUpperBound.x, (int)viewportUpperBound.y );

		glClearColor( 1.f, 1.f, 1.f, 1.f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		cg2::GlslProgram::setActiveProgram( prog );
		prog->setUniformMat4( std::string("viewTf"), viewTf );
		prog->setUniformMat4( std::string("projectionTf"), projectionTf );

		glBindVertexArray( sphereMesh->handleToVertexArrayObject );

		for (Sphere const& s : spheres)
		{
			prog->setUniformMat4( std::string("modelTf"), glm::translate( glm::mat4( 1.f ), s.center ) * glm::scale( glm::mat4( 1.f ), glm::vec3( s.radius ) ) );
			glVertexAttrib3f( 2, s.color.r, s.color.g, s.color.b );
			glDrawElements( GL_TRIANGLES, sphereMesh->indexCount, GL_UNSIGNED_INT, nullptr );
		}

		glEnd();

		glBindVertexArray( 0 );

		cg2::GlslProgram::setActiveProgram( nullptr );

		glDisable( GL_DEPTH_TEST );
		glDisable( GL_MULTISAMPLE );

	// UPDATE:
	// view matrix update: rotate the camera position & the up axis, leave center of interest @origin
		cameraPos = glm::mat3_cast( cameraController.getCurrentRotation() ) * glm::vec3( 0.f, 0.f, cameraDistanceToOrigin );
		up = glm::mat3_cast( cameraController.getCurrentRotation() ) * glm::vec3( 0.f, 1.f, 0.f );
		viewTf = glm::lookAt( cameraPos, centerOfInterest, up );
		viewTfInverse = glm::inverse( viewTf );

		window.display();
	}

	cleanUpMesh( sphereMesh );

	return 0;
}