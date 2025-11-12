/************************************************************************
     File:        TrainView.cpp

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						The TrainView is the window that actually shows the 
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within 
						a TrainWindow
						that is the outer window with all the widgets. 
						The TrainView needs 
						to be aware of the window - since it might need to 
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know 
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <algorithm>
#include <ctime>
#include <cmath>
#include <iostream>
#include <Fl/fl.h>
#include "GL/glu.h"

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"


#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif

//************************************************************************
//
// * Shader
//========================================================================
#include "RenderUtilities/BufferObject.h";
#include "RenderUtilities/Shader.h";
#include "RenderUtilities/Texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define M_PI 3.14159265359
static std::time_t startTime = std::time(nullptr);

void TrainView::setUBO() {
	float wdt = this->pixel_w();
	float hgt = this->pixel_h();

	glm::mat4 view_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);

	glm::mat4 projection_matrix;
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);

	glBindBuffer(GL_UNIFORM_BUFFER, this->common_matrices->ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection_matrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view_matrix[0][0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l) 
	: Fl_Gl_Window(x,y,w,h,l)
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) 
			return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		// Mouse button being pushed event
		case FL_PUSH:
			last_push = Fl::event_button();
			// if the left button be pushed is left mouse button
			if (last_push == FL_LEFT_MOUSE  ) {
				doPick();
				damage(1);
				return 1;
			};
			break;

	   // Mouse button release event
		case FL_RELEASE: // button release
			damage(1);
			last_push = 0;
			return 1;

		// Mouse button drag event
		case FL_DRAG:

			// Compute the new control point position
			if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
				ControlPoint* cp = &m_pTrack->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

				double rx, ry, rz;
				mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z, 
								static_cast<double>(cp->pos.x), 
								static_cast<double>(cp->pos.y),
								static_cast<double>(cp->pos.z),
								rx, ry, rz,
								(Fl::event_state() & FL_CTRL) != 0);

				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;

		// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;

		// every time the mouse enters this window, aggressively take focus
		case FL_ENTER:	
			focus(this);
			break;

		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k == 'p') {
					// Print out the selected control point information
					if (selectedCube >= 0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
								 selectedCube,
								 m_pTrack->points[selectedCube].pos.x,
								 m_pTrack->points[selectedCube].pos.y,
								 m_pTrack->points[selectedCube].pos.z,
								 m_pTrack->points[selectedCube].orient.x,
								 m_pTrack->points[selectedCube].orient.y,
								 m_pTrack->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");

					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	
	if (gladLoadGL())
	{
		if(tw->shaderBrowser->value() == 1)
			drawCastle();
		else if(tw->shaderBrowser->value() == 2) {
			drawColoredCastle();
		}else {
			drawWave(startTime);
		}
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	
	

	// Set up the view port
	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	// ensure depth testing is enabled (GL_DEPTH is invalid enum)
	glEnable(GL_DEPTH_TEST);

	// Blayne prefers GL_DIFFUSE
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);

	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} 

	switch (tw->lightBrowser->value()) {
	case 1:
	case 2:
		glDisable(GL_LIGHT3);
		glEnable(GL_LIGHT0);
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
		break;
	case 3:
		// Keep a weak GL_LIGHT0 as a fill light so the scene never becomes
		// completely dark if the spotlight misses or is clipped.
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
		glEnable(GL_LIGHT3);
		break;
	}
	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	GLfloat lightPosition1[]	= {0,1,1,0}; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[]	= {1, 0, 0, 0};
	GLfloat lightPosition3[]	= {0, -1, 0, 0};
	GLfloat yellowLight[]		= {0.5f, 0.5f, .1f, 1.0};
	GLfloat whiteLight[]			= {1.0f, 1.0f, 1.0f, 1.0};
	GLfloat blueLight[]			= {.1f,.1f,.3f,1.0};
	GLfloat grayLight[]			= {.3f, .3f, .3f, 1.0};

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);

	// Ensure we are in MODELVIEW when setting light positions/directions
	// otherwise the positions get transformed by the projection matrix
	glMatrixMode(GL_MODELVIEW);

	// directional light
	if (tw->lightBrowser->value() == 2) {
		float noAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float whiteDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float position[] = { 1.0f, 1.0f, 0.0f, 0.0f };
		glLightfv(GL_LIGHT0, GL_AMBIENT, noAmbient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteDiffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, position);
	}
	else if (tw->lightBrowser->value() == 3) {

		// spot-light parameters will be set later (after the modelview/camera
		// transform is established) so the position/direction are in the
		// correct coordinate space. See the setup after setupObjects().
		GLfloat spotPos[] = { 0.0f, 100.0f, 0.0f, 1.0f };
		glLightfv(GL_LIGHT3, GL_POSITION, spotPos);
		// Direction pointing downwards
		GLfloat spotDir[] = { 0.0f, -1.0f, 0.0f };
		glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, spotDir);
		// Make the cone a bit wider and moderate exponent
		glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 35.0f);
		glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 8.0f);
		// Blueish color components (ambient / diffuse / specular)
		GLfloat ambient3[]  = { 0.02f, 0.03f, 0.08f, 1.0f };
		GLfloat diffuse3[]  = { 0.2f, 0.35f, 1.0f, 1.0f };
		GLfloat specular3[] = { 0.3f, 0.45f, 1.0f, 1.0f };
		glLightfv(GL_LIGHT3, GL_AMBIENT, ambient3);
		glLightfv(GL_LIGHT3, GL_DIFFUSE, diffuse3);
		glLightfv(GL_LIGHT3, GL_SPECULAR, specular3);
		// Simple attenuation so spotlight fades with distance
		glLightf(GL_LIGHT3, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION, 0.02f);
		glLightf(GL_LIGHT3, GL_QUADRATIC_ATTENUATION, 0.005f);
		glLightfv(GL_LIGHT3, GL_AMBIENT, ambient3);
		glLightfv(GL_LIGHT3, GL_DIFFUSE, diffuse3);
		glLightfv(GL_LIGHT3, GL_SPECULAR, specular3);
		// Use constant attenuation so the spotlight doesn't disappear when zooming
		glLightf(GL_LIGHT3, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION, 0.0f);
		glLightf(GL_LIGHT3, GL_QUADRATIC_ATTENUATION, 0.0f);
		// increase global ambient a touch so scene isn't so dark
		GLfloat globalAmbient[] = { 0.12f, 0.12f, 0.12f, 1.0f };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
		}
	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	if (tw->lightBrowser->value() == 1) {
		glDisable(GL_LIGHTING);
	}
	drawFloor(200,10);


	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();

	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}

	setUBO();
	glBindBufferRange(
		GL_UNIFORM_BUFFER, /*binding point*/0, this->common_matrices->ubo, 0, this->common_matrices->size);

	//bind shader
	this->shader->Use();

	glm::mat4 model_matrix = glm::mat4();
	model_matrix = glm::translate(model_matrix, glm::vec3(0, 10.0f, 0.0f));
	model_matrix = glm::scale(model_matrix, glm::vec3(10.0f, 10.0f, 10.0f));
	glUniformMatrix4fv(
		glGetUniformLocation(this->shader->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);
	glUniform3fv(
		glGetUniformLocation(this->shader->Program, "u_color"),
		1,
		&glm::vec3(1.0f, 1.0f, 0.0f)[0]);
	this->texture->bind(0);
	glUniform1i(glGetUniformLocation(this->shader->Program, "u_texture"), 0);

	//bind VAO
	glBindVertexArray(this->plane->vao);
	glDrawElements(GL_TRIANGLES, this->plane->element_amount, GL_UNSIGNED_INT, 0);

	//unbind VAO
	glBindVertexArray(0);

	//unbind shader(switch to fixed pipeline)
	glUseProgram(0);
}


