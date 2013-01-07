#include "GenericUnit.h"
#include "AudioUnitTap.h"
#include "AudioUnitUtils.h"
#include <iostream>

using namespace cinder::audiounit;
using namespace std;

GenericUnit::GenericUnit(AudioComponentDescription description)
: _desc(description)
{
	initUnit();
}

GenericUnit::GenericUnit(OSType type,
						 OSType subType,
						 OSType manufacturer)
{
	_desc.componentType         = type;
	_desc.componentSubType      = subType;
	_desc.componentManufacturer = manufacturer;
	_desc.componentFlags        = 0;
	_desc.componentFlagsMask    = 0;
	initUnit();
};

GenericUnit::GenericUnit(const GenericUnit &orig)
: _desc(orig._desc)
{
	initUnit();
}

GenericUnit& GenericUnit::operator=(const GenericUnit &orig)
{
	if(this != &orig) {
		_desc = orig._desc;
		initUnit();
	}
	return *this;
}

GenericUnit::GenericUnit(GenericUnit&& orig)
: _desc(move(orig._desc))
, _unit(move(orig._unit))
{
}

GenericUnit& GenericUnit::operator=(GenericUnit &&orig)
{
	if(this != &orig) {
		_desc = move(orig._desc);
		_unit = move(orig._unit);
	}
	return *this;
}

void GenericUnit::initUnit()
{
	AudioComponent component = AudioComponentFindNext(NULL, &_desc);
	if(!component) {
		cout << "Couldn't locate an Audio Unit to match description "
		<< StringForAudioComponentDescription(_desc) << endl;
		return;
	}
	
	_unit = AudioUnitRef((AudioUnit *)malloc(sizeof(AudioUnit)), AudioUnitDeleter);
	RETURN_IF_ERR(AudioComponentInstanceNew(component, _unit.get()), "creating new unit");
	RETURN_IF_ERR(AudioUnitInitialize(*_unit),                       "initializing unit");
}

void GenericUnit::AudioUnitDeleter(AudioUnit * unit)
{
	PRINT_IF_ERR(AudioUnitUninitialize(*unit),         "uninitializing unit");
	PRINT_IF_ERR(AudioComponentInstanceDispose(*unit), "disposing unit");
}

GenericUnit::~GenericUnit()
{
	// _unit will be freed by AudioUnitDeleter
}

#pragma mark - Connections

GenericUnit& GenericUnit::connectTo(GenericUnit &otherUnit, UInt32 destinationBus, UInt32 sourceBus)
{
	AudioUnitConnection connection;
	connection.sourceAudioUnit    = *_unit;
	connection.sourceOutputNumber = sourceBus;
	connection.destInputNumber    = destinationBus;
	
	PRINT_IF_ERR(AudioUnitSetProperty(otherUnit,
									  kAudioUnitProperty_MakeConnection,
									  kAudioUnitScope_Input,
									  destinationBus,
									  &connection,
									  sizeof(AudioUnitConnection)),
				 "connecting units");
	
	return otherUnit;
}

Tap& GenericUnit::connectTo(Tap &tap)
{
	tap.setSource(this);
	return tap;
}

OSStatus GenericUnit::render(AudioUnitRenderActionFlags *flags,
							 const AudioTimeStamp *timestamp,
							 UInt32 bus,
							 UInt32 frames,
							 AudioBufferList *data)
{
	return AudioUnitRender(*_unit, flags, timestamp, bus, frames, data);
}

#pragma mark - Presets

bool GenericUnit::loadPreset(const DataSourceRef presetSource)
{
	if(presetSource->isFilePath()) {
		return loadPreset(presetSource->getFilePath());
	} else {
		return false;
	}
}

bool GenericUnit::savePreset(const DataSourceRef presetSource) const
{
	if(presetSource->isFilePath()) {
		return savePreset(presetSource->getFilePath());
	} else {
		return false;
	}
}

bool GenericUnit::loadPreset(const fs::path &path)
{
	CFURLRef URL = CreateURLFromPath(path);
	if(URL) {
		bool successful = loadPreset(URL);
		CFRelease(URL);
		return successful;
	} else {
		return false;
	}
}

bool GenericUnit::savePreset(const fs::path &path) const
{
	CFURLRef URL = CreateURLFromPath(path);
	if(URL) {
		bool successful = savePreset(URL);
		CFRelease(URL);
		return successful;
	} else {
		return false;
	}
}

bool GenericUnit::loadPreset(const CFURLRef &presetURL)
{
	CFDataRef presetData;
	SInt32    errorCode;
	Boolean   dataInitSuccess = CFURLCreateDataAndPropertiesFromResource(NULL,
																		 presetURL,
																		 &presetData,
																		 NULL,
																		 NULL,
																		 &errorCode);
	
	
	OSStatus presetSetStatus;
	
	if(dataInitSuccess) {
		CFPropertyListRef presetPList = CFPropertyListCreateWithData(kCFAllocatorDefault,
																	 presetData,
																	 kCFPropertyListImmutable,
																	 NULL,
																	 NULL);
		
		presetSetStatus = AudioUnitSetProperty(*_unit,
											   kAudioUnitProperty_ClassInfo,
											   kAudioUnitScope_Global,
											   0,
											   &presetPList,
											   sizeof(presetPList));
		CFRelease(presetData);
		CFRelease(presetPList);
	} else {
		cout << "Couldn't read preset at " << StringForPathFromURL(presetURL) << endl;
	}
	
	bool presetSetSuccess = dataInitSuccess && (presetSetStatus == noErr);
	
	if(presetSetSuccess) {
		// Notify any listeners that params probably changed
		AudioUnitParameter paramNotification;
		paramNotification.mAudioUnit   = *_unit;
		paramNotification.mParameterID = kAUParameterListener_AnyParameter;
		AUParameterListenerNotify(NULL, NULL, &paramNotification);
	} else {
		cout << "Error " << presetSetStatus << " while attempting to set preset at "
		<< StringForPathFromURL(presetURL) << endl;
	}
	
	return presetSetSuccess;
}

bool GenericUnit::savePreset(const CFURLRef &presetURL) const
{
	// getting preset data (plist) from AU
	CFPropertyListRef preset;
	UInt32 presetSize = sizeof(preset);
	RETURN_FALSE_IF_ERR(AudioUnitGetProperty(*_unit,
											 kAudioUnitProperty_ClassInfo,
											 kAudioUnitScope_Global,
											 0,
											 &preset,
											 &presetSize),
						"getting preset data");
	
	if(!CFPropertyListIsValid(preset, kCFPropertyListXMLFormat_v1_0)) return false;
	
	// if succesful, write it to a file
	CFDataRef presetData = CFPropertyListCreateXMLData(kCFAllocatorDefault, preset);
	
	SInt32 errorCode;
	Boolean writeSuccess = CFURLWriteDataAndPropertiesToResource(presetURL,
																 presetData,
																 NULL,
																 &errorCode);
	
	CFRelease(presetData);
	
	if(!writeSuccess)
	{
		cout << "Error " << errorCode << " writing preset file at "
		<< StringForPathFromURL(presetURL) << endl;
	}
	
	return writeSuccess;
}

#pragma mark - Render Callbacks

void GenericUnit::setRenderCallback(AURenderCallbackStruct callback, UInt32 bus)
{
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAudioUnitProperty_SetRenderCallback,
									  kAudioUnitScope_Input,
									  bus,
									  &callback,
									  sizeof(callback)),
				 "setting render callback");
}
