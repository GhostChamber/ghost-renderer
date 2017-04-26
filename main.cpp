//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

// Hello_Triangle.c
//
//    This is a simple example that draws a single triangle with
//    a minimal vertex/fragment shader.  The purpose of this 
//    example is to demonstrate the basic concepts of 
//    OpenGL ES 2.0 rendering.
#include <stdlib.h>
#include "esUtil.h"
#include <stdio.h>

GLuint vbo = 0;
GLuint texture = 0;
GLuint faces = 0;

#define OBJ_MAX_SIZE 1048576
static char s_arFileBuffer[OBJ_MAX_SIZE];

typedef struct
{
   // Handle to a program object
   GLuint programObject;

} UserData;

///
// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint LoadShader ( GLenum type, const char *shaderSrc )
{
   GLuint shader;
   GLint compiled;
   
   // Create the shader object
   shader = glCreateShader ( type );

   if ( shader == 0 )
   	return 0;

   // Load the shader source
   glShaderSource ( shader, 1, &shaderSrc, NULL );
   
   // Compile the shader
   glCompileShader ( shader );

   // Check the compile status
   glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

   if ( !compiled ) 
   {
      GLint infoLen = 0;

      glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );
      
      if ( infoLen > 1 )
      {
         char* infoLog = (char*) malloc (sizeof(char) * infoLen );

         glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
         esLogMessage ( "Error compiling shader:\n%s\n", infoLog );            
         
         free ( infoLog );
      }

      glDeleteShader ( shader );
      return 0;
   }

   return shader;

}

///
// Initialize the shader and program object
//
int Init ( ESContext *esContext )
{
   esContext->userData = malloc(sizeof(UserData));

   UserData *userData = (UserData*) esContext->userData;
   const char* vShaderStr =  
      "attribute vec4 vPosition;    \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = vPosition;  \n"
      "}                            \n";
   
   const char* fShaderStr =  
      "precision mediump float;\n"\
      "void main()                                  \n"
      "{                                            \n"
      "  gl_FragColor = vec4 ( 0.7, 0.0, 0.0, 1.0 );\n"
      "}                                            \n";

   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vShaderStr );
   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fShaderStr );

   // Create the program object
   programObject = glCreateProgram ( );
   
   if ( programObject == 0 )
      return 0;

   glAttachShader ( programObject, vertexShader );
   glAttachShader ( programObject, fragmentShader );

   // Bind vPosition to attribute 0   
   glBindAttribLocation ( programObject, 0, "vPosition" );

   // Link the program
   glLinkProgram ( programObject );

   // Check the link status
   glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );

   if ( !linked ) 
   {
      GLint infoLen = 0;

      glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );
      
      if ( infoLen > 1 )
      {
         char* infoLog = (char*) malloc (sizeof(char) * infoLen );

         glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
         esLogMessage ( "Error linking program:\n%s\n", infoLog );            
         
         free ( infoLog );
      }

      glDeleteProgram ( programObject );
      return GL_FALSE;
   }

   // Store the program object
   userData->programObject = programObject;

   glClearColor ( 0.2f, 0.2f, 0.2f, 1.0f );
   return GL_TRUE;
}

void ReadAsset(const char* pFileName,
	char*       arBuffer,
	int         nMaxSize)
{
	FILE* pFile = 0;
	int   nFileSize = 0;

	// Open the requested file.
	pFile = fopen(pFileName, "rb");

	// Check if file was found
	if (pFile == 0)
	{
		printf("Asset could not be opened.");
		return;
	}

	// Check the file size.
	fseek(pFile, 0, SEEK_END);
	nFileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	// Check if file is too big
	if (nFileSize > nMaxSize)
	{
		printf("File is too large.");
		fclose(pFile);
		return;
	}

	// Clear the buffer that will hold contents of obj file
	memset(arBuffer, 0, nFileSize + 1);

	// Read from file and store in character buffer.
	fread(arBuffer, nFileSize, 1, pFile);

	// Close file, it is no longer needed.
	fclose(pFile);
}

void GetCounts(const char* pFileName,
	int& nNumVerts,
	int& nNumUVs,
	int& nNumNormals,
	int& nNumFaces)
{
	int nFileSize = 0;
	int nLineLength = 0;
	char* pNewLine = 0;
	char* pStr = 0;
	FILE* pFile = 0;

	nNumVerts = 0;
	nNumFaces = 0;
	nNumNormals = 0;
	nNumUVs = 0;

	ReadAsset(pFileName,
		s_arFileBuffer,
		OBJ_MAX_SIZE);

	pStr = s_arFileBuffer;

	while (1)
	{
		// Find the next newline
		pNewLine = strchr(pStr, '\n');

		// If a null pointer was returned, that means
		// a null terminator was hit.
		if (pNewLine == 0)
		{
			break;
		}

		// Parse the beginning of each line to count
		// the total number of vertices/UVs/normals/faces.
		if (pStr[0] == 'v' &&
			pStr[1] == ' ')
		{
			nNumVerts++;
		}
		else if (pStr[0] == 'v' &&
			pStr[1] == 't')
		{
			nNumUVs++;
		}
		else if (pStr[0] == 'v' &&
			pStr[1] == 'n')
		{
			nNumNormals++;
		}
		else if (pStr[0] == 'f')
		{
			nNumFaces++;
		}

		// Now move the pointer to the next line.
		pStr = pNewLine + 1;
	}
}