//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->worldCam->value())
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		} 
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} 
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {
#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	// draw the track
	//####################################################################
	// TODO: 
	// // Draw the control points
    // don't draw the control points if you're driving 
    // (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if (((int)i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
}
	}
	// draw the track
	//####################################################################
	if(tw->shaderBrowser->value() == 3)
		updateWater(static_cast<float>(std::difftime(std::time(nullptr), startTime)), 100, 10.0f);

	drawTrack(doingShadows);
	drawSleepers(doingShadows);

	/*
	// Train-attached spotlight (disabled per user request).
	// If you want to enable it again, remove the surrounding comment block.
	if (!doingShadows && tw->lightBrowser->value() == 3) {
		if (!sleeperSamples.empty() && m_pTrack) {
			const size_t count = sleeperSamples.size();
			float u = m_pTrack->trainU;
			float total = static_cast<float>(count);
			if (total > 0.0f) {
				u = std::fmod(u, total);
				if (u < 0.0f) u += total;
			} else u = 0.0f;
			size_t idx0 = static_cast<size_t>(std::floor(u));
			if (idx0 >= count) idx0 = count - 1;
			size_t idx1 = (idx0 + 1) % count;
			float interp = (count > 1) ? (u - static_cast<float>(idx0)) : 0.0f;
			const SplineSample &s0 = sleeperSamples[idx0];
			const SplineSample &s1 = sleeperSamples[idx1];
			Pnt3f pos = (count > 1) ? lerp(s0.pos, s1.pos, interp) : s0.pos;
			Pnt3f up = (count > 1) ? normalizeVector(lerp(s0.orient, s1.orient, interp)) : normalizeVector(s0.orient);
			Pnt3f forward = (count > 1) ? normalizeVector(lerp(s0.tangent, s1.tangent, interp)) : normalizeVector(s0.tangent);
			if (lengthSquared(up) < 1e-6f) up = Pnt3f(0.0f, 1.0f, 0.0f);
			if (lengthSquared(forward) < 1e-6f) forward = Pnt3f(0.0f, 0.0f, 1.0f);
			forward.normalize(); up.normalize();
			Pnt3f right = forward * up;
			if (lengthSquared(right) < 1e-6f) {
				up = Pnt3f(0.0f, 1.0f, 0.0f);
				right = forward * up;
			}
			right.normalize(); up = right * forward; up.normalize();

			const float halfSize = 3.0f;
			const Pnt3f cubeCenter = pos + up * halfSize; // same as drawTrain

			GLfloat lightPos[] = { cubeCenter.x, cubeCenter.y, cubeCenter.z, 1.0f };
			// Ensure the light is enabled and set a forward-and-down direction
			glEnable(GL_LIGHT3);
			// bias the spot direction slightly downward so it lights the ground in front
			Pnt3f downBias = Pnt3f(0.0f, -0.35f, 0.0f);
			Pnt3f rawDir = forward + downBias;
			rawDir.normalize();
			GLfloat spotDir[] = { rawDir.x, rawDir.y, rawDir.z };
			glLightfv(GL_LIGHT3, GL_POSITION, lightPos);
			glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, spotDir);
			// reinforce cutoff/exponent and intensities in case previous setup was skipped
			glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 30.0f);
			glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 20.0f);
			GLfloat ambient3[] = { 0.05f, 0.05f, 0.05f, 1.0f };
			GLfloat diffuse3[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			GLfloat spec3[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glLightfv(GL_LIGHT3, GL_AMBIENT, ambient3);
			glLightfv(GL_LIGHT3, GL_DIFFUSE, diffuse3);
			glLightfv(GL_LIGHT3, GL_SPECULAR, spec3);
		}
	}
	*/
	// call your own track drawing code
	//####################################################################
	if (!tw->trainCam->value())
		drawTrain(doingShadows);

