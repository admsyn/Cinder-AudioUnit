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

typedef std::vector<AudioUnitSampleType> TapSampleBuffer;
	
// The Tap acts like an Audio Unit (as in, you can connect
// it to other Audio Units). In reality, it hooks up two
// Audio Units to each other, but also copies the samples
// that are passing through the chain. This allows you to
// extract the samples to do audio visualization, FFT
// analysis and so on.

// Samples retreived from the Tap will be floating point
// numbers between -1 and 1. It is possible for them to exceed
// this range, but this will typically be due to an Audio Unit
// overloading its output.

// Note that if you just want to know how loud the audio is,
// the Mixer will allow you to access that value with less overhead.

class Tap
{
	struct TapImpl;
	boost::shared_ptr<TapImpl> _impl;
	
public:
	Tap(unsigned int samplesToTrack = 4096);
	~Tap();
	
	GenericUnit& connectTo(GenericUnit &destination, UInt32 destinationBus = 0, UInt32 sourceBus = 0);
	
	void setSource(GenericUnit * source);
	void setSource(AURenderCallbackStruct callback, UInt32 channels = 2);
	
	void getSamples(TapSampleBuffer &buffer); // retrieves a mono buffer
	void getSamples(std::vector<TapSampleBuffer> &buffers);
};

} } // namespace cinder::audiounit
