/*
 This is part of a block for Audio Unit integration in Cinder (http://libcinder.org)
 Copyright (c) 2013, Adam Carlucci. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "GenericUnit.h"

namespace cinder { namespace audiounit {

// Wraps the AUMultiChannelMixer unit.
	
// This is a multiple-input, single-output mixer.
// Call setInputBusCount() to change the number
// of inputs on the mixer.

// You can use this unit to get access to the level
// of the audio going through it (in decibels).
// First call enableInputMetering() or
// enableOutputMetering(). After this, getInputLevel()
// or getOutputLevel() will return a float describing
// the current level in decibles (most likely in the
// range -120 to 0)

class Mixer : public GenericUnit
{
public:
	Mixer();
	
	void setInputVolume (float volume, int bus = 0);
	void setOutputVolume(float volume);
	void setPan(float pan, int bus = 0);
	
	bool setInputBusCount(UInt32 numberOfInputBusses);
	UInt32 getInputBusCount() const;
	
	float getInputLevel(int bus = 0) const;
	float getOutputLevel() const;
	
	void  enableInputMetering(int bus = 0);
	void  enableOutputMetering();
	
	void  disableInputMetering(int bus = 0);
	void  disableOutputMetering();
};

// Wraps the AUAudioFilePlayer unit.
	
// This audio unit allows you to play any file that
// Core Audio supports (mp3, aac, caf, aiff, etc)

class FilePlayer : public GenericUnit
{
	AudioFileID _fileID[1];
	ScheduledAudioFileRegion _region;
	
public:
	FilePlayer();
	~FilePlayer();
	
	bool   setFile(const fs::path &filePath);
	bool   setFile(const DataSourceRef fileSource);
	UInt32 getLength();
	void   setLength(UInt32 length);
	
	enum {
		CI_AU_LOOP_FOREVER = -1
	};
	
	// You can get the startTime arg from mach_absolute_time().
	// Note that all of these args are optional; you can just
	// call play() / loop() and it will start right away.
	void play(uint64_t startTime = 0);
	void loop(unsigned int timesToLoop = CI_AU_LOOP_FOREVER, uint64_t startTime = 0);
	void stop();
};

// Wraps the AUHAL output unit

// This unit drives the "pull" model of Core Audio and
// sends audio to the actual hardware (ie. speakers / headphones)

class Output : public GenericUnit
{
public:
	Output();
	~Output(){stop();}
	
	bool start();
	bool stop();
};
	
// Wraps the AUHAL output unit, but configures it
// for audio input instead of output
	
// This unit renders live input into a circular buffer,
// which can be pulled by connected units downstream.
	
// Note that this unit is a bit of an illusion, in that it
// does not make a "true" connection. Instead, it manually
// sets a render callback on downstream units and retreives
// samples from an internal buffer when render() is called.

class Input : public GenericUnit
{
	struct InputImpl;
	boost::shared_ptr<InputImpl> _impl;
	
	bool _isReady;
	bool configureInputDevice();
	
public:
	Input(unsigned int samplesToBuffer = 2048);
	~Input();
	
	GenericUnit& connectTo(GenericUnit &otherUnit, UInt32 destinationBus = 0, UInt32 sourceBus = 0);
	using GenericUnit::connectTo; // for connectTo(Tap&)
	
	OSStatus render(AudioUnitRenderActionFlags *ioActionFlags,
					const AudioTimeStamp *inTimeStamp,
					UInt32 inOutputBusNumber,
					UInt32 inNumberFrames,
					AudioBufferList *ioData);
	
	bool start();
	bool stop();
};
	
// Wraps the AUSampler unit.
	
// This is a basic few-frills sampler which can play
// audio files and "sound fonts" at various pitches.
// It also responds to MIDI. By default it starts up
// with a sine wave sample.
	
// Check out its GUI to see more of its features

class Sampler : public GenericUnit
{
public:
	Sampler();
	
	bool setSample(const fs::path &samplePath);
	bool setSamples(const std::vector<fs::path> &samplePaths);
};

#pragma mark - - OSX only below here - -

#if !TARGET_OS_IPHONE

// NetSend wraps the AUNetSend unit. 
// This audio unit allows you to send audio to
// an AUNetReceive via bonjour. It supports a handful
// of different stream formats as well.

// You can change the stream format by calling
// setFormat() with one of "kAUNetSendPresetFormat_"
// constants

// Multiple AUNetSends are differentiated by port
// number, NOT by bonjour name

class NetSend : public GenericUnit
{
public:
	NetSend();
	void setName(const std::string &name);
	void setPort(unsigned int portNumber);
	void setFormat(unsigned int formatIndex);
};

// NetReceive wraps the AUNetReceive unit.
// This audio unit receives audio from an AUNetSend.
// Call connectToHost() with the IP Address of a
// host running an instance of AUNetSend in order to
// connect.

// For example, myNetReceive.connectToHost("127.0.0.1")
// will start streaming audio from an AUNetSend on the
// same machine.

class NetReceive : public GenericUnit
{
public:
	NetReceive();
	void connectToHost(const std::string &ipAddress, unsigned long port = 52800);
	void disconnect();
};

// SpeechSynth wraps the AUSpeechSynthesis unit.
// This unit lets you access the Speech Synthesis API
// for text-to-speech on your mac (the same thing that
// powers the VoiceOver utility).

class SpeechSynth : public GenericUnit
{
	struct pImpl;
	boost::shared_ptr<pImpl> _impl;
	
public:
	SpeechSynth();
	
	void say(const std::string &phrase);
	void stop();
	
	void printAvailableVoices();
	std::vector<std::string> getAvailableVoices();
	bool setVoice(short voiceIndex);
	bool setVoice(const std::string &voiceName);
};

#endif // !TARGET_OS_IPHONE

} } // namespace cinder::audiounit
