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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>	
#include <netdb.h>

GLuint vbo = 0;
GLuint texture = 0;
GLuint faces = 0;
int serverSocket = 0;

float rotation = 0.0f;

#define VIEWING_OFFSET_Y -0.4f
#define VIEWING_DISTANCE_Z -8.0f

#define SERVER_PORT 4000

#define OBJ_MAX_SIZE 13200000
#define BMP_MAX_SIZE 3200000
#define MAX_TEXTURE_SIZE 1024
#define RECV_BUFFER_SIZE 2048

#define MSG_ROTATE_LEFT 1
#define MSG_ROTATE_RIGHT 2

#define ROTATE_SPEED 10.0f

static char s_arFileBuffer[OBJ_MAX_SIZE];
static char s_arRecvBuffer[RECV_BUFFER_SIZE];

static float triangleVerts[] = { -1.0f, -1.0f,
				 -1.0f,  1.0f,
				  0.0f,  1.0f,
				  0.0f,  1.0f,
				  1.0f,  1.0f,
				  1.0f,  -1.0f };
static float lineVerts[] = { -1.0f, -1.0f,
			      0.0f,  1.0f,
			      0.0f,  1.0f,
			      1.0f, -1.0f };

GLuint colorShader = 0;

 static const char* vShaderStr =  
      "attribute vec4 vPosition;    \n"
      "attribute vec2 vTexcoord;    \n"
      "attribute vec3 vNormal;      \n"
      "varying vec2 inTexcoord;\n"
      "varying vec3 inNormal;      \n"
      "uniform mat4 mViewMatrix;        \n"
      "uniform mat4 mModelMatrix;  \n"
      "void main()                  \n"
      "{                            \n"
      "   inTexcoord = vTexcoord;   \n"
      "   inNormal =  (mModelMatrix * vec4(vNormal, 0.0)).xyz;      \n"
      "   vec4 pos = vec4(vPosition.xyz, 1.0);        \n"
      "   gl_Position = mViewMatrix * mModelMatrix * pos;  \n"
      "}                            \n";
   
static const char* fShaderStr =  
      "precision mediump float;\n"
      "uniform sampler2D sTexture; \n"
      "varying vec2 inTexcoord;\n"
      "varying vec3 inNormal;\n"
      "void main()                                  \n"
      "{                                            \n"
      "  const vec3 lightDir = normalize(vec3(0.577, 0.577, 0.577));\n"
      "  vec4 texColor = texture2D(sTexture, inTexcoord);\n"
      "  texColor.rgb = texColor.rgb * max(dot(inNormal, lightDir) , 0.0);\n" // vec4(inNormal, 1.0) * texColor; \n"
      "  gl_FragColor = texColor;"
      "}                                            \n";

static const char* vColorShader = 
	"attribute vec4 aPosition; \n"
	"void main() \n"
        "{ vec4 pos = aPosition;\n"
	" pos.z = -0.9;\n"
	" gl_Position = pos;}\n"; // aPosition; } \n";

static const char* fColorShader = 
	"uniform vec4 uColor;\n"
	"void main() { gl_FragColor = uColor; }\n";


void UpdateServer();

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

GLuint CreateShaderProgram(const char* vertShader, const char* fragShader)
{
   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vertShader);
   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fragShader);

   // Create the program object
   programObject = glCreateProgram ();

   if ( programObject == 0 )
      return 0;

   glAttachShader ( programObject, vertexShader );
   glAttachShader ( programObject, fragmentShader );

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
      return 0;
   }

   return programObject;
}

///
// Initialize the shader and program object
//
int Init ( ESContext *esContext )
{
   esContext->userData = malloc(sizeof(UserData));

   UserData *userData = (UserData*) esContext->userData;

   // Store the program object
   userData->programObject = CreateShaderProgram(vShaderStr, fShaderStr);
   colorShader = CreateShaderProgram(vColorShader, fColorShader);

   glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );

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

	printf("Face Count: %d", nNumFaces);
}

GLuint LoadBMP(const char* path)
{
	GLuint texHandle = 0;

	ReadAsset(path, s_arFileBuffer, BMP_MAX_SIZE);
	char* data = s_arFileBuffer;

	unsigned char* pData = 0;
	unsigned char* pSrc = 0;

	int nWidth = 0;
	int nHeight = 0;
	unsigned short sBPP = 0;



	// Grab the dimensions of texture
	nWidth = *reinterpret_cast<int*>(&data[18]);
	nHeight = *reinterpret_cast<int*>(&data[22]);
	sBPP = *reinterpret_cast<short*>(&data[28]);

	printf("Width: %d\n", nWidth);
	printf("Height: %d\n", nHeight);
	printf("BPP: %d\n", sBPP);


	if (nWidth > MAX_TEXTURE_SIZE ||
		nHeight > MAX_TEXTURE_SIZE)
	{
		// Texture too big.
		printf("Texture too big!");
	}

	if (sBPP == 24)
	{
		pSrc = reinterpret_cast<unsigned char*>(&data[54]);

		pData = new unsigned char[nWidth * nHeight * 4];

		unsigned char* pDst = pData;

		int nPadding = (nWidth * 3) % 4;

		if (nPadding != 0)
			nPadding = 4 - nPadding;

		for (int i = 0; i < nHeight; i++)
		{
			for (int j = 0; j < nWidth; j++)
			{
				pDst[0] = pSrc[2];
				pDst[1] = pSrc[1];
				pDst[2] = pSrc[0];

				pDst += 3;
				pSrc += 3;
			}

			pSrc += nPadding;
		}

		// Create texture 
		// Generate and bind as current texture
		glGenTextures(1, &texHandle);
		glBindTexture(GL_TEXTURE_2D, texHandle);

		// Set default texture paramters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Allocate graphics memory and upload texture
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGB,
			nWidth,
			nHeight,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			pData);

		delete pData;
		pData = nullptr;
	}
	else
	{
		// Unsupported bits per pixel!
		printf("Unsupported BPP");
	}

	return texHandle;
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

	printf("Generated Veretex Buffer.\n");

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
void DrawTriangles()
{
	if (colorShader == 0)
		printf("Color shader not set.\n");

	glUseProgram(colorShader);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	int hColor = glGetUniformLocation(colorShader, "uColor");
	int hPosition = glGetAttribLocation(colorShader, "aPosition");

	float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glUniform4fv(hColor, 1, black);
	glVertexAttribPointer(hPosition, 2, GL_FLOAT, GL_FALSE, 0, triangleVerts);
	glEnableVertexAttribArray(hPosition);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawArrays(GL_TRIANGLES, 0, 3 * 2);
}

