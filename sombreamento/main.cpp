
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdlib.h>
#include <GL/glu.h>
#include <GL/gl.h>
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_COMPARE_MODE 0x812F
#define GL_COMPARE_R_TO_TEXTURE 0x812F
#define GL_TEXTURE_COMPARE_FUNC 0x812F
#define GL_DEPTH_TEXTURE_MODE 0x812F

#include <cstdio>
#include <cmath>

class Scene {
	private:
		// Desenha o ch�o no plano Y=0.
		// O tamanho do plano � 200x200 com o seu centro em 0,0,0
		void drawFloor() {

			glColor3f(0.0, 1.0, 0.0);
			glBegin(GL_TRIANGLES);
				glNormal3f(0,1,0);
				glVertex3f(-100, 0, -100);
				glVertex3f(-100, 0,  100);
				glVertex3f( 100, 0, -100);
				glVertex3f( 100, 0,  100);
			glEnd();
		}

		// Desenha um torus para projetar a sombra
		// Posi��o do torus est� no centro a 25 pontos acima do plano
		void drawTorus() {
			glPushMatrix();
			// Alterando a posi��o e rota��o do torus
			glTranslatef(0, 25, 1);
			glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
			// Desenhando o torys
			glColor3f(0.7, 0.1, 0.6);
			glutSolidTorus(5, 10, 60, 60);
			glPopMatrix();
		}

	public:
		void draw() {
			drawTorus();
			drawFloor();
		}

		Scene() {}
		virtual ~Scene() {}
	};

class Camera {
	public:
		Camera() {}
		virtual ~Camera() {}

		// Prepara a c�mera.
		void setup(int window_width, int window_height) {
			glViewport(0,0,window_width,window_height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(60, 1, 85, 400);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(0, 200, 200,   // posi��o
					  0, 0, 0,       // olhando para...
					  0, 1, 0);      // up vector.
			//Limpa o buffer de cor e o Depth Buffer.
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
};

class Light{
	float lightAngleInRad;
	GLenum lightNumber;
	public:
		GLfloat lightPos[4];
		Light(GLenum _lightNumber) {
			lightAngleInRad = 0;
			lightNumber = _lightNumber;
		}
		virtual ~Light() {}

		void updatePosition() {
			lightAngleInRad += 3.1416/180.0f;
			lightPos[0] = sin(lightAngleInRad) * 100;
			lightPos[1] = 100;
			lightPos[2] = cos(lightAngleInRad) * 100;
			lightPos[3] = 1;
			glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
		}

		void enable() {
		    glEnable(lightNumber);
		    GLfloat lightColorAmb[4] = {0.4f, 0.4f, 0.4f, 1.0f};
		    GLfloat lightColorSpc[4] = {0.2f, 0.2f, 0.2f, 1.0f};
		    GLfloat lightColorDif[4] = {0.4f, 0.4f, 0.4f, 1.0f};
		    glLightfv(GL_LIGHT0, GL_AMBIENT, lightColorAmb);
		    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColorDif);
		    glLightfv(GL_LIGHT0, GL_SPECULAR, lightColorSpc);
		    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
		}

		void disable() {
			glDisable(lightNumber);
		}

		void draw() {
			glPushMatrix();
			// Alterando a posi��o e rota��o do torus
			glTranslatef(lightPos[0], lightPos[1], lightPos[2]);
			// Desenhando o torys
			glColor3f(1.0, 1.0, 0.0);
			glutSolidSphere(2, 40, 60);
			glPopMatrix();
		}
};

//Classe para criar o sombreamento
class ShadowMapping {
	// Considera apenas uma fonte de luz.
	GLuint shadowMapTexture;
	// Este tamanho n�o pode ser maior que a resolu��o da window.
	// Para isto, deve-se usar FBO com Off-screen rendering.
	int shadowMapSize;