#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif
	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif
}

size_t TrainView::wrapIndex(int idx, size_t count) const
{
	if (count == 0)
		return 0;
	int mod = idx % static_cast<int>(count);
	if (mod < 0)
		mod += static_cast<int>(count);
	return static_cast<size_t>(mod);
}

Pnt3f TrainView::lerp(const Pnt3f& a, const Pnt3f& b, float t) const
{
	return a * (1.0f - t) + b * t;
}

float TrainView::lengthSquared(const Pnt3f& v) const
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

Pnt3f TrainView::normalizeVector(const Pnt3f& v) const
{
	Pnt3f copy = v;
	copy.normalize();
	return copy;
}

float TrainView::distanceBetween(const Pnt3f& a, const Pnt3f& b) const
{
	return std::sqrt(lengthSquared(b - a));
}

void TrainView::cardinalWeights(float t, float weights[4]) const
{
	const float t2 = t * t;
	const float t3 = t2 * t;
	weights[0] = -0.5f * t3 + t2 - 0.5f * t;
	weights[1] = 1.5f * t3 - 2.5f * t2 + 1.0f;
	weights[2] = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
	weights[3] = 0.5f * t3 - 0.5f * t2;
}

void TrainView::cardinalDerivatives(float t, float deriv[4]) const
{
	const float t2 = t * t;
	deriv[0] = -1.5f * t2 + 2.0f * t - 0.5f;
	deriv[1] = 4.5f * t2 - 5.0f * t;
	deriv[2] = -4.5f * t2 + 4.0f * t + 0.5f;
	deriv[3] = 1.5f * t2 - t;
}

void TrainView::bsplineWeights(float t, float weights[4]) const
{
	const float t2 = t * t;
	const float t3 = t2 * t;
	weights[0] = (-t3 + 3.0f * t2 - 3.0f * t + 1.0f) / 6.0f;
	weights[1] = (3.0f * t3 - 6.0f * t2 + 4.0f) / 6.0f;
	weights[2] = (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f) / 6.0f;
	weights[3] = t3 / 6.0f;
}

void TrainView::bsplineDerivatives(float t, float deriv[4]) const
{
	const float t2 = t * t;
	deriv[0] = (-3.0f * t2 + 6.0f * t - 3.0f) / 6.0f;
	deriv[1] = (9.0f * t2 - 12.0f * t) / 6.0f;
	deriv[2] = (-9.0f * t2 + 6.0f * t + 3.0f) / 6.0f;
	deriv[3] = (3.0f * t2) / 6.0f;
}

Pnt3f TrainView::railOffset(const Pnt3f& dir, const Pnt3f& up, float halfWidth) const
{
	Pnt3f side = dir * up;
	if (lengthSquared(side) < 1e-6f) {
		const Pnt3f fallbackUp(0.0f, 1.0f, 0.0f);
		side = dir * fallbackUp;
		if (lengthSquared(side) < 1e-6f) {
			const Pnt3f fallbackUp2(0.0f, 0.0f, 1.0f);
			side = dir * fallbackUp2;
		}
	}
	side.normalize();
	return side * halfWidth;
}

Pnt3f TrainView::orientPoint(const Pnt3f& origin, const Pnt3f& right, const Pnt3f& up, const Pnt3f& forward, float x, float y, float z) const
{
	return origin + right * x + up * y + forward * z;
}