void DrawLines()
{

}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw ( ESContext *esContext )
{
   //rotation += ROTATE_SPEED;
   UserData *userData = (UserData*) esContext->userData;
   GLfloat vVertices[] = { 0.0f,  0.5f, 0.0f,
						   -0.5f, -0.5f, 0.0f,
							0.5f, -0.5f, 0.0f };

   // Set the viewport
   glViewport ( 0, 0, esContext->width, esContext->height );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // Use the program object
   glUseProgram ( userData->programObject );

   // Load VBO and attributes
   GLint hPosition = glGetAttribLocation(userData->programObject, "vPosition");
   if (hPosition == -1)
   {
	   printf("Failed to find position attribute\n");
   }

   GLint hTexcoord = glGetAttribLocation(userData->programObject, "vTexcoord");
   if (hTexcoord == -1)
   {
	printf("Failed to find texcoord attribute.\n ");
   }

   GLint hNormal = glGetAttribLocation(userData->programObject, "vNormal");
   if (hNormal == -1)
   {
	printf("Failed to find normal attribute\n");
   }

   GLint hTexture = glGetUniformLocation(userData->programObject, "sTexture");
   if (hTexture == -1)
   {
	printf("Texture sampler uniform not found.\n");
   }

   GLint hViewMatrix = glGetUniformLocation(userData->programObject, "mViewMatrix");
   GLint hModelMatrix = glGetUniformLocation(userData->programObject, "mModelMatrix");
   ESMatrix viewMatrix;
   ESMatrix modelMatrix;
   esMatrixLoadIdentity(&viewMatrix);
   esMatrixLoadIdentity(&modelMatrix);
   esFrustum(&viewMatrix, -0.025f, 0.025f, -0.017f, 0.017f, 0.1f, 1024.0f);
   esTranslate(&viewMatrix, 0.0f, VIEWING_OFFSET_Y, VIEWING_DISTANCE_Z);
   esRotate(&modelMatrix, rotation, 0.0f, 1.0f, 0.0f);
   glUniformMatrix4fv(hViewMatrix, 1, GL_FALSE, &viewMatrix.m[0][0]);
   glUniformMatrix4fv(hModelMatrix, 1, GL_FALSE, &modelMatrix.m[0][0]);

   glBindTexture(GL_TEXTURE_2D, texture);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);

   glUniform1i(hTexture, 0);

   glVertexAttribPointer ( hPosition, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0 );
   glVertexAttribPointer(hTexcoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (3 * sizeof(float)));
   glVertexAttribPointer(hNormal, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (5 * sizeof(float)));
   glEnableVertexAttribArray (hPosition);
   glEnableVertexAttribArray(hTexcoord);
   glEnableVertexAttribArray(hNormal);

   glEnable(GL_DEPTH_TEST);
   glDrawArrays ( GL_TRIANGLES, 0, 3 * faces );

   UpdateServer();

   DrawTriangles();
   DrawLines();
}


int InitServer()
{
	serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (serverSocket < 0)
	{
		printf("Failed to create socket.\n");
		return 0;
	}

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVER_PORT);

	if (bind(serverSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0)
	{
		printf("Failed to bind socket.\n");
		return 0;
	}

	timeval timeoutLength;
	timeoutLength.tv_sec = 0;
	timeoutLength.tv_usec = 10;
	setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &timeoutLength, sizeof(timeval));

	return 1;
}

void UpdateServer()
{
	struct sockaddr_in remoteAddr;
	socklen_t addrlen = sizeof(remoteAddr);

	if (serverSocket < 0)
	{
		return;
	}

	int recvLen = recvfrom(serverSocket, s_arRecvBuffer, RECV_BUFFER_SIZE, 0, (struct sockaddr*) &remoteAddr, &addrlen);

	if (recvLen > 0)
	{
		printf("Message Received: %d bytes \n", recvLen);

		float pitch = 0.0f;
		float yaw = 0.0f;
		float roll = 0.0f;

		sscanf(s_arRecvBuffer, "%f %f %f", &pitch, &yaw, &roll);

		printf("Yaw: %d\n", yaw);
		rotation = yaw;
	}
}


int main ( int argc, char *argv[] )
{
   ESContext esContext;
   UserData  userData;

   esInitContext ( &esContext );
   esContext.userData = &userData;

   esCreateWindow ( &esContext, "Ghost Renderer", 1920, 1080, ES_WINDOW_RGB | ES_WINDOW_DEPTH);

   if ( !Init ( &esContext ) )
      return 0;

   vbo = LoadOBJ("Models/Gun.obj", faces, nullptr);
   texture = LoadBMP("Textures/grid.bmp");

   InitServer();

   esRegisterDrawFunc ( &esContext, Draw );

   esMainLoop ( &esContext );

   close(serverSocket);
}
