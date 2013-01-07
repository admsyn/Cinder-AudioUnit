#include "GenericUnitSubclasses.h"
#include "AudioUnitUtils.h"

using namespace cinder::audiounit;

AudioComponentDescription mixerDesc = {
	kAudioUnitType_Mixer,
	kAudioUnitSubType_MultiChannelMixer,
	kAudioUnitManufacturer_Apple
};

Mixer::Mixer()
{
	_desc = mixerDesc;
	initUnit();
	
	// Default volume is 0, which can make things seem like they aren't working.
	// Setting the volumes to 1 instead.
	int busses = getInputBusCount();
	for(int i = 0; i < busses; i++) {
		setInputVolume(1, i);
	}
	setOutputVolume(1);
}

#pragma mark - Volume / Pan

void Mixer::setInputVolume(float volume, int bus)
{
	PRINT_IF_ERR(AudioUnitSetParameter(*_unit,
									   kMultiChannelMixerParam_Volume,
									   kAudioUnitScope_Input,
									   bus,
									   volume,
									   0),
				 "setting mixer input gain");
}

void Mixer::setOutputVolume(float volume)
{
	PRINT_IF_ERR(AudioUnitSetParameter(*_unit,
									   kMultiChannelMixerParam_Volume,
									   kAudioUnitScope_Output,
									   0,
									   volume,
									   0),
				 "setting mixer output gain");
}

void Mixer::setPan(float pan, int bus)
{
	PRINT_IF_ERR(AudioUnitSetParameter(*_unit,
									   kMultiChannelMixerParam_Pan,
									   kAudioUnitScope_Input,
									   bus,
									   pan,
									   0),
				 "setting mixer pan");
}

#pragma mark - Busses

bool Mixer::setInputBusCount(UInt32 numberOfInputBusses)
{
	RETURN_BOOL(AudioUnitSetProperty(*_unit,
									 kAudioUnitProperty_ElementCount,
									 kAudioUnitScope_Input,
									 0,
									 &numberOfInputBusses,
									 sizeof(numberOfInputBusses)),
				"setting number of input busses");
}

UInt32 Mixer::getInputBusCount() const
{
	UInt32 busCount;
	UInt32 busCountSize = sizeof(busCount);
	PRINT_IF_ERR(AudioUnitGetProperty(*_unit,
									  kAudioUnitProperty_ElementCount,
									  kAudioUnitScope_Input,
									  0,
									  &busCount,
									  &busCountSize),
				 "getting input bus count");
	return busCount;
}

#pragma mark - Metering

float Mixer::getInputLevel(int bus) const
{	
	AudioUnitParameterValue level;
	PRINT_IF_ERR(AudioUnitGetParameter(*_unit,
									   kMultiChannelMixerParam_PreAveragePower,
									   kAudioUnitScope_Input,
									   bus,
									   &level),
				 "getting mixer input level");
	return level;
}

float Mixer::getOutputLevel() const
{	
	AudioUnitParameterValue level;
	PRINT_IF_ERR(AudioUnitGetParameter(*_unit,
									   kMultiChannelMixerParam_PreAveragePower,
									   kAudioUnitScope_Output,
									   0,
									   &level),
				 "getting mixer output level");
	return level;
}

void Mixer::enableInputMetering(int bus)
{
	UInt32 on = 1;
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAudioUnitProperty_MeteringMode,
									  kAudioUnitScope_Input,
									  bus,
									  &on,
									  sizeof(on)),
				 "enabling input metering");
}

void Mixer::enableOutputMetering()
{
	UInt32 on = 1;
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAudioUnitProperty_MeteringMode,
									  kAudioUnitScope_Output,
									  0,
									  &on,
									  sizeof(on)),
				 "enabling output metering");
}

void Mixer::disableInputMetering(int bus)
{
	UInt32 off = 0;
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAudioUnitProperty_MeteringMode,
									  kAudioUnitScope_Input,
									  bus,
									  &off,
									  sizeof(off)),
				 "disabling input metering");
}

void Mixer::disableOutputMetering()
{
	UInt32 off = 0;
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
						 kAudioUnitProperty_MeteringMode,
						 kAudioUnitScope_Output,
						 0,
						 &off,
						 sizeof(off)),
				 "disabling output metering");
}