void TrainView::drawTrack(bool doingShadows)
{
	if (!m_pTrack || m_pTrack->points.size() < 2)
		return;

	const int splineChoice = currentSplineChoice();
	const size_t pointCount = m_pTrack->points.size();
	const int stepsPerSegment = (DIVIDE_LINE < 1.0f) ? 1 : static_cast<int>(DIVIDE_LINE);
	const float invSteps = 1.0f / static_cast<float>(stepsPerSegment);
	const float trackHalfWidth = 2.5f;

	if (!doingShadows)
		glColor3ub(32, 32, 64);
	glLineWidth(3.0f);

	for (size_t segIdx = 0; segIdx < pointCount; ++segIdx) {
		const float baseU = static_cast<float>(segIdx);
		for (int step = 0; step < stepsPerSegment; ++step) {
			const float u0 = baseU + step * invSteps;
			const float u1 = baseU + (step + 1) * invSteps;
			const SplineSample sample0 = sampleSpline(u0, splineChoice);
			const SplineSample sample1 = sampleSpline(u1, splineChoice);
			Pnt3f dir = sample1.pos - sample0.pos;
			if (lengthSquared(dir) < 1e-6f)
				continue;
			dir.normalize();
			const Pnt3f offset0 = railOffset(dir, sample0.orient, trackHalfWidth);
			const Pnt3f offset1 = railOffset(dir, sample1.orient, trackHalfWidth);

			glBegin(GL_LINES);
			glVertex3f(sample0.pos.x + offset0.x, sample0.pos.y + offset0.y, sample0.pos.z + offset0.z);
			glVertex3f(sample1.pos.x + offset1.x, sample1.pos.y + offset1.y, sample1.pos.z + offset1.z);
			glVertex3f(sample0.pos.x - offset0.x, sample0.pos.y - offset0.y, sample0.pos.z - offset0.z);
			glVertex3f(sample1.pos.x - offset1.x, sample1.pos.y - offset1.y, sample1.pos.z - offset1.z);
			glEnd();
		}
	}
}

void TrainView::drawSleepers(bool doingShadows)
{
	if (!m_pTrack || m_pTrack->points.size() < 2)
		return;

	const int splineChoice = currentSplineChoice();
	const size_t pointCount = m_pTrack->points.size();
	const int stepsPerSegment = (DIVIDE_LINE < 1.0f) ? 1 : static_cast<int>(DIVIDE_LINE);
	const float invSteps = 1.0f / static_cast<float>(stepsPerSegment);
	const float sleeperSpacing = 8.0f;
	const float sleeperHalfWidth = 3.0f;
	const float sleeperHalfLength = 2.0f;
	const float halfStep = invSteps * 0.5f;
	const float tangentSampleOffset = (halfStep > 0.01f) ? halfStep : 0.01f;

	sleeperSamples.clear();

	float distSinceLastSleeper = 0.0f;

	for (size_t segIdx = 0; segIdx < pointCount; ++segIdx) {
		const float baseU = static_cast<float>(segIdx);
		for (int step = 0; step < stepsPerSegment; ++step) {
			const float u0 = baseU + step * invSteps;
			const float u1 = baseU + (step + 1) * invSteps;
			SplineSample startSample = sampleSpline(u0, splineChoice);
			const SplineSample endSample = sampleSpline(u1, splineChoice);
			float segmentLength = distanceBetween(startSample.pos, endSample.pos);
			if (segmentLength < 1e-5f)
				continue;

			float currentU = u0;
			while (distSinceLastSleeper + segmentLength >= sleeperSpacing) {
				const float needed = sleeperSpacing - distSinceLastSleeper;
				float ratio = needed / segmentLength;
				if (ratio < 0.0f) ratio = 0.0f;
				if (ratio > 1.0f) ratio = 1.0f;
				const float sleeperU = currentU + (u1 - currentU) * ratio;
				const SplineSample sleeperSample = sampleSpline(sleeperU, splineChoice);
				SplineSample aheadSample = sampleSpline(sleeperU + tangentSampleOffset, splineChoice);
				Pnt3f tangent = aheadSample.pos - sleeperSample.pos;
				if (lengthSquared(tangent) < 1e-6f) {
					SplineSample behindSample = sampleSpline(sleeperU - tangentSampleOffset, splineChoice);
					tangent = sleeperSample.pos - behindSample.pos;
				}
				if (lengthSquared(tangent) < 1e-6f)
					tangent = endSample.pos - startSample.pos;
				tangent.normalize();

				const Pnt3f right = railOffset(tangent, sleeperSample.orient, sleeperHalfWidth);
				const Pnt3f forward = tangent * sleeperHalfLength;
				const Pnt3f center = sleeperSample.pos;

				SplineSample storedSleeper = sleeperSample;
				storedSleeper.tangent = tangent;
				storedSleeper.param = sleeperU;
				sleeperSamples.push_back(storedSleeper);

				glBegin(GL_QUADS);
				if (!doingShadows)
					glColor3ub(255, 255, 255);
				Pnt3f normal = normalizeVector(sleeperSample.orient);
				glNormal3f(normal.x, normal.y, normal.z);
				const Pnt3f v0 = center - forward - right;
				const Pnt3f v1 = center - forward + right;
				const Pnt3f v2 = center + forward + right;
				const Pnt3f v3 = center + forward - right;
				glVertex3f(v0.x, v0.y, v0.z);
				glVertex3f(v1.x, v1.y, v1.z);
				glVertex3f(v2.x, v2.y, v2.z);
				glVertex3f(v3.x, v3.y, v3.z);
				glEnd();

				distSinceLastSleeper = 0.0f;
				currentU = sleeperU;
				startSample = sleeperSample;
				segmentLength = distanceBetween(startSample.pos, endSample.pos);
				if (segmentLength < 1e-5f)
					break;
			}

			distSinceLastSleeper += segmentLength;
		}
	}

	if (sleeperSamples.empty()) {
		SplineSample fallback = sampleSpline(0.0f, splineChoice);
		SplineSample ahead = sampleSpline(0.1f, splineChoice);
		Pnt3f tangent = ahead.pos - fallback.pos;
		if (lengthSquared(tangent) < 1e-6f)
			tangent = Pnt3f(0.0f, 0.0f, 1.0f);
		tangent.normalize();
		fallback.tangent = tangent;
		fallback.param = 0.0f;
		sleeperSamples.push_back(fallback);
	}
}

