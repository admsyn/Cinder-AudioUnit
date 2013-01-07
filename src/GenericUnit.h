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

#include <AudioToolbox/AudioToolbox.h>
#include "cinder/Filesystem.h"
#include "AudioUnitTypes.h"

#ifndef CI_AU_ENABLE_GUI
	#define CI_AU_ENABLE_GUI !(TARGET_OS_IPHONE)
#endif

namespace cinder { namespace audiounit {
	
class Tap;

// GenericUnit is a general-purpose class to simplify using Audio Units in
// Cinder apps. It can be used to represent any Audio Unit. Note that
// there are several subclasses of GenericUnit which provide additional
// convenience functions & utilities for specific Audio Units.

class GenericUnit
{	
public:
	GenericUnit(){};
	explicit GenericUnit(AudioComponentDescription description);
	explicit GenericUnit(OSType type,
						 OSType subType,
						 OSType manufacturer = kAudioUnitManufacturer_Apple);
	
	explicit GenericUnit(const GenericUnit &orig);
	GenericUnit& operator=(const GenericUnit &orig);
	
	explicit GenericUnit(GenericUnit&& orig);
	GenericUnit& operator=(GenericUnit&& orig);
	
	virtual ~GenericUnit();
	
	virtual GenericUnit& connectTo(GenericUnit &otherUnit, UInt32 destinationBus = 0, UInt32 sourceBus = 0);
	virtual Tap&  connectTo(Tap &tap);
	
	// explicit and implicit conversions to the underlying AudioUnit struct
	AudioUnit getUnit()       {return *_unit;}
	operator AudioUnit()      {return *_unit;}
	AudioUnitRef getUnitRef() {return _unit;}
	
	// By default, this just calls AudioUnitRender() on the underlying
	// AudioUnit. However, some subclasses require more complex rendering
	// behaviour (for example, AUInput pulls from an internal buffer instead)
	virtual OSStatus render(AudioUnitRenderActionFlags *ioActionFlags,
							const AudioTimeStamp *inTimeStamp,
							UInt32 inOutputBusNumber,
							UInt32 inNumberFrames,
							AudioBufferList *ioData);
	
	// These functions expect an absolute path to the preset file (including the file extension).
	// The file extension should be ".aupreset" by convention, but it's not necessary
	bool savePreset(const fs::path &presetPath) const;
	bool loadPreset(const fs::path &presetPath);
	
	bool savePreset(const DataSourceRef presetSource) const;
	bool loadPreset(const DataSourceRef presetSource);
	
	bool savePreset(const CFURLRef &presetURL) const;
	bool loadPreset(const CFURLRef &presetURL);
	
	void setRenderCallback(AURenderCallbackStruct callback, UInt32 destinationBus = 0);
	void reset(){AudioUnitReset(*_unit, kAudioUnitScope_Global, 0);}
	
#if CI_AU_ENABLE_GUI
	void showUI(const std::string &title = "Audio Unit UI",
				UInt32 x = 50,
				UInt32 y = 50,
				bool forceGeneric = false);
#endif

protected:
	AudioUnitRef _unit;
	AudioComponentDescription _desc;
	
	void initUnit();

	static void AudioUnitDeleter(AudioUnit * unit);
};

} } // namespace cinder::audiounit