void GenerateVertexBuffer(int nNumFaces,
	float* pVertices,
	float* pUVs,
	float* pNormals,
	int*   pFaces,
	float* pVB)
{
	if (pVB != 0)
	{
		//Add vertex data for each face	
		for (int i = 0; i < nNumFaces; i++)
		{
			//Vertex 1
			pVB[i * 24 + 0] = pVertices[(pFaces[i * 9 + 0] - 1) * 3 + 0];
			pVB[i * 24 + 1] = pVertices[(pFaces[i * 9 + 0] - 1) * 3 + 1];
			pVB[i * 24 + 2] = pVertices[(pFaces[i * 9 + 0] - 1) * 3 + 2];
			pVB[i * 24 + 3] = pUVs[(pFaces[i * 9 + 1] - 1) * 2 + 0];
			pVB[i * 24 + 4] = pUVs[(pFaces[i * 9 + 1] - 1) * 2 + 1];
			pVB[i * 24 + 5] = pNormals[(pFaces[i * 9 + 2] - 1) * 3 + 0];
			pVB[i * 24 + 6] = pNormals[(pFaces[i * 9 + 2] - 1) * 3 + 1];
			pVB[i * 24 + 7] = pNormals[(pFaces[i * 9 + 2] - 1) * 3 + 2];

			//Vertex 2
			pVB[i * 24 + 8] = pVertices[(pFaces[i * 9 + 3] - 1) * 3 + 0];
			pVB[i * 24 + 9] = pVertices[(pFaces[i * 9 + 3] - 1) * 3 + 1];
			pVB[i * 24 + 10] = pVertices[(pFaces[i * 9 + 3] - 1) * 3 + 2];
			pVB[i * 24 + 11] = pUVs[(pFaces[i * 9 + 4] - 1) * 2 + 0];
			pVB[i * 24 + 12] = pUVs[(pFaces[i * 9 + 4] - 1) * 2 + 1];
			pVB[i * 24 + 13] = pNormals[(pFaces[i * 9 + 5] - 1) * 3 + 0];
			pVB[i * 24 + 14] = pNormals[(pFaces[i * 9 + 5] - 1) * 3 + 1];
			pVB[i * 24 + 15] = pNormals[(pFaces[i * 9 + 5] - 1) * 3 + 2];

			//Vertex 3
			pVB[i * 24 + 16] = pVertices[(pFaces[i * 9 + 6] - 1) * 3 + 0];
			pVB[i * 24 + 17] = pVertices[(pFaces[i * 9 + 6] - 1) * 3 + 1];
			pVB[i * 24 + 18] = pVertices[(pFaces[i * 9 + 6] - 1) * 3 + 2];
			pVB[i * 24 + 19] = pUVs[(pFaces[i * 9 + 7] - 1) * 2 + 0];
			pVB[i * 24 + 20] = pUVs[(pFaces[i * 9 + 7] - 1) * 2 + 1];
			pVB[i * 24 + 21] = pNormals[(pFaces[i * 9 + 8] - 1) * 3 + 0];
			pVB[i * 24 + 22] = pNormals[(pFaces[i * 9 + 8] - 1) * 3 + 1];
			pVB[i * 24 + 23] = pNormals[(pFaces[i * 9 + 8] - 1) * 3 + 2];
		}
	}
}

