/************************************************************************
     File:        TrainWindow.H

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						this class defines the window in which the project 
						runs - its the outer windows that contain all of 
						the widgets, including the "TrainView" which has the 
						actual OpenGL window in which the train is drawn

						You might want to modify this class to add new widgets
						for controlling	your train

						This takes care of lots of things - including installing 
						itself into the FlTk "idle" loop so that we get periodic 
						updates (if we're running the train).


     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <FL/fl.h>
#include <FL/Fl_Box.h>

// for using the real time clock
#include <time.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>

#include "TrainWindow.H"
#include "TrainView.H"
#include "CallBacks.H"



//************************************************************************
//
// * Constructor
//========================================================================
TrainWindow::
TrainWindow(const int x, const int y) 
	: Fl_Double_Window(x,y,800,600,"Train and Roller Coaster")
//========================================================================
{
	// make all of the widgets
	begin();	// add to this widget
	{
		int pty=5;			// where the last widgets were drawn

		trainView = new TrainView(5,5,590,590);
		trainView->tw = this;
		trainView->m_pTrack = &m_Track;
		this->resizable(trainView);

		// to make resizing work better, put all the widgets in a group
		widgets = new Fl_Group(600,5,190,590);
		widgets->begin();

		runButton = new Fl_Button(605,pty,60,20,"Run");
		togglify(runButton);

		Fl_Button* fb = new Fl_Button(700,pty,25,20,"@>>");
		fb->callback((Fl_Callback*)forwCB,this);
		Fl_Button* rb = new Fl_Button(670,pty,25,20,"@<<");
		rb->callback((Fl_Callback*)backCB,this);
		
		arcLength = new Fl_Button(730,pty,65,20,"ArcLength");
		togglify(arcLength,1);
  
		pty+=25;
		speed = new Fl_Value_Slider(655,pty,140,20,"speed");
		speed->range(0,10);
		speed->value(2);
		speed->align(FL_ALIGN_LEFT);
		speed->type(FL_HORIZONTAL);

		pty += 30;

		// camera buttons - in a radio button group
		Fl_Group* camGroup = new Fl_Group(600,pty,195,20);
		camGroup->begin();
		worldCam = new Fl_Button(605, pty, 60, 20, "World");
        worldCam->type(FL_RADIO_BUTTON);		// radio button
        worldCam->value(1);			// turned on
        worldCam->selection_color((Fl_Color)3); // yellow when pressed
		worldCam->callback((Fl_Callback*)damageCB,this);
		trainCam = new Fl_Button(670, pty, 60, 20, "Train");
        trainCam->type(FL_RADIO_BUTTON);
        trainCam->value(0);
        trainCam->selection_color((Fl_Color)3);
		trainCam->callback((Fl_Callback*)damageCB,this);
		topCam = new Fl_Button(735, pty, 60, 20, "Top");
        topCam->type(FL_RADIO_BUTTON);
        topCam->value(0);
        topCam->selection_color((Fl_Color)3);
		topCam->callback((Fl_Callback*)damageCB,this);
		camGroup->end();

		pty += 30;

		// browser to select spline types
		// TODO: make sure these choices are the same as what the code supports
		splineBrowser = new Fl_Browser(605,pty,120,75,"Spline Type");
		splineBrowser->type(2);		// select
		splineBrowser->callback((Fl_Callback*)damageCB,this);
		splineBrowser->add("Linear");
		splineBrowser->add("Cardinal Cubic");
		splineBrowser->add("Cubic B-Spline");
		splineBrowser->select(2);

		pty += 110;

		lightBrowser = new Fl_Browser(605, pty, 120, 75, "Light Type");
		lightBrowser->type(2);
		lightBrowser->callback((Fl_Callback*)damageCB, this);
		lightBrowser->add("Normal");
		lightBrowser->add("Directional light");
		lightBrowser->add("Spot light");
		lightBrowser->select(1);

		pty += 110;

		shaderBrowser = new Fl_Browser(605, pty, 120, 75, "Image Type");
		shaderBrowser->type(2);
		shaderBrowser->callback((Fl_Callback*)damageCB, this);
		shaderBrowser->add("Regular Castle");
		shaderBrowser->add("Colored Castle");
		shaderBrowser->add("Height Map Wave");
		shaderBrowser->add("Sine Wave");
		shaderBrowser->select(1);
		
		pty += 110;

		// add and delete points
		Fl_Button* ap = new Fl_Button(605,pty,80,20,"Add Point");
		ap->callback((Fl_Callback*)addPointCB,this);
		Fl_Button* dp = new Fl_Button(690,pty,80,20,"Delete Point");
		dp->callback((Fl_Callback*)deletePointCB,this);

		pty += 25;
		// reset the points
		resetButton = new Fl_Button(735,pty,60,20,"Reset");
		resetButton->callback((Fl_Callback*)resetCB,this);
		Fl_Button* loadb = new Fl_Button(605,pty,60,20,"Load");
		loadb->callback((Fl_Callback*) loadCB, this);
		Fl_Button* saveb = new Fl_Button(670,pty,60,20,"Save");
		saveb->callback((Fl_Callback*) saveCB, this);

		pty += 25;
		// roll the points
		Fl_Button* rx = new Fl_Button(605,pty,30,20,"R+X");
		rx->callback((Fl_Callback*)rpxCB,this);
		Fl_Button* rxp = new Fl_Button(635,pty,30,20,"R-X");
		rxp->callback((Fl_Callback*)rmxCB,this);
		Fl_Button* rz = new Fl_Button(670,pty,30,20,"R+Z");
		rz->callback((Fl_Callback*)rpzCB,this);
		Fl_Button* rzp = new Fl_Button(700,pty,30,20,"R-Z");
		rzp->callback((Fl_Callback*)rmzCB,this);

		pty+=30;

		// TODO: add widgets for all of your fancier features here
#ifdef EXAMPLE_SOLUTION
		makeExampleWidgets(this,pty);
#endif

		// we need to make a little phantom widget to have things resize correctly
		Fl_Box* resizebox = new Fl_Box(600,595,200,5);
		widgets->resizable(resizebox);

		widgets->end();
	}
	end();	// done adding to this widget

	// set up callback on idle
	Fl::add_idle((void (*)(void*))runButtonCB,this);
}

//************************************************************************
//
// * handy utility to make a button into a toggle
//========================================================================
void TrainWindow::
togglify(Fl_Button* b, int val)
//========================================================================
{
	b->type(FL_TOGGLE_BUTTON);		// toggle
	b->value(val);		// turned off
	b->selection_color((Fl_Color)3); // yellow when pressed	
	b->callback((Fl_Callback*)damageCB,this);
}

//************************************************************************
//
// *
//========================================================================
void TrainWindow::
damageMe()
//========================================================================
{
	if (trainView->selectedCube >= ((int)m_Track.points.size()))
		trainView->selectedCube = 0;
	trainView->damage(1);
}

//************************************************************************
//
// * This will get called (approximately) 30 times per second
//   if the run button is pressed
//========================================================================
void TrainWindow::
advanceTrain(float dir)
//========================================================================
{
	const size_t segmentCount = m_Track.points.size();
	if (segmentCount < 2)
		return;

	const float direction = (dir >= 0.0f) ? 1.0f : -1.0f;
	const float absDir = std::fabs(dir);
	const float magnitude = (absDir > 0.0f) ? absDir : 0.0f;
	const float sliderSpeed = static_cast<float>(speed->value());
	const float stepScale = 0.3f;
	const float expectedUpdatesPerSecond = 30.0f;
	const float baseArcSpeed = stepScale * expectedUpdatesPerSecond;
	const float segmentDurationSeconds = 2.0f;
	const float minSliderValue = 0.05f;

	// compute elapsed time (seconds) between calls
	const auto now = std::chrono::steady_clock::now();
	if (lastAdvanceTick == std::chrono::steady_clock::time_point{})
		lastAdvanceTick = now;
	float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastAdvanceTick).count();
	if (!runButton || !runButton->value()) {
		// manual stepping – treat each call as a single frame
		dt = 1.0f / expectedUpdatesPerSecond;
	} else {
		if (dt <= 0.0f)
			dt = 1.0f / expectedUpdatesPerSecond;
		else if (dt > 0.25f)
			dt = 0.25f; // clamp so pauses don't fling the train
	}
	lastAdvanceTick = now;
	dt *= magnitude;

	if (!trainView)
		return;

	const size_t sleeperTotal = trainView->sleeperCount();
	const float maxU = (sleeperTotal > 0)
		? static_cast<float>(sleeperTotal)
		: static_cast<float>(segmentCount);
	if (maxU <= 0.0f)
		return;

	refreshSegmentMetrics();
	if (segmentSampleCounts.empty())
		return;

	float delta = 0.0f;
	if (arcLength && arcLength->value()) {
		int segmentIdx = segmentIndexFromTrainU();
		if (segmentIdx < 0)
			segmentIdx = 0;
		else if (segmentIdx >= static_cast<int>(segmentSampleCounts.size()))
			segmentIdx = static_cast<int>(segmentSampleCounts.size()) - 1;
		float samplesInSegment = segmentSampleCounts[segmentIdx];
		float lengthInSegment = segmentArcLengths[segmentIdx];
		if (samplesInSegment <= 1e-4f)
			samplesInSegment = (averageSamplesPerSegment > 1e-4f) ? averageSamplesPerSegment : 1.0f;
		if (lengthInSegment <= 1e-4f)
			lengthInSegment = (averageArcLengthPerSegment > 1e-4f) ? averageArcLengthPerSegment : 1.0f;
		float denomSamples = (samplesInSegment > 1.0f) ? samplesInSegment : 1.0f;
		float denomAverageSamples = (averageSamplesPerSegment > 1.0f) ? averageSamplesPerSegment : 1.0f;
		float lengthPerSample = lengthInSegment / denomSamples;
		float averageLengthPerSample = averageArcLengthPerSegment / denomAverageSamples;
		if (lengthPerSample <= 1e-4f)
			lengthPerSample = averageLengthPerSample;
		if (averageLengthPerSample <= 1e-4f)
			averageLengthPerSample = 1.0f;
		float effectiveSlider = (sliderSpeed > minSliderValue) ? sliderSpeed : minSliderValue;
		float physicalSpeed = effectiveSlider * baseArcSpeed * averageLengthPerSample;
		float safeLengthPerSample = (lengthPerSample > 1e-4f) ? lengthPerSample : 1e-4f;
		float unitsPerSecond = physicalSpeed / safeLengthPerSample;
		delta = direction * unitsPerSecond * dt;
	} else {
		// fixed time per control-point segment
		int segmentIdx = segmentIndexFromTrainU();
		if (segmentIdx < 0)
			segmentIdx = 0;
		else if (segmentIdx >= static_cast<int>(segmentSampleCounts.size()))
			segmentIdx = static_cast<int>(segmentSampleCounts.size()) - 1;
		float samplesInSegment = segmentSampleCounts[segmentIdx];
		if (samplesInSegment <= 1e-4f)
			samplesInSegment = averageSamplesPerSegment;
		float duration = segmentDurationSeconds;
		float speedFactor = (sliderSpeed > minSliderValue) ? sliderSpeed : minSliderValue;
		duration /= speedFactor; // faster slider -> shorter duration
		if (duration <= 1e-4f)
			duration = segmentDurationSeconds;
		float unitsPerSecond = samplesInSegment / duration;
		delta = direction * unitsPerSecond * dt;
	}

	m_Track.trainU += delta;
	while (m_Track.trainU >= maxU)
		m_Track.trainU -= maxU;
	while (m_Track.trainU < 0.0f)
		m_Track.trainU += maxU;

	if (trainView)
		trainView->damage(1);
}

void TrainWindow::refreshSegmentMetrics()
{
	const size_t segmentCount = m_Track.points.size();
	segmentSampleCounts.assign(segmentCount, 0.0f);
	segmentArcLengths.assign(segmentCount, 0.0f);
	averageSamplesPerSegment = 1.0f;
	averageArcLengthPerSegment = 1.0f;
	cachedSleeperSampleCount = (trainView) ? trainView->sleeperCount() : 0;
	cachedSplineChoice = (trainView) ? trainView->currentSplineChoice() : -1;

	if (segmentCount == 0 || !trainView)
		return;

	const auto& samples = trainView->sleeperSamples;
	const float sleeperSpacing = 8.0f;

	if (!samples.empty()) {
		const size_t sampleCount = samples.size();
		for (size_t i = 0; i < sampleCount; ++i) {
			const auto& s0 = samples[i];
			const auto& s1 = samples[(i + 1) % sampleCount];
			int segIdx = static_cast<int>(std::floor(s0.param));
			segIdx %= static_cast<int>(segmentCount);
			if (segIdx < 0)
				segIdx += static_cast<int>(segmentCount);
			float stepLength = trainView->distanceBetween(s0.pos, s1.pos);
			segmentArcLengths[segIdx] += stepLength;
			segmentSampleCounts[segIdx] += 1.0f;
		}
	} else {
		for (size_t seg = 0; seg < segmentCount; ++seg) {
			const ControlPoint& cp0 = m_Track.points[seg];
			const ControlPoint& cp1 = m_Track.points[(seg + 1) % segmentCount];
			float length = trainView->distanceBetween(cp0.pos, cp1.pos);
			segmentArcLengths[seg] = length;
			segmentSampleCounts[seg] = 1.0f;
		}
	}

	float totalSamples = std::accumulate(segmentSampleCounts.begin(), segmentSampleCounts.end(), 0.0f);
	if (totalSamples > 1e-4f)
		averageSamplesPerSegment = totalSamples / static_cast<float>(segmentCount);
	else
		averageSamplesPerSegment = 1.0f;

	float totalLength = std::accumulate(segmentArcLengths.begin(), segmentArcLengths.end(), 0.0f);
	if (totalLength > 1e-4f)
		averageArcLengthPerSegment = totalLength / static_cast<float>(segmentCount);
	else
		averageArcLengthPerSegment = sleeperSpacing;
}

int TrainWindow::segmentIndexFromTrainU() const
{
	const size_t segmentCount = m_Track.points.size();
	if (segmentCount == 0)
		return 0;
	if (!trainView)
		return 0;

	const auto& samples = trainView->sleeperSamples;
	if (samples.empty()) {
		int segIdx = static_cast<int>(std::floor(m_Track.trainU));
		segIdx %= static_cast<int>(segmentCount);
		if (segIdx < 0)
			segIdx += static_cast<int>(segmentCount);
		return segIdx;
	}

	const size_t sampleCount = samples.size();
	double wrapped = m_Track.trainU;
	if (sampleCount > 0) {
		wrapped = std::fmod(wrapped, static_cast<double>(sampleCount));
		if (wrapped < 0.0)
			wrapped += static_cast<double>(sampleCount);
	}

	const size_t idx0 = static_cast<size_t>(std::floor(wrapped)) % sampleCount;
	const size_t idx1 = (idx0 + 1) % sampleCount;
	const double localT = wrapped - std::floor(wrapped);

	float param0 = samples[idx0].param;
	float param1 = samples[idx1].param;
	if (idx1 == 0 && param1 < param0)
		param1 += static_cast<float>(segmentCount);
	if (param1 < param0)
		param1 += static_cast<float>(segmentCount);
	float interpolated = param0 + (param1 - param0) * static_cast<float>(localT);

	int segIdx = static_cast<int>(std::floor(interpolated));
	segIdx %= static_cast<int>(segmentCount);
	if (segIdx < 0)
		segIdx += static_cast<int>(segmentCount);
	return segIdx;
}