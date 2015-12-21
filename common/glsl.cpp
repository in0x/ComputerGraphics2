#include "glsl.h"

#include <iostream>
#include <sstream>
#include <fstream>

namespace cg2
{
	std::string loadShaderSource( const std::string filepath )
	{
		std::string source;

		std::ifstream file( filepath.c_str() );
		if (!file.is_open())
		{
			std::ostringstream msg;
			std::cout << "could not open shader source file: " << filepath << ", are you sure the file exists?" << std::endl;
			return source;
		}

		while (!file.eof())
		{
			std::string line;
			getline( file, line );
			source.append( line );
			source.append( "\n" );
		}

		file.close();

		return source;
	}

	struct TmpProgramBinding
	{
		TmpProgramBinding( std::shared_ptr< GlslProgram > t, std::shared_ptr< GlslProgram > o ) : tmpProg( t ), oldProg( o )
		{
			if ( oldProg != tmpProg )
				GlslProgram::setActiveProgram( tmpProg );
		}

		~TmpProgramBinding()
		{
			if ( oldProg != tmpProg )
				GlslProgram::setActiveProgram( oldProg );
		}

		std::shared_ptr< GlslProgram > tmpProg;
		std::shared_ptr< GlslProgram > oldProg;
	};

	std::weak_ptr< GlslProgram > GlslProgram::sCurrentProgram = std::weak_ptr< GlslProgram >();

	void GlslProgram::setActiveProgram( std::shared_ptr< GlslProgram > p )
	{
		sCurrentProgram = p;
		if ( p.get() )	glUseProgram( p->mHandle );
		else			glUseProgram( 0 );
	}

	std::shared_ptr< GlslProgram > GlslProgram::create( std::vector< ShaderInfo > shaders, const bool printLog )
	{
		std::shared_ptr< GlslProgram > result( new GlslProgram, GlslProgram::Deleter() );

		std::shared_ptr< GlslProgram > p( new GlslProgram, GlslProgram::Deleter() );
		p->mHandle = glCreateProgram();

		bool ok = true;
		for ( ShaderInfo const& s : shaders )
		{
			ok &= p->addShader( s.type, s.source );
		}
		ok &= p->link();

		if ( !ok )
		{
			std::cout << "--- program error log ---" << std::endl;
			std::cout << p->getProgramLog() << std::endl;
		}
		else
		{
			if ( printLog )
			{
				std::cout << "--- program attrib & uniform info ---" << std::endl;
				p->printActiveUniforms();
				p->printActiveAttributes();
			}
			result = p;
		}

		return result;
	}

	std::shared_ptr< GlslProgram > GlslProgram::create()
	{
		std::shared_ptr< GlslProgram > p( new GlslProgram, GlslProgram::Deleter() );
		p->mHandle = glCreateProgram();
		return p;
	}

	void GlslProgram::Deleter::operator()( GlslProgram *& p ) const
	{
		if ( p == nullptr )
			return;

		glDeleteProgram( p->mHandle );

		delete p;
		p = nullptr;
	}

	bool GlslProgram::addShader( const ShaderType shaderType, const std::string shaderSource )
	{
		GLenum type;
		switch ( shaderType )
		{
		case ShaderType::COMPUTE_SHADER:
			type = GL_COMPUTE_SHADER;
			break;
		case ShaderType::VERTEX_SHADER:
			type = GL_VERTEX_SHADER;
			break;
		case ShaderType::FRAGMENT_SHADER:
			type = GL_FRAGMENT_SHADER;
			break;
		case ShaderType::GEOMETRY_SHADER:
			type = GL_GEOMETRY_SHADER;
			break;
		case ShaderType::TESSELLATION_CONTROL_SHADER:
			type = GL_TESS_CONTROL_SHADER;
			break;
		case ShaderType::TESSELLATION_EVALUATION_SHADER:
			type = GL_TESS_EVALUATION_SHADER;
			break;
		default:
			std::cout << "unknown shader type, was ignored " << std::endl;
			return false;
		}

		GLuint handle = glCreateShader( type );

		const char* source = shaderSource.c_str();
		glShaderSource( handle, 1, &source, nullptr );
		glCompileShader( handle );

		GLint compileStatus;
		glGetShaderiv( handle, GL_COMPILE_STATUS, &compileStatus );

		GLint sz = 0;
		glGetShaderiv( handle, GL_INFO_LOG_LENGTH, &sz );
		if (sz > 0)
		{
			char *buffer = new char[ sz ];
			memset( buffer, 0, sz );
			glGetShaderInfoLog( handle, sz, NULL, buffer );
			mProgramLog.append( buffer );
			delete[] buffer;
		}

		if (!compileStatus)
			return false;

		glAttachShader( mHandle, handle );
		return true;
	}