void TrainView::drawTrain(bool doingShadows)
{
	const size_t count = sleeperSamples.size();
	if (count == 0)
		return;

	const float total = static_cast<float>(count);
	float u = (m_pTrack) ? m_pTrack->trainU : 0.0f;
	if (total > 0.0f) {
		u = std::fmod(u, total);
		if (u < 0.0f)
			u += total;
	} else {
		u = 0.0f;
	}

	size_t idx0 = static_cast<size_t>(std::floor(u));
	if (idx0 >= count)
		idx0 = count - 1;
	size_t idx1 = (idx0 + 1) % count;
	const float interp = (count > 1) ? (u - static_cast<float>(idx0)) : 0.0f;
	const SplineSample& s0 = sleeperSamples[idx0];
	const SplineSample& s1 = sleeperSamples[idx1];

	const Pnt3f pos = (count > 1) ? lerp(s0.pos, s1.pos, interp) : s0.pos;
	Pnt3f up = (count > 1) ? normalizeVector(lerp(s0.orient, s1.orient, interp)) : normalizeVector(s0.orient);
	Pnt3f forward = (count > 1) ? normalizeVector(lerp(s0.tangent, s1.tangent, interp)) : normalizeVector(s0.tangent);

	if (lengthSquared(up) < 1e-6f)
		up = Pnt3f(0.0f, 1.0f, 0.0f);
	if (lengthSquared(forward) < 1e-6f)
		forward = Pnt3f(0.0f, 0.0f, 1.0f);

	forward.normalize();
	up.normalize();

	Pnt3f right = forward * up;
	if (lengthSquared(right) < 1e-6f) {
		up = Pnt3f(0.0f, 1.0f, 0.0f);
		right = forward * up;
		if (lengthSquared(right) < 1e-6f) {
			up = Pnt3f(1.0f, 0.0f, 0.0f);
			right = forward * up;
		}
	}
	right.normalize();
	up = right * forward;
	up.normalize();

	const float halfSize = 3.0f;
	const Pnt3f cubeCenter = pos + up * halfSize; // lift cube so it rides on top of the sleeper
	const Pnt3f c000 = orientPoint(cubeCenter, right, up, forward, -halfSize, -halfSize, -halfSize);
	const Pnt3f c100 = orientPoint(cubeCenter, right, up, forward, halfSize, -halfSize, -halfSize);
	const Pnt3f c110 = orientPoint(cubeCenter, right, up, forward, halfSize, halfSize, -halfSize);
	const Pnt3f c010 = orientPoint(cubeCenter, right, up, forward, -halfSize, halfSize, -halfSize);
	const Pnt3f c001 = orientPoint(cubeCenter, right, up, forward, -halfSize, -halfSize, halfSize);
	const Pnt3f c101 = orientPoint(cubeCenter, right, up, forward, halfSize, -halfSize, halfSize);
	const Pnt3f c111 = orientPoint(cubeCenter, right, up, forward, halfSize, halfSize, halfSize);
	const Pnt3f c011 = orientPoint(cubeCenter, right, up, forward, -halfSize, halfSize, halfSize);

	if (!doingShadows)
	{
		glColor3ub(255, 255, 255);
		// ensure the train has material properties so it responds to lighting
		GLfloat matAmbient[] = {0.8f, 0.8f, 0.8f, 1.0f};
		GLfloat matDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
		GLfloat matSpecular[] = {0.6f, 0.6f, 0.6f, 1.0f};
		GLfloat matShininess[] = {32.0f};
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);
	}

	glBegin(GL_QUADS);
	// bottom face
	glNormal3f(-up.x, -up.y, -up.z);
	glVertex3f(c000.x, c000.y, c000.z);
	glVertex3f(c100.x, c100.y, c100.z);
	glVertex3f(c101.x, c101.y, c101.z);
	glVertex3f(c001.x, c001.y, c001.z);
	// top face
	glNormal3f(up.x, up.y, up.z);
	glVertex3f(c010.x, c010.y, c010.z);
	glVertex3f(c011.x, c011.y, c011.z);
	glVertex3f(c111.x, c111.y, c111.z);
	glVertex3f(c110.x, c110.y, c110.z);
	// left face
	glNormal3f(-right.x, -right.y, -right.z);
	glVertex3f(c000.x, c000.y, c000.z);
	glVertex3f(c001.x, c001.y, c001.z);
	glVertex3f(c011.x, c011.y, c011.z);
	glVertex3f(c010.x, c010.y, c010.z);
	// right face
	glNormal3f(right.x, right.y, right.z);
	glVertex3f(c100.x, c100.y, c100.z);
	glVertex3f(c110.x, c110.y, c110.z);
	glVertex3f(c111.x, c111.y, c111.z);
	glVertex3f(c101.x, c101.y, c101.z);
	// back face
	glNormal3f(-forward.x, -forward.y, -forward.z);
	glVertex3f(c000.x, c000.y, c000.z);
	glVertex3f(c010.x, c010.y, c010.z);
	glVertex3f(c110.x, c110.y, c110.z);
	glVertex3f(c100.x, c100.y, c100.z);
	// front face
	glNormal3f(forward.x, forward.y, forward.z);
	glVertex3f(c001.x, c001.y, c001.z);
	glVertex3f(c101.x, c101.y, c101.z);
	glVertex3f(c111.x, c111.y, c111.z);
	glVertex3f(c011.x, c011.y, c011.z);
	glEnd();
}