unsigned int LoadOBJ(const char*   pFileName,
	unsigned int& nFaces,
	float** pVertexArray)
{
	int i = 0;
	char* pStr = 0;
	char* pNewLine = 0;
	int v = 0;
	int n = 0;
	int t = 0;
	int f = 0;
	unsigned int unVBO = 0;
	float* pVertexBuffer = 0;

	int nNumVerts = 0;
	int nNumFaces = 0;
	int nNumNormals = 0;
	int nNumUVs = 0;

	float* pVertices = 0;
	float* pNormals = 0;
	float* pUVs = 0;
	int*   pFaces = 0;

	// Retrieve the counts for positions/UVs/normals/faces
	GetCounts(pFileName,
		nNumVerts,
		nNumUVs,
		nNumNormals,
		nNumFaces);

	// Set the number of faces output
	nFaces = nNumFaces;

	// Allocate arrays to store vertex data
	pVertices = new float[nNumVerts * 3];
	pNormals = new float[nNumNormals * 3];
	pUVs = new float[nNumUVs * 2];
	pFaces = new   int[nNumFaces * 9];

	// Set string pointer to beginning of file buffer
	pStr = s_arFileBuffer;

	// Loop until end of file, reading line by line.
	while (1)
	{
		pNewLine = strchr(pStr, '\n');

		if (pNewLine == 0)
		{
			break;
		}

		if (pStr[0] == 'v' &&
			pStr[1] == ' ')
		{
			// Extract X/Y/Z coordinates
			pStr = &pStr[2];
			pStr = strtok(pStr, " ");
			pVertices[v * 3] = (float)atof(pStr);
			pStr = strtok(0, " ");
			pVertices[v * 3 + 1] = (float)atof(pStr);
			pStr = strtok(0, " \n\r");
			pVertices[v * 3 + 2] = (float)atof(pStr);

			// Increase the number of vertices
			v++;
		}
		else if (pStr[0] == 'v' &&
			pStr[1] == 't')
		{
			// Extract U/V coordinates
			pStr = &pStr[3];
			pStr = strtok(pStr, " ");
			pUVs[t * 2] = (float)atof(pStr);
			pStr = strtok(0, " \n\r");
			pUVs[t * 2 + 1] = (float)atof(pStr);

			// Increase number of texcoords
			t++;
		}
		else if (pStr[0] == 'v' &&
			pStr[1] == 'n')
		{
			pStr = &pStr[3];
			pStr = strtok(pStr, " ");
			pNormals[n * 3] = (float)atof(pStr);
			pStr = strtok(0, " ");
			pNormals[n * 3 + 1] = (float)atof(pStr);
			pStr = strtok(0, " \n\r");
			pNormals[n * 3 + 2] = (float)atof(pStr);

			// Increase number of normals
			n++;
		}
		else if (pStr[0] == 'f')
		{
			pStr = &pStr[2];

			// Parse and convert all 9 indices.
			for (i = 0; i < 9; i++)
			{
				if (i == 0)
				{
					pStr = strtok(pStr, " /\n\r");
				}
				else
				{
					pStr = strtok(0, " /\n\r");
				}
				pFaces[f * 9 + i] = atoi(pStr);
			}

			// Increase the face count
			f++;
		}

		// Now move the pointer to the next line.
		pStr = pNewLine + 1;
	}

	// From the separate arrays, create one big buffer
	// that has the vertex data interleaved.
	pVertexBuffer = new float[nNumFaces * 24];
	GenerateVertexBuffer(nNumFaces,
		pVertices,
		pUVs,
		pNormals,
		pFaces,
		pVertexBuffer);

	// Free arrays made to hold data.
	delete[] pVertices;
	pVertices = 0;
	delete[] pNormals;
	pNormals = 0;
	delete[] pUVs;
	pUVs = 0;
	delete[] pFaces;
	pFaces = 0;

	// Create the Vertex Buffer Object and fill it with vertex data
	glGenBuffers(1, &unVBO);
	glBindBuffer(GL_ARRAY_BUFFER, unVBO);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(float) * (nNumFaces) * 24,
		pVertexBuffer,
		GL_STATIC_DRAW);

	// Delete the clientside buffer, since it is no longer needed.

	if (pVertexArray != 0)
	{
		*pVertexArray = pVertexBuffer;
	}
	else
	{
		delete[] pVertexBuffer;
		pVertexBuffer = 0;
	}

	return unVBO;
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw ( ESContext *esContext )
{
   UserData *userData = (UserData*) esContext->userData;
   GLfloat vVertices[] = { 0.0f,  0.5f, 0.0f,
						   -0.5f, -0.5f, 0.0f,
							0.5f, -0.5f, 0.0f };
      
   // Set the viewport
   glViewport ( 0, 0, esContext->width, esContext->height );
   
   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   // Use the program object
   glUseProgram ( userData->programObject );

   // Load VBO and attributes
   GLint hPosition = glGetAttribLocation(userData->programObject, "vPosition");
   if (hPosition == -1)
   {
	   printf("Failed to find position attribute");
   }

   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glVertexAttribPointer ( hPosition, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0 );
   glEnableVertexAttribArray ( 0 );

   glDrawArrays ( GL_TRIANGLES, 0, 3 * faces );
}

int main ( int argc, char *argv[] )
{
   ESContext esContext;
   UserData  userData;

   esInitContext ( &esContext );
   esContext.userData = &userData;

   esCreateWindow ( &esContext, "Hello Triangle", 1920, 1080, ES_WINDOW_RGB );

   if ( !Init ( &esContext ) )
      return 0;

   vbo = LoadOBJ("Models/Kat.obj", faces, nullptr);

   esRegisterDrawFunc ( &esContext, Draw );

   esMainLoop ( &esContext );
}
