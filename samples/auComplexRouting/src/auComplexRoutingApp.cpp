#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

#include "AudioUnit.h"

using namespace ci;
using namespace ci::app;
using namespace ci::au; // <- alias for cinder::audiounit
using namespace std;

typedef struct {
	double wavePhase;
	double pulsePhase;
}
SineWaveRenderContext; // <- used in the render callback to maintain state

class auComplexRoutingApp : public AppNative {
  public:
	~auComplexRoutingApp();
	void setup();
	void mouseDown( MouseEvent event );
	void keyDown( KeyEvent event );
	void update();
	void draw();
	void drawHoverRect();
	Vec2i gridSize;
	
	FilePlayer kickPlayer;
	FilePlayer hatsPlayer;
	FilePlayer snarePlayer;
	Mixer mixer;
	Output output;
	
	GenericUnit compressor;
	GenericUnit distortion;
	GenericUnit reverb;
	GenericUnit lowPass;
	GenericUnit panner;
	
	Tap kickTap;
	Tap hatsTap;
	Tap snareTap;
	Tap renderTap;
	Tap mixdownTap;
	
	TapSampleBuffer kickBuffer;
	TapSampleBuffer hatsBuffer;
	TapSampleBuffer snareBuffer;
	TapSampleBuffer renderBuffer;
	TapSampleBuffer mixdownBuffer;
	
	SineWaveRenderContext renderContext;
	
	static OSStatus renderCallback(void * inRefCon,
								   AudioUnitRenderActionFlags * ioActionFlags,
								   const AudioTimeStamp * inTimeStamp,
								   UInt32 inBusNumber,
								   UInt32 inNumberFrames,
								   AudioBufferList * ioData);
};

void auComplexRoutingApp::setup()
{
	setWindowSize(1280, 800);
	setWindowPos(50, 50);
	
	// In this app, we'll set up a few chains of audio units, mix them down
	// with the Mixer unit, then send the resulting audio out to the speakers
	
	// We'll set up 4 seperate tracks to be mixed down
	mixer.setInputBusCount(4);
	
	// Track 0 will be a kick loop, ran through a distortion effect
	// First, we'll load the kick loop from the asset folder
	kickPlayer.setFile(loadAsset("kick.wav"));
	// Next, set up the distortion unit
	distortion = GenericUnit(kAudioUnitType_Effect, kAudioUnitSubType_Distortion);
	// now, set up the chain, connecting to bus 0 on the mixer
	kickPlayer.connectTo(distortion).connectTo(kickTap).connectTo(mixer, 0);
	
	// Track 1 will be a hihat loop, ran through a low pass filter
	hatsPlayer.setFile(loadAsset("hats.wav"));
	lowPass = GenericUnit(kAudioUnitType_Effect, kAudioUnitSubType_LowPassFilter);
	hatsPlayer.connectTo(lowPass).connectTo(hatsTap).connectTo(mixer, 1);
	// Since the hihat loop is pretty loud, we'll set the volume of bus 1 to 0.1
	mixer.setInputVolume(0.1, 1);
	
	// Track 2 will be a snare loop, ran through a reverb effect
	snarePlayer.setFile(loadAsset("snare.wav"));
	reverb = GenericUnit(kAudioUnitType_Effect, kAudioUnitSubType_MatrixReverb);
	snarePlayer.connectTo(reverb).connectTo(snareTap).connectTo(mixer, 2);
	
	// Track 3 will be sine waves rendered live from our app, ran through a 3D panning effect
	panner = GenericUnit(kAudioUnitType_Panner, kAudioUnitSubType_SphericalHeadPanner);
	// We need to tell the panner unit to get its source audio from our renderCallback.
	// We can also pass in a context, which is any arbitrary pointer. We can use this pointer
	// later to maintain state between render callbacks.
	AURenderCallbackStruct callback = {&renderCallback, &renderContext};
	panner.setRenderCallback(callback);
	panner.connectTo(renderTap).connectTo(mixer, 3);
	
	// Now, we'll send the mixdown output from the mixer through a compressor effect, and
	// then out to the speakers
	compressor = GenericUnit(kAudioUnitType_Effect, kAudioUnitSubType_DynamicsProcessor);
	mixer.connectTo(compressor).connectTo(mixdownTap).connectTo(output);
	
	// Loading saved presets for each effect. You can save the state of the effects by
	// pressing 's', which will save presets in the app's asset folder
	distortion.loadPreset(loadAsset("distortion.aupreset"));
	lowPass.loadPreset(loadAsset("lowpass.aupreset"));
	reverb.loadPreset(loadAsset("reverb.aupreset"));
	panner.loadPreset(loadAsset("panner.aupreset"));
	compressor.loadPreset(loadAsset("compressor.aupreset"));
	
	// Here we're starting all of the loops at the same time, so they stay in sync.
	kickPlayer.loop();
	hatsPlayer.loop();
	snarePlayer.loop();
	
	// Starting the output, which will pull audio through the mixer, which will pull through
	// each of the tracks we've set up
	output.start();
}