	GLfloat textureTrasnformS[4];
	GLfloat textureTrasnformT[4];
	GLfloat textureTrasnformR[4];
	GLfloat textureTrasnformQ[4];
	private:
		void createDepthTexture() {
			//Create the shadow map texture
			glGenTextures(1, &shadowMapTexture);
			glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
			//Enable shadow comparison
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			//Shadow comparison should be true (ie not in shadow) if r<=texture
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
			//Shadow comparison should generate an INTENSITY result
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
		}

		void loadTextureTransform() {
			GLfloat lightProjectionMatrix[16];
			GLfloat lightViewMatrix[16];
			// Busca as matrizes de view e projection da luz
			glGetFloatv(GL_PROJECTION_MATRIX, lightProjectionMatrix);
			glGetFloatv(GL_MODELVIEW_MATRIX, lightViewMatrix);
			// Salva o estado da matrix mode.
			glPushAttrib(GL_TRANSFORM_BIT);
			glMatrixMode(GL_TEXTURE);
			glPushMatrix();
			//Calcula a matiz de proje��o
			GLfloat biasMatrix[16]= {0.5f, 0.0f, 0.0f, 0.0f,
									 0.0f, 0.5f, 0.0f, 0.0f,
									 0.0f, 0.0f, 0.5f, 0.0f,
									 0.5f, 0.5f, 0.5f, 1.0f};	//bias from [-1, 1] to [0, 1]

			GLfloat textureMatrix[16];

			// Aplica as 3 matrizes em uma s�, levando um fragmento em 3D para o espa�o
			// can�nico da c�mera.
			glLoadMatrixf(biasMatrix);
			glMultMatrixf(lightProjectionMatrix);
			glMultMatrixf(lightViewMatrix);
			glGetFloatv(GL_TEXTURE_MATRIX, textureMatrix);

			// Separa as colunas em arrays diferentes por causa da opengl
			for (int i=0; i<4; i++) {
				textureTrasnformS[i] = textureMatrix[i*4];
				textureTrasnformT[i] = textureMatrix[i*4+1];
				textureTrasnformR[i] = textureMatrix[i*4+2];
				textureTrasnformQ[i] = textureMatrix[i*4+3];
			}

			glPopMatrix();
			glPopAttrib();
		}

	public:
		//Respons�vel por colocar sombra
		void enableDepthCapture() {
			// Protege o c�digo anterior a esta fun��o
			glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT);
			// Se a textura ainda n�o tiver sido criada, crie
			if (!shadowMapTexture) createDepthTexture();
			// Seta a viewport com o mesmo tamanho da textura.
			// O tamanho da viewport n�o pode ser maior que o tamanho da tela.
			// SE for, deve-se usar offline rendering e FBOs.
			glViewport(0, 0, shadowMapSize, shadowMapSize);
			// Calcula a transforma��o do espa�o de c�mera para o espa�o da luz
			// e armazena a transforma��o para ser utilizada no teste de sombra do rendering
			loadTextureTransform();

			// Habilita Offset para evitar flickering.
			// Desloca o mapa de altura 1.9 vezes + 4.00 para tr�s.
			glPolygonOffset(1.9, 4.00);
			glEnable(GL_POLYGON_OFFSET_FILL);

			// Flat shading for speed
			glShadeModel(GL_FLAT);
			// Disable Lighting for performance.
			glDisable(GL_LIGHTING);
			// N�o escreve no buffer de cor, apenas no depth
			glColorMask(0, 0, 0, 0);
		}

