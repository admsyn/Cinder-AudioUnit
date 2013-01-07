#include "GenericUnitSubclasses.h"
#include "AudioUnitUtils.h"
#include "cinder/Filesystem.h"

using namespace cinder::audiounit;

AudioComponentDescription samplerDesc = {
	kAudioUnitType_MusicDevice,
	kAudioUnitSubType_Sampler,
	kAudioUnitManufacturer_Apple
};

Sampler::Sampler()
{
	_desc = samplerDesc;
	initUnit();
}

bool Sampler::setSample(const fs::path &samplePath)
{
	CFURLRef sampleURL[1] = {
		CreateURLFromPath(samplePath)
	};
	
	CFArrayRef sample = CFArrayCreate(NULL, (const void **)&sampleURL, 1, &kCFTypeArrayCallBacks);

	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAUSamplerProperty_LoadAudioFiles,
									  kAudioUnitScope_Global,
									  0,
									  &sample,
									  sizeof(sample)),
				 "setting ofxAudioUnitSampler's source sample");
	
	CFRelease(sample);
	CFRelease(sampleURL[0]);
	
	return true;
}

bool Sampler::setSamples(const std::vector<fs::path> &samplePaths)
{
	CFURLRef sampleURLs[samplePaths.size()];
	
	for(int i = 0; i < samplePaths.size(); i++) {
		sampleURLs[i] = CreateURLFromPath(samplePaths[i]);
	}
	
	CFArrayRef samples = CFArrayCreate(NULL, (const void **)&sampleURLs, samplePaths.size(), &kCFTypeArrayCallBacks);
	
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAUSamplerProperty_LoadAudioFiles,
									  kAudioUnitScope_Global,
									  0,
									  &samples,
									  sizeof(samples)),
				 "setting sampler's source samples");
	
	for(int i = 0; i < samplePaths.size(); i++) {
		CFRelease(sampleURLs[i]);
	}
	
	CFRelease(samples);
	
	return true;
}