int TrainView::currentSplineChoice() const
{
	return (tw && tw->splineBrowser) ? tw->splineBrowser->value() : 1;
}

TrainView::SplineSample TrainView::sampleSpline(float u, int splineChoice) const
{
	SplineSample sample{};
	if (!m_pTrack || m_pTrack->points.empty())
		return sample;
	sample.tangent = Pnt3f(0.0f, 0.0f, 0.0f);

	const size_t pointCount = m_pTrack->points.size();
	const float totalSpan = static_cast<float>(pointCount);
	float wrappedU = std::fmod(u, totalSpan);
	if (wrappedU < 0.0f)
		wrappedU += totalSpan;
	int baseSeg = static_cast<int>(std::floor(wrappedU));
	float localT = wrappedU - static_cast<float>(baseSeg);
	baseSeg = static_cast<int>(wrapIndex(baseSeg, pointCount));

	const ControlPoint& cp0 = m_pTrack->points[wrapIndex(baseSeg, pointCount)];
	const ControlPoint& cp1 = m_pTrack->points[wrapIndex(baseSeg + 1, pointCount)];

	sample.pos = lerp(cp0.pos, cp1.pos, localT);
	sample.orient = normalizeVector(lerp(cp0.orient, cp1.orient, localT));
	sample.tangent = cp1.pos - cp0.pos;

	if (pointCount >= 4) {
		const ControlPoint& cm1 = m_pTrack->points[wrapIndex(baseSeg - 1, pointCount)];
		const ControlPoint& cp2 = m_pTrack->points[wrapIndex(baseSeg + 2, pointCount)];

		if (splineChoice == 2) {
			float weights[4];
			float deriv[4];
			cardinalWeights(localT, weights);
			cardinalDerivatives(localT, deriv);
			Pnt3f geomPos[4] = { cm1.pos, cp0.pos, cp1.pos, cp2.pos };
			Pnt3f geomOrient[4] = { cm1.orient, cp0.orient, cp1.orient, cp2.orient };
			sample.pos = Pnt3f(0.0f, 0.0f, 0.0f);
			sample.orient = Pnt3f(0.0f, 0.0f, 0.0f);
			sample.tangent = Pnt3f(0.0f, 0.0f, 0.0f);
			for (int i = 0; i < 4; ++i) {
				sample.pos = sample.pos + geomPos[i] * weights[i];
				sample.orient = sample.orient + geomOrient[i] * weights[i];
				sample.tangent = sample.tangent + geomPos[i] * deriv[i];
			}
			sample.orient = normalizeVector(sample.orient);
		}
		else if (splineChoice == 3) {
			float weights[4];
			float deriv[4];
			bsplineWeights(localT, weights);
			bsplineDerivatives(localT, deriv);
			Pnt3f geomPos[4] = { cm1.pos, cp0.pos, cp1.pos, cp2.pos };
			Pnt3f geomOrient[4] = { cm1.orient, cp0.orient, cp1.orient, cp2.orient };
			sample.pos = Pnt3f(0.0f, 0.0f, 0.0f);
			sample.orient = Pnt3f(0.0f, 0.0f, 0.0f);
			sample.tangent = Pnt3f(0.0f, 0.0f, 0.0f);
			for (int i = 0; i < 4; ++i) {
				sample.pos = sample.pos + geomPos[i] * weights[i];
				sample.orient = sample.orient + geomOrient[i] * weights[i];
				sample.tangent = sample.tangent + geomPos[i] * deriv[i];
			}
			sample.orient = normalizeVector(sample.orient);
		}
	}

	sample.param = static_cast<float>(baseSeg) + localT;

	return sample;
}

