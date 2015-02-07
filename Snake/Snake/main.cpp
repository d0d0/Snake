#define _USE_MATH_DEFINES
#include <windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include "shaderLoader.h"
#include "textureLoader.h"
#include "skybox.h"
#include "world.h"
#include "glew.h"
#include "glut-3.7.6-bin/glut.h"
#include "fonts.h"
#include "il.h"
#include <math.h>
#include "assimp/Importer.hpp"
#include "assimp/scene.h"           
#include "assimp/postprocess.h"
#include "Model.h"
#include "ObjectLoader.h"
#include "Sphere.h"
#include "snake.h"
#include <vector>
#include "glm\glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"

using namespace Assimp;
using namespace std;

GLuint  shaders_envmap, shaders_perpixel_phong, shaders_pervertex_phong, tex_point_sprite;
GLfloat sun_rotation = 0.0f;
GLdouble vzd, old_vzd, fi, xi, old_fi, old_xi;
GLint left_mouse_down_x, left_mouse_down_y;
GLint right_mouse_down_y;
GLboolean right_down = false, left_down = false;
skybox* sky;
world* wor;
Model obj;
snake* sn;
GLboolean isWired = false;
Model wall;
Sphere* slnko;

// casticovy system
#define NUM_PARTICLES 1000
struct Particle
{
	float position[3];
	float velocity[3];
	float color[4];
	float life;
};
Particle g_particle_system[NUM_PARTICLES];

ofstream logFile;
void reshape(int w, int h){

	// nastvenie casti okna pre vykreslenie, tentoraz kreslime do celeho okna
	glViewport(0, 0, w, h);
	// Budeme menit projekcnu maticu (transformaciu)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (h == 0){
		h = 1;
	}
	// Chceme perspektivnu projekciu
	float ratio = (float)w / (float)h;
	gluPerspective(60, ratio, 0.1, 1000.0);
}

float getRandomMinMax(float fMin, float fMax)
{
	float fRandNum = (float)rand() / RAND_MAX;
	return fMin + (fMax - fMin) * fRandNum;
}

void getRandomVector(float* vector)
{
	vector[2] = getRandomMinMax(0.4f, 1.0f);
	float radius = (float)sqrt(1 - vector[2] * vector[2]);
	float t = getRandomMinMax(-M_PI, M_PI);
	vector[0] = (float)cosf(t) * radius;
	vector[1] = (float)sinf(t) * radius;
}

void initParticles()
{
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		// emitter of all particles is point [0,0,0]
		g_particle_system[i].position[0] = 0;
		g_particle_system[i].position[1] = 0;
		g_particle_system[i].position[2] = 0;
		getRandomVector(g_particle_system[i].velocity);
		g_particle_system[i].color[0] = getRandomMinMax(0.0f, 1.0f);
		g_particle_system[i].color[1] = getRandomMinMax(0.0f, 1.0f);
		g_particle_system[i].color[2] = getRandomMinMax(0.0f, 1.0f);
		g_particle_system[i].color[3] = getRandomMinMax(0.0f, 0.4f);
		g_particle_system[i].life = getRandomMinMax(1.0f, 3.0f);
	}
}

void updateParticles()
{
	static double dLastFrameTime = timeGetTime();
	double dCurrenFrameTime = timeGetTime();
	double dElpasedFrameTime = (float)((dCurrenFrameTime - dLastFrameTime) * 0.001);
	dLastFrameTime = dCurrenFrameTime;
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		g_particle_system[i].position[0] += (float)dElpasedFrameTime * g_particle_system[i].velocity[0];
		g_particle_system[i].position[1] += (float)dElpasedFrameTime * g_particle_system[i].velocity[1];
		g_particle_system[i].position[2] += (float)dElpasedFrameTime * g_particle_system[i].velocity[2];
		g_particle_system[i].life -= dElpasedFrameTime;
		if (g_particle_system[i].life < 0 || g_particle_system[i].position[2] < 0)
		{
			g_particle_system[i].position[0] = 0;
			g_particle_system[i].position[1] = 0;
			g_particle_system[i].position[2] = 0;
			getRandomVector(g_particle_system[i].velocity);
			g_particle_system[i].color[0] = getRandomMinMax(0.0f, 1.0f);
			g_particle_system[i].color[1] = getRandomMinMax(0.0f, 1.0f);
			g_particle_system[i].color[2] = getRandomMinMax(0.0f, 1.0f);
			g_particle_system[i].color[3] = getRandomMinMax(0.0f, 0.4f);
			g_particle_system[i].life = getRandomMinMax(1.0f, 3.0f);
		}
	}
}