// This callback will be called whenever the panner unit needs more samples (very frequently)
// It renders a chord of sine waves, and the chord's volume is pulsed by another sine wave
// Note that Render Callbacks are run on a separate, real-time thread. Be prepared to
// handle the standard mutex, concurrency, etc issues if you're writing your own render
// callbacks which need to share data with other objects. Also note that performing heavy
// tasks like allocating memory or reading from disk are A Bad Idea.
OSStatus auComplexRoutingApp::renderCallback(void *inRefCon,
											 AudioUnitRenderActionFlags *ioActionFlags,
											 const AudioTimeStamp *inTimeStamp,
											 UInt32 inBusNumber,
											 UInt32 inNumberFrames,
											 AudioBufferList *ioData)
{
	SineWaveRenderContext * context = static_cast<SineWaveRenderContext *>(inRefCon);
	
	AudioUnitSampleType * outData = (AudioUnitSampleType *)ioData->mBuffers[0].mData;
	const double waveFrequency = 300.;
	const double wavePhaseStep = (waveFrequency / 44100.) * (M_PI * 2.);
	const double pulsePhaseStep = (2 / 44100.) * (M_PI * 2.);
	
	// rendering into the left channel
	for(int i = 0; i < inNumberFrames; i++) {
		outData[i]  = sin(context->wavePhase) * 0.25;
		outData[i] += sin(context->wavePhase * 0.5) * 0.25;
		outData[i] += sin(context->wavePhase * 0.75) * 0.25;
		context->wavePhase += wavePhaseStep;
		
		outData[i] *= sin(context->pulsePhase);
		context->pulsePhase += pulsePhaseStep;
	}
	
	// copying the left channel into all remaining channels
	for(int i = 1; i < ioData->mNumberBuffers; i++) {
		memcpy(ioData->mBuffers[i].mData, outData, ioData->mBuffers[i].mDataByteSize);
	}
	
	return noErr;
}

// Showing the UI for the effect unit of the clicked-on track
void auComplexRoutingApp::mouseDown( MouseEvent event )
{
	if(event.getX() > gridSize.x) {
		compressor.showUI("Compressor");
	} else if(event.getY() < gridSize.y) {
		distortion.showUI("Distortion");
	} else if(event.getY() < gridSize.y * 2) {
		lowPass.showUI("Low Pass Filter");
	} else if(event.getY() < gridSize.y * 3) {
		reverb.showUI("Reverb");
	} else {
		panner.showUI("Panner");
	}
}

// Saving the state of all of the effect units when "s" is pressed
void auComplexRoutingApp::keyDown( KeyEvent event )
{
	if(event.getChar() == 's') {
		distortion.savePreset(getAssetPath("distortion.aupreset"));
		lowPass.savePreset(getAssetPath("lowpass.aupreset"));
		reverb.savePreset(getAssetPath("reverb.aupreset"));
		panner.savePreset(getAssetPath("panner.aupreset"));
		compressor.savePreset(getAssetPath("compressor.aupreset"));
	}
}

void auComplexRoutingApp::update()
{
	gridSize = Vec2i(getWindowWidth() / 2., getWindowHeight() / 4.);
	// getting the last batch of samples which have passed through each of the taps
	kickTap.getSamples(kickBuffer);
	hatsTap.getSamples(hatsBuffer);
	snareTap.getSamples(snareBuffer);
	renderTap.getSamples(renderBuffer);
	mixdownTap.getSamples(mixdownBuffer);
}

PolyLine2f waveformForSamples(TapSampleBuffer buffer, float width, float height)
{
	PolyLine2f waveform;
	
	if(!buffer.empty()) {
		waveform.getPoints().reserve(buffer.size());
		
		const float xStep = width / (float)buffer.size();
		
		for(int i = 0; i < buffer.size(); i++) {
			float x = i * xStep;
			float y = buffer[i] * (height / 2.f) + (height / 2.f);
			waveform.push_back(Vec2f(x, y));
		}
	}
	
	return waveform;
}

void auComplexRoutingApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
	
	// drawing a solid rect if the cursor is over any of the tracks
	gl::color(0.7, 0.2, 0);
	drawHoverRect();
	
	// drawing the waveforms of each track
	gl::color(1,1,1);
	gl::pushMatrices();
	{
		gl::draw(waveformForSamples(kickBuffer, gridSize.x, gridSize.y));
		gl::translate(0, gridSize.y);
		gl::draw(waveformForSamples(hatsBuffer, gridSize.x, gridSize.y));
		gl::translate(0, gridSize.y);
		gl::draw(waveformForSamples(snareBuffer, gridSize.x, gridSize.y));
		gl::translate(0, gridSize.y);
		gl::draw(waveformForSamples(renderBuffer, gridSize.x, gridSize.y));
	}
	gl::popMatrices();
	
	// drawing the mixdown waveform
	gl::pushMatrices();
	{
		gl::translate(gridSize.x, 0);
		gl::draw(waveformForSamples(mixdownBuffer, gridSize.x, getWindowHeight()));
	}
	gl::popMatrices();
}

void auComplexRoutingApp::drawHoverRect()
{
	Vec2i mouseInWindow = getMousePos() - getWindowPos();
	bool isInWindow = !(mouseInWindow.x < 0 || mouseInWindow.x > getWindowWidth() || mouseInWindow.y < 0 || mouseInWindow.y > getWindowHeight());
	
	if( isInWindow ) {
		Rectf hoverRect;
		if(mouseInWindow.x > gridSize.x) {
			hoverRect = Rectf(gridSize.x, 0, getWindowWidth(), getWindowHeight());
		} else {
			int y = (mouseInWindow.y / gridSize.y) * gridSize.y;
			hoverRect = Rectf(0, y, gridSize.x, gridSize.y + y);
		}
		gl::drawSolidRect(hoverRect);
	}
}

auComplexRoutingApp::~auComplexRoutingApp()
{
	output.stop(); // <- this isn't strictly necessary
}

CINDER_APP_NATIVE( auComplexRoutingApp, RendererGl )