size_t TrainView::sleeperCount() const
{
	return sleeperSamples.size();
}



// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();		

	// where is the mouse?
	int mx = Fl::event_x(); 
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}

void TrainView::drawCastle() {
	this->shader = new Shader("./shaders/simple.vert", nullptr, nullptr, nullptr, "./shaders/simple.frag");
	this->common_matrices = new UBO();


	this->common_matrices->size = 2 * sizeof(glm::mat4);
	glGenBuffers(1, &this->common_matrices->ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, this->common_matrices->ubo);
	glBufferData(GL_UNIFORM_BUFFER, this->common_matrices->size, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	GLfloat vertices[] = {
		-0.5f ,0.0f , -0.5f,
		-0.5f ,0.0f , 0.5f ,
		0.5f ,0.0f , 0.5f ,
		0.5f ,0.0f , -0.5f };

	GLfloat normal[] = {
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f };

	GLfloat texture_coordinate[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f };

	GLuint element[] = {
		0, 1, 2,
		0, 2, 3, };

	this->plane = new VAO;
	this->plane->element_amount = sizeof(element) / sizeof(GLuint);
	glGenVertexArrays(1, &this->plane->vao);
	glGenBuffers(3, this->plane->vbo);
	glGenBuffers(1, &this->plane->ebo);

	glBindVertexArray(this->plane->vao);

	// Position attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	// Texture Coordinate attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	// Element attribute
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);
	
	this->texture = new Texture2D("./images/church.png");
}

void TrainView::drawColoredCastle()  {
	this->shader = new Shader("./shaders/colored.vert", nullptr, nullptr, nullptr, "./shaders/colored.frag");
	this->common_matrices = new UBO();

	this->common_matrices->size = 2 * sizeof(glm::mat4);
	glGenBuffers(1, &this->common_matrices->ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, this->common_matrices->ubo);
	glBufferData(GL_UNIFORM_BUFFER, this->common_matrices->size, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	
	GLfloat vertices[] = {
		-0.5f, 0.0f, -0.5f,  
		-0.5f, 0.0f,  0.5f,   
		0.366f, 0.0f, 0.0f,  
	};

	GLfloat normal[] = {
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,};

	GLfloat texture_coordinate[] = {
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.5f, 1.0f, };

	GLuint element[] = {
		0, 1, 2 };

	GLfloat color[] = {
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f
	};


	this->plane = new VAO;
	this->plane->element_amount = sizeof(element) / sizeof(GLuint);
	glGenVertexArrays(1, &this->plane->vao);
	glGenBuffers(4, this->plane->vbo);
	glGenBuffers(1, &this->plane->ebo);

	glBindVertexArray(this->plane->vao);

	// Position attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Normal attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normal), normal, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	// Texture Coordinate attribute
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coordinate), texture_coordinate, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	// color attribute
		
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(3);

	// Element attribute
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);

	// Unbind VAO
	glBindVertexArray(0);
	this->texture = new Texture2D("./images/church.png");
}