void renderParticles()
{
	glPushMatrix();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
	glTranslatef(3, 0, 0);
	glScalef(0.5, 0.5, 0.5);

	float quadratic[] = { 1.0f, 0.0f, 0.01f };
	glPointParameterfvARB(GL_POINT_DISTANCE_ATTENUATION_ARB, quadratic);
	glPointSize(50);
	glPointParameterfARB(GL_POINT_FADE_THRESHOLD_SIZE_ARB, 60.0f);
	glPointParameterfARB(GL_POINT_SIZE_MIN_ARB, 1.0f);
	glPointParameterfARB(GL_POINT_SIZE_MAX_ARB, 50.0f);

	glBindTexture(GL_TEXTURE_2D, tex_point_sprite);
	glTexEnvf(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_POINT_SPRITE_ARB);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_POINTS);
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		glColor4fv(g_particle_system[i].color);
		glVertex3fv(g_particle_system[i].position);
	}
	glEnd();
	glDisable(GL_POINT_SPRITE_ARB);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glPopMatrix();
}

void render(){
	// Vymaz (vypln) obrazovku a z-buffer definovanymi hodnotami
	if (isWired){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Nastav modelview maticu (transformaciu) na jednotkovu maticu
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Nastav view matice pre pohlad na svetovy suradnicovy system
	GLfloat camera_pos[4];
	camera_pos[0] = vzd*cos(fi)*cos(xi);
	camera_pos[1] = vzd*sin(fi)*cos(xi);
	camera_pos[2] = vzd*sin(xi);
	camera_pos[3] = 1;
	gluLookAt(camera_pos[0], camera_pos[1], camera_pos[2], 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	float view_matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);
	shaderLoader* sl = new shaderLoader();
	sl->SetShaderUniformMatrix4f(shaders_envmap, "view_matrix", view_matrix);

	glEnable(GL_LIGHTING);
	// vykresli suradnicove osi svetoveho systemu roznymi farbami
	glLineWidth(1);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(100.0f, 0.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 100.0f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 100.0f);
	glEnd();
	// vykresli skybox
	// skybox je dany vo svetovych suradniciach, akurat jeho stred je stale v bode, kdeje kamera
	sky->render();

	render2DTextFT(10, 45, "F1 - zapni/vypni animaciu");
	render3DTextGLUT(6, 0, 2, 0.01, GLUT_STROKE_ROMAN, "Projekt OpenGL - GLUT 3D text");

	wor->render();
	glPushMatrix();
	glUseProgram(shaders_perpixel_phong);
	GLfloat sun_position[] = { 10.0f, 10.0f, 10.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, sun_position);


	/*GLuint tex_enabled = glGetUniformLocation(shaders_perpixel_phong, "texturing_enabled");
	glDisable(GL_TEXTURE_2D);
	glUniform1i(tex_enabled, 0);*/

	glUseProgram(shaders_pervertex_phong);
	GLuint tex_enabled = glGetUniformLocation(shaders_pervertex_phong, "texturing_enabled");
	glUniform1i(tex_enabled, 0);

	/*
	glEnable(GL_LIGHTING);
	GLfloat earth_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat earth_diffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };
	GLfloat earth_specular[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, earth_ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, earth_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, earth_specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);
	*/
	glColor3f(1, 0, 0);

	glPopMatrix();
	sn->render();
	glUseProgram(0);
	glColor3f(1, 1, 1);

	renderParticles();
	glutSwapBuffers();
}


bool init(void)
{
	// Cierne pozadie
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Nastav z-buffer testovanie
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// nastav stencil buffer
	glClearStencil(0);

	// nastav zmiesavaciu funkciu
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// nastav vhodne mapovanie textur
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// initializuj kameru
	fi = 0.7f; xi = 0.7f; vzd = 10.0f;


	GLfloat light_ambient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat light_diffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };
	GLfloat light_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0);
	glEnable(GL_LIGHT0);

	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
	glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0);
	glEnable(GL_LIGHT1);

	glLightfv(GL_LIGHT3, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT3, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT3, GL_SPECULAR, light_specular);
	glLightf(GL_LIGHT3, GL_CONSTANT_ATTENUATION, 1.0);
	glEnable(GL_LIGHT3);

	glLightfv(GL_LIGHT2, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT2, GL_SPECULAR, light_specular);
	glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 1.0);
	glEnable(GL_LIGHT2);

	glLightfv(GL_LIGHT4, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT4, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT4, GL_SPECULAR, light_specular);
	glLightf(GL_LIGHT4, GL_CONSTANT_ATTENUATION, 1.0);
	glEnable(GL_LIGHT4);

	glLightfv(GL_LIGHT5, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT5, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT5, GL_SPECULAR, light_specular);
	glLightf(GL_LIGHT5, GL_CONSTANT_ATTENUATION, 1.0);
	glEnable(GL_LIGHT5);




	// parametre hmly
	glFogi(GL_FOG_MODE, GL_EXP2);
	GLfloat fogColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_DENSITY, 0.1f);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogf(GL_FOG_START, 3.0f);
	glFogf(GL_FOG_END, 20.0f);
	//glEnable(GL_FOG);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	return true;
}


