/************************************************************************
     File:        TrainView.H

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

#pragma once
// Preclarify for preventing the compiler error
class TrainWindow;
class CTrack;


//#######################################################################
// TODO
// You might change the TrainView in order to add different objects to
// be drawn, or adjust the lighting, or ...
//#######################################################################


// The TrainView also needs to handle its user interface events.
// since this is an FLTk Window, we need to include FlTk Headers
// the pragmas turn off the warnings from FlTk - our code should have no warnings,
// but their code doesn't meet that standard
#pragma warning(push)
#pragma warning(disable:4312)
#pragma warning(disable:4311)
#include <Fl/Fl_Gl_Window.h>
#pragma warning(pop)

// this uses the old ArcBall Code
#include <vector>

#include "Utilities/ArcBallCam.H"
#include "../src/Utilities/Pnt3f.h"
#include "RenderUtilities/BufferObject.h";
#include "RenderUtilities/Shader.h";
#include "RenderUtilities/Texture.h";

class TrainView : public Fl_Gl_Window
{
	public:
		Shader* currentShader = nullptr;
		Shader* normalCastle = nullptr;
		Shader* coloredCastle = nullptr;
		Shader* wave = nullptr;
		Texture2D* heightMap = nullptr;
		Texture2D* texture = nullptr;
		VAO* plane = nullptr;
		UBO* common_matrices = nullptr;
		void setUBO();

		float getTime();
		void drawSleepers(bool);
		// note that we keep the "standard widget" constructor arguments
		TrainView(int x, int y, int w, int h, const char* l = 0);

		// overrides of important window things
		virtual int handle(int);
		virtual void draw();

		// all of the actual drawing happens in this routine
		// it has to be encapsulated, since we draw differently if
		// we're drawing shadows (no colors, for example)
		void drawStuff(bool doingShadows=false);

		void drawTrack(bool doingShadows);

		void drawTrain(bool doingShadows);

		// setup the projection - assuming that the projection stack has been
		// cleared for you
		void setProjection();

		// Reset the Arc ball control
		void resetArcball();

		// pick a point (for when the mouse goes down)
		void doPick();

	public:
		float DIVIDE_LINE = 1000.0f;

		ArcBallCam		arcball;			// keep an ArcBall for the UI
		int				selectedCube;  // simple - just remember which cube is selected

		TrainWindow*	tw;			// The parent of this display window
		CTrack*		m_pTrack; 	// The track of the entire scene

		size_t sleeperCount() const;

	public:
		struct SplineSample {
			Pnt3f pos;
			Pnt3f orient;
			Pnt3f tangent;
			float param = 0.0f;
		};

		struct Wave {
			glm::vec2 direction;
			float wavelength;
			float amplitude;
			float speed;
			float frequency;
		};

		void updateWater(float time, int N, float size);
		void setCastle();
		void setColoredCastle();
		void setWave(float);
		void useShader(int shaderChoice);
		int currentSplineChoice() const;
		SplineSample sampleSpline(float u, int splineChoice) const;
		size_t wrapIndex(int idx, size_t count) const;
		Pnt3f lerp(const Pnt3f& a, const Pnt3f& b, float t) const;
		float lengthSquared(const Pnt3f& v) const;
		Pnt3f normalizeVector(const Pnt3f& v) const;
		float distanceBetween(const Pnt3f& a, const Pnt3f& b) const;
		Pnt3f railOffset(const Pnt3f& dir, const Pnt3f& up, float halfWidth) const;
		Pnt3f orientPoint(const Pnt3f& origin, const Pnt3f& right, const Pnt3f& up, const Pnt3f& forward, float x, float y, float z) const;
		void cardinalWeights(float t, float weights[4]) const;
		void cardinalDerivatives(float t, float deriv[4]) const;
		void bsplineWeights(float t, float weights[4]) const;
		void bsplineDerivatives(float t, float deriv[4]) const;
		std::vector<SplineSample> sleeperSamples;
		static float startTime;
	private:
		std::vector<Wave> waves = {
			{{1.0f, 0.0f}, 2.0f, 0.10f, 1.0f},
			{{0.7f, 0.7f}, 3.0f, 0.05f, 0.8f},
			{{-0.6f, 0.8f}, 1.5f, 0.07f, 1.2f}
		};
		std::vector<GLfloat> vertices;
		std::vector<GLfloat> normals;
		std::vector<GLfloat> texcoords;
		std::vector<GLfloat> colors;
		std::vector<GLuint> elements;
		
};