void TrainView::drawWave(float time) {
	
	this->shader = new Shader("./shaders/temp.vert", nullptr, nullptr, nullptr, "./shaders/temp.frag");
	this->common_matrices = new UBO();

	this->common_matrices->size = 2 * sizeof(glm::mat4);
	glGenBuffers(1, &this->common_matrices->ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, this->common_matrices->ubo);
	glBufferData(GL_UNIFORM_BUFFER, this->common_matrices->size, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// ====== 參數設定 ======
	const int N = 100;           // 網格細緻度（越大越平滑）
	const float SIZE = 10.0f;    // 整體平面大小（10x10）

	for (auto& w : waves)
		w.frequency = 2.0f * M_PI / w.wavelength;

	// ====== 產生頂點資料 ======
	float half = SIZE / 2.0f;

	for (int j = 0; j <= N; ++j) {
		for (int i = 0; i <= N; ++i) {
			std::time_t now = std::time(nullptr);           
			float currentTime =  static_cast<float>(std::difftime(now, startTime));
			float x = (float)i / N * SIZE - SIZE * 0.5f;
			float z = (float)j / N * SIZE - SIZE * 0.5f;

			// ====== 計算動態波紋高度 ======
			float h = 0.0f;
			for (const auto& w : waves) {
				float phase = glm::dot(w.direction, glm::vec2(x, z)) * w.frequency + w.speed * currentTime;
				h += w.amplitude * sin(phase);
			}

			// 頂點位置
			vertices.push_back(x);
			vertices.push_back(h);
			vertices.push_back(z);

			// ====== 法線計算 (簡化：利用微分近似) ======
			float eps = 0.1f;
			float hL = 0.0f, hR = 0.0f, hD = 0.0f, hU = 0.0f;

			for (const auto& w : waves) {
				float phaseL = glm::dot(w.direction, glm::vec2(x - eps, z)) * w.frequency + w.speed * time;
				float phaseR = glm::dot(w.direction, glm::vec2(x + eps, z)) * w.frequency + w.speed * time;
				float phaseD = glm::dot(w.direction, glm::vec2(x, z - eps)) * w.frequency + w.speed * time;
				float phaseU = glm::dot(w.direction, glm::vec2(x, z + eps)) * w.frequency + w.speed * time;
				hL += w.amplitude * sin(phaseL);
				hR += w.amplitude * sin(phaseR);
				hD += w.amplitude * sin(phaseD);
				hU += w.amplitude * sin(phaseU);
			}

			glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * eps, hD - hU));
			normals.push_back(normal.x);
			normals.push_back(normal.y);
			normals.push_back(normal.z);

			// 貼圖座標
			texcoords.push_back((float)i / N);
			texcoords.push_back((float)j / N);

			// 顏色 — 隨高度微微改變
			float blue = 0.6f + h * 3.0f;
			float green = 0.5f + h * 2.0f;
			colors.push_back(0.0f);
			colors.push_back(glm::clamp(green, 0.0f, 1.0f));
			colors.push_back(glm::clamp(blue, 0.0f, 1.0f));
		}
	}

	// ====== 產生索引資料 ======
	for (int j = 0; j < N; ++j) {
		for (int i = 0; i < N; ++i) {
			GLuint topLeft = j * (N + 1) + i;
			GLuint topRight = topLeft + 1;
			GLuint bottomLeft = (j + 1) * (N + 1) + i;
			GLuint bottomRight = bottomLeft + 1;

			elements.push_back(topLeft);
			elements.push_back(bottomLeft);
			elements.push_back(topRight);

			elements.push_back(topRight);
			elements.push_back(bottomLeft);
			elements.push_back(bottomRight);
		}
	}

	// ====== 建立 VAO / VBO ======
	this->plane = new VAO();
	this->plane->element_amount = elements.size();

	glGenVertexArrays(1, &this->plane->vao);
	glGenBuffers(4, this->plane->vbo);
	glGenBuffers(1, &this->plane->ebo);

	glBindVertexArray(this->plane->vao);

	// 頂點位置
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
		vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// 法線
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
		normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	// 貼圖座標
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat),
		texcoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	// 顏色
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat),
		colors.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(3);

	// 索引
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->plane->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(GLuint),
		elements.data(), GL_STATIC_DRAW);

	// 解除綁定
	glBindVertexArray(0);

}

void TrainView::updateWater(float time, int N, float size) {
	for (int j = 0; j <= N; ++j) {
		for (int i = 0; i <= N; ++i) {
			int index = (j * (N + 1) + i) * 3;
			float x = (float)i / N * size - size * 0.5f;
			float z = (float)j / N * size - size * 0.5f;

			// === 動態波紋高度 ===
			float h = 0.0f;
			for (const auto& w : waves) {
				float phase = glm::dot(w.direction, glm::vec2(x, z)) * w.frequency + w.speed * time;
				h += w.amplitude * sin(phase);
			}
			vertices[index + 1] = h;

			// === 顏色根據高度調整 ===
			float blue = 0.6f + h * 3.0f;
			float green = 0.5f + h * 2.0f;
			colors[index + 0] = 0.0f;
			colors[index + 1] = glm::clamp(green, 0.0f, 1.0f);
			colors[index + 2] = glm::clamp(blue, 0.0f, 1.0f);

			// === 簡易法線（利用微分近似） ===
			float eps = 0.1f;
			float hL = 0.0f, hR = 0.0f, hD = 0.0f, hU = 0.0f;
			for (const auto& w : waves) {
				hL += w.amplitude * sin(glm::dot(w.direction, glm::vec2(x - eps, z)) * w.frequency + w.speed * time);
				hR += w.amplitude * sin(glm::dot(w.direction, glm::vec2(x + eps, z)) * w.frequency + w.speed * time);
				hD += w.amplitude * sin(glm::dot(w.direction, glm::vec2(x, z - eps)) * w.frequency + w.speed * time);
				hU += w.amplitude * sin(glm::dot(w.direction, glm::vec2(x, z + eps)) * w.frequency + w.speed * time);
			}
			glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * eps, hD - hU));
			normals[index + 0] = normal.x;
			normals[index + 1] = normal.y;
			normals[index + 2] = normal.z;
		}
	}

	// === 更新到 GPU ===
	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, normals.size() * sizeof(float), normals.data());

	glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[3]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
}