		void disableDepthCapture() {
			// Copia o Depth buffer para a textura.
			glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
			// SubTexture n�o realoca a textura toda, como faz o texture
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, shadowMapSize, shadowMapSize);
			// Limpa o Buffer de profundidade
			glClear(GL_DEPTH_BUFFER_BIT);
			// Retorna as configura��es anteriores ao depthCapture
			glPopAttrib();
		}

		void enableShadowTest() {
			// Protege o c�digo anterior a esta fun��o
			glPushAttrib(GL_TEXTURE_BIT |  GL_ENABLE_BIT);
			// Habilita a gera��o autom�tica de coordenadas de textura do ponto de vista da c�mera
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
			glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
			glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
			// Aplica a transforma��o destas coordenadas para o espa�o da luz
			glTexGenfv(GL_S, GL_EYE_PLANE, textureTrasnformS);
			glTexGenfv(GL_T, GL_EYE_PLANE, textureTrasnformT);
			glTexGenfv(GL_R, GL_EYE_PLANE, textureTrasnformR);
			glTexGenfv(GL_Q, GL_EYE_PLANE, textureTrasnformQ);
			// Ativa
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glEnable(GL_TEXTURE_GEN_R);
			glEnable(GL_TEXTURE_GEN_Q);

			//Bind & enable shadow map texture
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		}

		void disableShadowTest() {
			// Retorna as configura��es anteriores do programa
			glPopAttrib();
		}

		ShadowMapping(int resolution = 512) {
			//shadowMapTexture = NULL;
			shadowMapSize = resolution;
		}
		virtual ~ShadowMapping() {
			//shadowMapTexture = NULL;
			glDeleteTextures(1, &shadowMapTexture);
		}
	};

	int SCREEN_SIZE = 600;
	Scene scene;
	Light light(GL_LIGHT0);
	Camera camera;
	ShadowMapping shadowRenderer;

	// Prepara a c�mera real.
	void setupCameraInLightPosition() {
		//Use viewport the same size as the shadow map
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		// fovy, aspect, near, far.
		gluPerspective(60, 1, 85, 400);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		gluLookAt(light.lightPos[0], light.lightPos[1], light.lightPos[2],   // posi��o
				  0, 0, 0, // olhando para...
				  0, 1, 0); // up vector.
	}

	// Callback da glut.
	void display() {
		// Limpa os Buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Posiciona uma camera na posi��o da luz
		setupCameraInLightPosition();

		shadowRenderer.enableDepthCapture();
		scene.draw();
		shadowRenderer.disableDepthCapture();

		camera.setup(SCREEN_SIZE, SCREEN_SIZE);

	    shadowRenderer.enableShadowTest();
	    light.enable();
	    scene.draw();
		light.disable();
	    shadowRenderer.disableShadowTest();
		light.draw();
		glutSwapBuffers();
		glutPostRedisplay();  // Pede para a glut chamar esta fun��o novamente
	}

	// Cria o buffer para renderizar a primeira passada.
	void setupGL() {
		// Configura��o dos Algoritimos de Visibilidade
	    glClearDepth(1.0f);       // M�ximo valor para a Limpeza do DepthBuffer
	    glEnable(GL_DEPTH_TEST);  // Habilita o teste usando o DepthBuffer
	    glEnable(GL_CULL_FACE);   // Habilita o corte das faces
	    glCullFace(GL_BACK);      // Configura o corte para as faces cuja normal n�o aponta na dire��o da camera

	    glShadeModel(GL_SMOOTH);

	    // Illumination
	    glEnable(GL_LIGHTING);
	    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
	    glEnable(GL_COLOR_MATERIAL);
	}

	void updateLight(int value) {
		light.updatePosition();
		glutTimerFunc(10,updateLight,0);
	}

	void createWindow(int argc, char *argv[]) {
		// Inicializa��o Glut
		glutInit(&argc, argv);
		// Configurando a janela para aceitar o Double Buffer, Z-Buffer e Alpha.
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
		// Tamanho da Janela
		glutInitWindowSize(SCREEN_SIZE, SCREEN_SIZE);
		// Cria efetivamente a janela e o contexto OpenGL.
		glutCreateWindow("Desenho de uma cena com ilumina��o/sombreamento");
		// Configura a Callback que desenhar� na tela.
		glutDisplayFunc(display);
		// Configura a atualiza��o da luz
		glutTimerFunc(10,updateLight,0);
	}

	int main(int argc, char *argv[]) {
		// Cria uma Janela OpenGL.
		createWindow(argc, argv);
		// Configura a m�quina de estados da OpenGL
		setupGL();

		// Loop da Glut.
		glutMainLoop();
		return 0;
	}