	bool GlslProgram::link()
	{
		glLinkProgram( mHandle );

		GLint linkStatus;
		glGetProgramiv( mHandle, GL_LINK_STATUS, &linkStatus );

		GLint sz = 0;
		glGetProgramiv( mHandle, GL_INFO_LOG_LENGTH, &sz );
		if (sz > 0)
		{
			char *buffer = new char[ sz ];
			memset( buffer, 0, sz );
			glGetProgramInfoLog( mHandle, sz, NULL, buffer );
			mProgramLog.append( buffer );
			delete[] buffer;
		}

		if (!linkStatus)
			return false;

		queryActiveAttributes();
		queryActiveUniforms();
		return true;
	}

	bool GlslProgram::isValid() const
	{
		glValidateProgram( mHandle );
		GLint validateStatus;
		glGetProgramiv( mHandle, GL_VALIDATE_STATUS, &validateStatus );
		return (validateStatus ? true : false);
	}

	std::string GlslProgram::getProgramLog() const
	{
		return mProgramLog;
	}

	void GlslProgram::recordInterleavedOutputs( const std::vector< std::string > outputs )
	{
		std::vector< char const* > tmp;	//ugh
		for (unsigned int i = 0; i<outputs.size(); ++i) tmp.push_back( outputs[ i ].c_str() );
		glTransformFeedbackVaryings( mHandle, tmp.size(), &tmp[ 0 ], GL_INTERLEAVED_ATTRIBS );
	}

	void GlslProgram::queryActiveUniforms()
	{
		GLint count, maxLength;
		glGetProgramiv( mHandle, GL_ACTIVE_UNIFORMS, &count );
		glGetProgramiv( mHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength );

		char* buffer = new char[ maxLength ];
		memset( buffer, 0, maxLength );

		GLenum type;
		GLint size, location;
		std::string name;

		for (int i = 0; i < count; ++i)
		{
			glGetActiveUniform( mHandle, i, maxLength, nullptr, &size, &type, buffer );
			name = buffer;

			location = glGetUniformLocation( mHandle, buffer );
			VariableInfo info;
			info.location = location;
			info.size = size;
			info.type = type;
			mUniforms[ name ] = info;
		}

		delete[] buffer;
	}

	void GlslProgram::queryActiveAttributes()
	{
		GLint count, maxLength;
		glGetProgramiv( mHandle, GL_ACTIVE_ATTRIBUTES, &count );
		glGetProgramiv( mHandle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength );

		char* buffer = new char[ maxLength ];
		memset( buffer, 0, maxLength );

		GLint size, location;
		GLenum type;
		std::string name;

		for (int i = 0; i < count; ++i)
		{
			glGetActiveAttrib( mHandle, i, maxLength, nullptr, &size, &type, buffer );
			name = buffer;
			location = glGetAttribLocation( mHandle, buffer );
			VariableInfo info;
			info.location = location;
			info.size = size;
			info.type = type;
			mAttributes[ name ] = info;
		}

		delete[] buffer;
	}

	void GlslProgram::printActiveUniforms() const
	{
		for (auto it = mUniforms.begin(); it != mUniforms.end(); ++it)
			std::cout << "active uniform\t\t" << it->first << " at ( uniform ) location " << it->second.location << std::endl;
	}

	void GlslProgram::printActiveAttributes() const
	{
		for (auto it = mAttributes.begin(); it != mAttributes.end(); ++it)
			std::cout << "active attribute\t" << it->first << " at ( attribute ) location " << it->second.location << std::endl;
	}

