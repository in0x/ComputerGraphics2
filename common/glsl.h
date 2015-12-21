#pragma once

#include "opengl.h"
#include "glm.h"
#include "glm_additional.h"

#include <string>
#include <map>
#include <memory>
#include <vector>

namespace cg2
{

	/*
	*	loads shader a shader source from the given filepath
	*	if this fails, the returned string will be empty & a message will be printed to std::cout
	*/
	std::string loadShaderSource( const std::string filepath );

	enum class ShaderType
	{
		VERTEX_SHADER,
		FRAGMENT_SHADER,
		GEOMETRY_SHADER,
		TESSELLATION_CONTROL_SHADER,
		TESSELLATION_EVALUATION_SHADER,
		COMPUTE_SHADER,
		UNDEF
	};

	struct ShaderInfo
	{
		ShaderInfo( ShaderType t, std::string src ) : type( t ), source( src ) {}

		ShaderType		type = ShaderType::UNDEF;
		std::string		source = "";
	};

	class GlslProgram : public std::enable_shared_from_this < GlslProgram >
	{

	public:

		static std::shared_ptr< GlslProgram > create( std::vector< ShaderInfo > shaders, const bool printLog = false );

		/*
		*	makes the GlslProgram the active program
		*	meaning: it will be used for subsequent rendering operations
		*	nullptr -> no program
		*/
		static void setActiveProgram( std::shared_ptr< GlslProgram > prog );

		/*
		*	various functions for setting uniforms
		*	arrays: TODO
		*/
		void setUniformTexVal( const std::string name, const unsigned int v );
		void setUniformVal( const std::string name, const float v );
		void setUniformIVal( const std::string name, const int v );
		void setUniformUVal( const std::string name, const unsigned int v );
		void setUniformVec2( const std::string name, const float x, const float y );
		void setUniformVec3( const std::string name, const float x, const float y, const float z );
		void setUniformVec4( const std::string name, const float x, const float y, const float z, const float w );
		void setUniformIVec2( const std::string name, const int x, const int y );
		void setUniformIVec3( const std::string name, const int x, const int y, const int z );
		void setUniformIVec4( const std::string name, const int x, const int y, const int z, const int w );
		void setUniformUVec2( const std::string name, const unsigned int x, const unsigned int y );
		void setUniformUVec3( const std::string name, const unsigned int x, const unsigned int y, const unsigned int z );
		void setUniformUVec4( const std::string name, const unsigned int x, const unsigned int y, const unsigned int z, const unsigned int w );
		void setUniformMat2( const std::string name, const glm::mat2 mat, const bool transpose = false );
		void setUniformMat3( const std::string name, const glm::mat3 mat, const bool transpose = false );
		void setUniformMat4( const std::string name, const glm::mat4 mat, const bool transpose = false );

		void setUniformMat4Array( const std::string name, const std::vector< glm::mat4 > mats, const bool transpose = false );

		GLint getFragDataLocation( const std::string name ) const;
		GLint getAttributeLocation( const std::string name ) const;
		GLint getUniformLocation( const std::string name ) const;

		/*
		*	does exactly what the name says
		*	'active' means: the uniform / attribute was not optimized away by the compiler
		*/
		void printActiveUniforms() const;
		void printActiveAttributes() const;

		std::string getProgramLog() const;

		/*
		*	can be used to determine if the GlslProgram can safely be used, given the current state
		*	of the opengl pipeline
		*/
		bool isValid() const;

		/*
		*	stuff for creating a GlslProgram manually. use the other create function ( above ), it's better
		*	the only reason these functions even exist, is because of transform feedbacks
		*/
		static std::shared_ptr< GlslProgram > create();
		bool addShader( const ShaderType shaderType, const std::string shaderSource );
		void recordInterleavedOutputs( const std::vector< std::string > outputs );
		// missing: function for non-interleaved
		bool link();

		// no copies & stuff
		GlslProgram( GlslProgram const& ) = delete;
		GlslProgram& operator=( GlslProgram const& ) = delete;
		GlslProgram( GlslProgram && ) = delete;
		GlslProgram& operator=( GlslProgram && ) = delete;

	private:

		// you shall not use these, ever
		~GlslProgram() = default;
		GlslProgram() = default;

		struct Deleter
		{
			void operator()( GlslProgram *& p ) const;
		};

		static std::weak_ptr< GlslProgram > sCurrentProgram;

		void queryActiveUniforms();
		void queryActiveAttributes();
		GLint getUniformLocation_Internal( const std::string ) const;

		struct VariableInfo
		{
			GLint	location;
			GLenum	type;
			GLint	size;
		};
		typedef std::map< std::string, VariableInfo >	LookupTable;

		GLuint					mHandle = 0;
		std::string				mProgramLog;
		LookupTable				mUniforms;
		LookupTable				mAttributes;
	};

}