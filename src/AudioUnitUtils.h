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

#include "AudioUnitTypes.h"
#include "cinder/Filesystem.h"
#include <string>
#include <sstream>

#define CINDER_AUDIOUNIT_LOG_ERRORS

#ifdef  CINDER_AUDIOUNIT_LOG_ERRORS
#define AU_LOG(status, stage) std::cout << "Error " << (status) << " while " << (stage) << std::endl
#else
#define AU_LOG(status, stage)
#endif

// these macros make the "do core audio thing, check for error" process less tedious
#define PRINT_IF_ERR(s, stage){\
OSStatus status = (s);\
if(status!=noErr){\
	AU_LOG(status, stage);\
}}

#define RETURN_IF_ERR(s, stage){\
OSStatus status = (s);\
if(status!=noErr){\
	AU_LOG(status, stage);\
	return;\
}}

#define RETURN_FALSE_IF_ERR(s, stage){\
OSStatus status = (s);\
if(status!=noErr){\
	AU_LOG(status, stage);\
	return false;\
}}

#define RETURN_STATUS_IF_ERR(s, stage){\
OSStatus status = (s);\
if(status!=noErr){\
	AU_LOG(status, stage);\
	return status;\
}}

#define RETURN_BOOL(s, stage){\
OSStatus status = (s);\
if(status!=noErr){\
	AU_LOG(status, stage);\
	return false;\
}\
return true;}

static std::string StringForOSType(const OSType &type)
{
	char s[4] = {char(type >> 24), char(type >> 16), char(type >> 8), char(type)};
	return std::string(s, 4);
}

static std::string StringForAudioComponentDescription(const AudioComponentDescription &desc)
{
	std::stringstream ss;
	ss << "'" << StringForOSType(desc.componentType) << "' ";
	ss << "'" << StringForOSType(desc.componentSubType) << "' ";
	ss << "'" << StringForOSType(desc.componentManufacturer) << "'";
	return ss.str();
}

static AudioBufferList * AudioBufferListAlloc(UInt32 channels, UInt32 samplesPerChannel)
{
	AudioBufferList * bufferList = NULL;
	size_t bufferListSize = offsetof(AudioBufferList, mBuffers[0]) + (sizeof(AudioBuffer) * channels);
	bufferList = (AudioBufferList *)calloc(1, bufferListSize);
	bufferList->mNumberBuffers = channels;
	
	for(UInt32 i = 0; i < bufferList->mNumberBuffers; i++) {
		bufferList->mBuffers[i].mNumberChannels = 1;
		bufferList->mBuffers[i].mDataByteSize   = samplesPerChannel * sizeof(AudioUnitSampleType);
		bufferList->mBuffers[i].mData           = calloc(samplesPerChannel, sizeof(AudioUnitSampleType));
	}
	return bufferList;
}

static void AudioBufferListRelease(AudioBufferList * bufferList)
{
	for(int i = 0; i < bufferList->mNumberBuffers; i++) {
		free(bufferList->mBuffers[i].mData);
	}
		
	free(bufferList);
}

static std::string StringForPathFromURL(const CFURLRef &urlRef)
{
	CFStringRef filePath = CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
	char buf[PATH_MAX];
	CFStringGetCString(filePath, buf, PATH_MAX, kCFStringEncodingUTF8);
	CFRelease(filePath);
	return std::string(buf);
}

static CFURLRef CreateURLFromPath(const ci::fs::path &path)
{
	return CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
												   (const UInt8 *)path.c_str(),
												   path.string().length(),
												   false);
}
