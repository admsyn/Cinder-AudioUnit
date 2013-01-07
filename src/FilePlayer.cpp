#include "GenericUnitSubclasses.h"
#include "AudioUnitUtils.h"
#import <mach/mach_time.h>

using namespace cinder::audiounit;

AudioComponentDescription filePlayerDesc =
{
	kAudioUnitType_Generator,
	kAudioUnitSubType_AudioFilePlayer,
	kAudioUnitManufacturer_Apple
};

FilePlayer::FilePlayer()
{
	_desc = filePlayerDesc;
	initUnit();
}

FilePlayer::~FilePlayer()
{
	stop();
	AudioFileClose(_fileID[0]);
}

#pragma mark - Properties

// TODO: non-local file support?
bool FilePlayer::setFile(const DataSourceRef fileSource)
{
	if(fileSource->isFilePath()) {
		return setFile(fileSource->getFilePath());
	} else {
		return false;
	}
}

bool FilePlayer::setFile(const fs::path &filePath)
{
	CFURLRef fileURL;
	fileURL = CreateURLFromPath(filePath);
	
	if(_fileID[0]) AudioFileClose(_fileID[0]);
	
	OSStatus s = AudioFileOpenURL(fileURL, kAudioFileReadPermission, 0, _fileID);
	
	CFRelease(fileURL);
	
	if(s != noErr) {
		std::cout << "Error " << s << " while opening file at " << filePath << std::endl;
		return false;
	}
	
	UInt64 numPackets = 0;
	UInt32 dataSize   = sizeof(numPackets);
	
	AudioFileGetProperty(_fileID[0],
	                     kAudioFilePropertyAudioDataPacketCount,
	                     &dataSize,
	                     &numPackets);
	
	AudioStreamBasicDescription asbd = {0};
	dataSize = sizeof(asbd);
	
	AudioFileGetProperty(_fileID[0], kAudioFilePropertyDataFormat, &dataSize, &asbd);
	
	// defining a region which basically says "play the whole file"
	memset(&_region, 0, sizeof(_region));
	_region.mTimeStamp.mFlags       = kAudioTimeStampHostTimeValid;
	_region.mTimeStamp.mSampleTime  = 0;
	_region.mCompletionProc         = NULL;
	_region.mCompletionProcUserData = NULL;
	_region.mAudioFile              = _fileID[0];
	_region.mLoopCount              = 0;
	_region.mStartFrame             = 0;
	_region.mFramesToPlay           = (UInt32) numPackets * asbd.mFramesPerPacket;
	
	// setting the file ID now since it seems to have some overhead.
	// Doing it now ensures you'll get sound pretty much instantly after
	// calling play() (subsequent calls don't have the overhead)
	RETURN_BOOL(AudioUnitSetProperty(*_unit,
									 kAudioUnitProperty_ScheduledFileIDs,
									 kAudioUnitScope_Global,
									 0,
									 _fileID,
									 sizeof(_fileID)),
				"setting file player's file ID");
}

void FilePlayer::setLength(UInt32 length)
{
	_region.mFramesToPlay = length;
}

UInt32 FilePlayer::getLength()
{
	return _region.mFramesToPlay;
}

#pragma mark - Playback

void FilePlayer::play(uint64_t startTime)
{
	if(!(_region.mTimeStamp.mFlags & kAudioTimeStampHostTimeValid))
	{
		std::cout << "ofxAudioUnitFilePlayer has no file to play" << std::endl;
		return;
	}
	
	RETURN_IF_ERR(AudioUnitSetProperty(*_unit,
									   kAudioUnitProperty_ScheduledFileIDs,
									   kAudioUnitScope_Global,
									   0,
									   _fileID,
									   sizeof(_fileID)),
				  "setting file player's file ID");
	
	RETURN_IF_ERR(AudioUnitSetProperty(*_unit,
									   kAudioUnitProperty_ScheduledFileRegion,
									   kAudioUnitScope_Global,
									   0,
									   &_region,
									   sizeof(_region)),
				  "setting file player region");
	
	if(startTime == 0) {
		startTime = mach_absolute_time();
	}

	AudioTimeStamp startTimeStamp = {0};
	FillOutAudioTimeStampWithHostTime(startTimeStamp, startTime);
	
	RETURN_IF_ERR(AudioUnitSetProperty(*_unit,
									   kAudioUnitProperty_ScheduleStartTimeStamp,
									   kAudioUnitScope_Global,
									   0,
									   &startTimeStamp,
									   sizeof(startTimeStamp)),
				  "setting file player start time");
}

void FilePlayer::loop(unsigned int timesToLoop, uint64_t startTime)
{
	_region.mLoopCount = timesToLoop;
	play(startTime);
}

void FilePlayer::stop()
{
	reset();
}