// Procedura obsluhujuca stlacenie tlacitka mysi
void mouse_klik(int button, int state, int x, int y){
	switch (button){
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN){
			left_down = true;
			left_mouse_down_x = x;
			left_mouse_down_y = y;
			old_fi = fi;
			old_xi = xi;
		}
		else if (state == GLUT_UP){
			left_down = false;
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN){
			right_down = true;
			right_mouse_down_y = y;
			old_vzd = vzd;
		}
		else if (state == GLUT_UP){
			right_down = false;
		}
		break;
	default:
		break;
	}

	glutPostRedisplay();
}

void mouse_move(int x, int y){
	if (left_down == true){
		fi = old_fi + (left_mouse_down_x - x) / 200.0f;
		xi = old_xi + (y - left_mouse_down_y) / 200.0f;
		cout << "fi " << fi << endl;
		cout << "xi " << xi << endl;
		glutPostRedisplay();
	}

	if (right_down == true){
		if (old_vzd + (right_mouse_down_y - y) / 10.0f > 6.5f){
			vzd = old_vzd + (right_mouse_down_y - y) / 10.0f;
			glutPostRedisplay();
		}
		cout << "vzd" << vzd << endl;
	}

}

void timer(int a){
	//cout << sn->getWall() << endl;
	sun_rotation += 0.5f;
	sn->posun();
	glutPostRedisplay();
	glutTimerFunc(10, timer, a);
	updateParticles();
}

void keyboard(unsigned char key, int x, int y){
	switch (key){
	case 27:
		exit(0);
		break;
	case 'w':
		isWired = !isWired;
	default:
		break;
	}
}

void special_keys(int a_keys, int x, int y){
	//sn->addSphere();
	sn->setDirection(a_keys);
}


int main(int argc, char** argv){
	logFile.open("test.txt");
	// GLUT Inicializacia
	glutInit(&argc, argv);
	// Display Mod (Rgb a Double Buffering)
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
	//glutInitDisplayString("stencil>=8 rgb>=8 double depth>=24");
	// Nastavime pociatocnu poziciu a velkost okna
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(800, 600);
	// Vyrobime okno s danym menom
	glutCreateWindow("--== OpenGL projekt ==--");

	// Zinicializujeme GLEW
	glewInit();
	init();
	ilInit();
	sn = new snake();
	glutMouseFunc(mouse_klik);
	glutMotionFunc(mouse_move);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special_keys);
	glutTimerFunc(10, timer, 0);
	ObjectLoader loader = ObjectLoader();
	sky = new skybox();
	wor = new world();
	//loader.loadModel("hulk/hulk.obj");
	//loader.loadModel("mountain/Mountain Dew Code Red soda can.obj");
	loader.loadModel("apple/app.obj");
	obj = loader.getModel();
	loader.loadModel("wall/grade.obj");
	wall = loader.getModel();
	shaderLoader* sl = new shaderLoader();
	shaders_envmap = sl->loadProgram("shaders/perpixel_envmap.vert", "shaders/perpixel_envmap.frag");

	sl->SetShaderUniform1i(shaders_envmap, "color_texture", 0);
	sl->SetShaderUniform1i(shaders_envmap, "cubemap_texture", 1);

	shaders_pervertex_phong = sl->loadProgram("shaders/pervertex_phong.vert", "shaders/pervertex_phong.frag");
	int locationTexture = glGetUniformLocation(shaders_pervertex_phong, "color_texture");
	glUniform1i(locationTexture, 0);

	shaders_perpixel_phong = sl->loadProgram("shaders/perpixel_phong.vert", "shaders/perpixel_phong.frag");
	sl->SetShaderUniform1i(shaders_perpixel_phong, "color_texture", 0);

	textureLoader* tl = new textureLoader();
	tex_point_sprite = tl->LoadTexture("textures/particle.png");
	glutDisplayFunc(render);
	glutReshapeFunc(reshape);

	// Spustenie hlavneho okruhu zachytavania sprav
	glutMainLoop();
	logFile.close();
	return 0;
}