	void GlslProgram::setUniformTexVal( const std::string name, const unsigned int texUnit )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform1i( location, texUnit );
	}

	void GlslProgram::setUniformVal( const std::string name, const float v )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform1f( location, v );
	}

	void GlslProgram::setUniformIVal( const std::string name, const int v )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform1i( location, v );
	}

	void GlslProgram::setUniformUVal( const std::string name, const unsigned int v )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform1ui( location, v );
	}

	void GlslProgram::setUniformVec2( const std::string name, const float x, const float y )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform2f( location, x, y );
	}

	void GlslProgram::setUniformVec3( const std::string name, const float x, const float y, const float z )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform3f( location, x, y, z );
	}

	void GlslProgram::setUniformVec4( const std::string name, const float x, const float y, const float z, const float w )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform4f( location, x, y, z, w );
	}

	void GlslProgram::setUniformIVec2( const std::string name, const int x, const int y )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform2i( location, x, y );
	}

	void GlslProgram::setUniformIVec3( const std::string name, const int x, const int y, const int z )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform3i( location, x, y, z );
	}

	void GlslProgram::setUniformIVec4( const std::string name, const int x, const int y, const int z, const int w )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform4i( location, x, y, z, w );
	}

	void GlslProgram::setUniformUVec2( const std::string name, const unsigned int x, const unsigned int y )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform2ui( location, x, y );
	}

	void GlslProgram::setUniformUVec3( const std::string name, const unsigned int x, const unsigned int y, const unsigned int z )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform3ui( location, x, y, z );
	}

	void GlslProgram::setUniformUVec4( const std::string name, const unsigned int x, const unsigned int y, const unsigned int z, const unsigned int w )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniform4ui( location, x, y, z, w );
	}

	void GlslProgram::setUniformMat2( const std::string name, const glm::mat2 mat, const bool transpose )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniformMatrix2fv( location, 1, transpose, std::addressof( mat[ 0 ][ 0 ] ) );
	}

	void GlslProgram::setUniformMat3( const std::string name, const glm::mat3 mat, const bool transpose )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniformMatrix3fv( location, 1, transpose, std::addressof( mat[ 0 ][ 0 ] ) );
	}

	void GlslProgram::setUniformMat4( const std::string name, const glm::mat4 mat, const bool transpose )
	{
		GLint location = getUniformLocation_Internal( name );
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );
		glUniformMatrix4fv( location, 1, transpose,  std::addressof( mat[ 0 ][ 0 ] ) );
	}

	void GlslProgram::setUniformMat4Array( const std::string name, const std::vector< glm::mat4 > mats, const bool transpose )
	{
		TmpProgramBinding tmp( shared_from_this(), sCurrentProgram.lock() );

		std::stringstream base;
		base << name << "[" << 0 << "]";
		GLint location = getUniformLocation_Internal( base.str() );

		for ( unsigned int i=0; i<mats.size(); ++i )
			glUniformMatrix4fv( location+i, 1, GL_FALSE, std::addressof( mats[ i ][ 0 ][ 0 ] ) );
	}

	GLint GlslProgram::getUniformLocation_Internal( const std::string name ) const
	{
		GLint location = getUniformLocation( name );
		//if (location == -1)
		//	std::cout << name << std::endl;
		//	std::cout << "the uniform " << name << " does not exist, please double-check the spelling ( case-sensititve! ) / or maybe your glsl compiler decided it was superfluous ( do you even use it? )" << std::endl;
		return location;
	}

	GLint GlslProgram::getUniformLocation( const std::string name ) const
	{
		GLint location = -1;
		auto it = mUniforms.find( name );
		if ( it != mUniforms.end() )
			location = it->second.location;

		return location;
	}

	GLint GlslProgram::getFragDataLocation( const std::string name ) const
	{
		return glGetFragDataLocation( mHandle, name.c_str() );
	}

	GLint GlslProgram::getAttributeLocation( const std::string name ) const
	{
		GLint location = -1;
		auto it = mAttributes.find( name );
		if ( it != mAttributes.end() )
			location = it->second.location;

		return location;
	}
}