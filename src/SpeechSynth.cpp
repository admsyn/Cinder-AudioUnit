#if !(TARGET_OS_IPHONE)

#include <ApplicationServices/ApplicationServices.h>
#include "GenericUnitSubclasses.h"
#include "AudioUnitUtils.h"

using namespace cinder::audiounit;
using namespace std;

AudioComponentDescription speechDesc =
{
	kAudioUnitType_Generator,
	kAudioUnitSubType_SpeechSynthesis,
	kAudioUnitManufacturer_Apple
};

struct SpeechSynth::pImpl
{
	SpeechChannel channel;
};

SpeechSynth::SpeechSynth() : _impl(new pImpl)
{
	_desc = speechDesc;
	initUnit();
	
	UInt32 dataSize = sizeof(SpeechChannel);
	
	PRINT_IF_ERR(AudioUnitGetProperty(*_unit,
									  kAudioUnitProperty_SpeechChannel,
									  kAudioUnitScope_Global,
									  0,
									  &_impl->channel,
									  &dataSize),
				 "getting speech channel");
}

void SpeechSynth::say(const string &phrase)
{
	CFStringRef string = CFStringCreateWithCString(kCFAllocatorDefault,
												   phrase.c_str(),
												   kCFStringEncodingUTF8);
	
	SpeakCFString(_impl->channel, string, NULL);
	
	CFRelease(string);
}

void SpeechSynth::stop()
{
	StopSpeech(_impl->channel);
}

void SpeechSynth::printAvailableVoices()
{
	vector<string> voiceNames = getAvailableVoices();
	for(int i = 0; i < voiceNames.size(); i++)
	{
		cout << i+1 << ":\t" << voiceNames.at(i) << endl;
	}
}

vector<string> SpeechSynth::getAvailableVoices()
{
	SInt16 numVoices = 0;
	CountVoices(&numVoices);
	vector<string> voiceNames;
	
	// voices seem to be 1-indexed instead of 0-indexed
	for(short i = 1; i <= numVoices; i++) {
		VoiceSpec vSpec;
		VoiceDescription vDesc;
		GetIndVoice(i, &vSpec);
		GetVoiceDescription(&vSpec, &vDesc, sizeof(VoiceDescription));
		string name = string((const char *)vDesc.name);
		
		// the first "character" in vDesc.name is actually just the length
		// of the string.
		voiceNames.push_back(string(name, 1, name[0]));
	}
	return voiceNames;
}

bool SpeechSynth::setVoice(short voiceIndex)
{
	VoiceSpec vSpec;
	OSErr err;
	err = GetIndVoice(voiceIndex, &vSpec);
	
	if(!err) {
		StopSpeech(_impl->channel);
		err = SetSpeechInfo(_impl->channel, soCurrentVoice, &vSpec);
	}
	
	return (err == 0);
}

bool SpeechSynth::setVoice(const string &voiceName)
{
	vector<string> voiceNames = getAvailableVoices();
	
	for(short i = 0; i < voiceNames.size(); i++) {
		if(voiceNames.at(i) == voiceName) {
			return setVoice(i+1);
		}
	}
	
	return false;
}

#endif //TARGET_OS_IPHONE
