#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/TextureFont.h"

#include "AudioUnit.h"

using namespace ci;
using namespace ci::app;
using namespace ci::au; // <- alias for cinder::audiounit
using namespace std;

class auMidiApp : public AppNative {
  public:
	void setup();
	void keyDown( KeyEvent event );
	void update();
	void draw();
	
	MidiReceiver midiReceiver;
	Sampler sampler;
	Output output;
	GenericUnit delay;
	
	Tap tap;
	TapSampleBuffer sampleBuffer;
};

void auMidiApp::setup()
{
	delay = GenericUnit(kAudioUnitType_Effect, kAudioUnitSubType_Delay);
	sampler.connectTo(delay).connectTo(tap).connectTo(output);
	output.start();
	
	// The midi receiver can either make a connection passively or actively.
	
	// Passively = creating a destination and waiting for another application
	// or midi object to send messages. For example, the following creates a
	// Midi destination which an application like Ableton Live can send Midi
	// messages to directly. (Note that if you test this with Ableton, you'll
	// need to go into the Midi preferences and turn "Track" on for this
	// destination)
	midiReceiver.createMidiDestination("Cinder AU MIDI");
	
	// Actively = connecting to an existing midi source, such as a keyboard
	// or other piece of hardware plugged into this machine. Uncomment the
	// following to print the name of all of the available devices, then
	// attempt to connect to one named "source name"
//	Midi::printSourceNames();
//	midiReceiver.connectToMidiSource("source name");
	
	// Here, we're telling the midiReceiver to send every message it gets
	// to the sampler. The sampler starts up with a sine wave sample pre-loaded
	// by default.
	midiReceiver.routeMidiTo(sampler);
}

void auMidiApp::keyDown( KeyEvent event )
{
	if(event.getChar() == 's') {
		sampler.showUI();
	} else if(event.getChar() == 'd') {
		delay.showUI();
	}
}

void auMidiApp::update()
{
}

void auMidiApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
	
	tap.getSamples(sampleBuffer);
	PolyLine2f waveform;
	for(int i = 0; i < sampleBuffer.size(); i++) {
		float x = i / (float)sampleBuffer.size() * (float)getWindowWidth();
		float y = ((sampleBuffer[i] + 1) / 2.) * (float)getWindowHeight();
		waveform.push_back(Vec2f(x,y));
	}
	
	gl::draw(waveform);
}

CINDER_APP_NATIVE( auMidiApp, RendererGl )
