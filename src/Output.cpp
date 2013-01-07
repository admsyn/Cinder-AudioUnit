#include "GenericUnitSubclasses.h"
#include "AudioUnitUtils.h"

using namespace cinder::audiounit;

AudioComponentDescription outputDesc = {
	kAudioUnitType_Output,
#if TARGET_OS_IPHONE
	kAudioUnitSubType_RemoteIO,
#else
	kAudioUnitSubType_HALOutput,
#endif
	kAudioUnitManufacturer_Apple
};


Output::Output()
{
	_desc = outputDesc;
	initUnit();
}

bool Output::start()
{
	RETURN_BOOL(AudioOutputUnitStart(*_unit), "starting output unit");
}

bool Output::stop()
{
	RETURN_BOOL(AudioOutputUnitStop(*_unit), "stopping output unit");
}
