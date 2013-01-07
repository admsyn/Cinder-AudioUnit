#include "GenericUnit.h"
#include "AudioUnitMidi.h"
#include "AudioUnitUtils.h"

using namespace cinder::audiounit;
using namespace std;

#pragma mark MIDI utilities

void MidiInputProc(const MIDINotification *message, void *refCon);
void MidiReadProc(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon);

vector<string> Midi::getSourceNames()
{
	vector<string> sourceNames;
	ItemCount sourceCount = MIDIGetNumberOfSources();
	for(int i = 0; i < sourceCount; i++) {
		MIDIEndpointRef source = MIDIGetSource(i);
		CFStringRef sourceName = NULL;
		MIDIObjectGetStringProperty(source, kMIDIPropertyName, &sourceName);
		char sourceNameCString[255];
		CFStringGetCString(sourceName, sourceNameCString, 255, kCFStringEncodingUTF8);
		sourceNames.push_back(sourceNameCString);
	}
	return sourceNames;
}

void Midi::printSourceNames()
{
	vector<string> midiSources = Midi::getSourceNames();
	for(int i = 0; i < midiSources.size(); i++) {
		cout << i << " : " << midiSources[i] << endl;
	}
}

#pragma mark - MIDI Receiver

MidiReceiver::MidiReceiver(const std::string &clientName)
{
	CFStringRef cName = CFStringCreateWithCString(kCFAllocatorDefault, clientName.c_str(), kCFStringEncodingUTF8);
	PRINT_IF_ERR(MIDIClientCreate(cName, MidiInputProc, this, &_client), "creating MIDI client");
	CFRelease(cName);
}

MidiReceiver::~MidiReceiver()
{
	MIDIPortDispose(_port);
	MIDIEndpointDispose(_endpoint);
}

bool MidiReceiver::createMidiDestination(const std::string &portName)
{
	OSStatus success = noErr;
	CFStringRef pName = CFStringCreateWithCString(NULL, portName.c_str(), kCFStringEncodingUTF8);
	success = MIDIDestinationCreate(_client, pName, MidiReadProc, &_unit, &_endpoint);
	CFRelease(pName);
	return (success == noErr);
}

bool MidiReceiver::connectToMidiSource(unsigned long midiSourceIndex)
{
	if(!_port) {
		OSStatus s = MIDIInputPortCreate(_client, 
										 CFSTR("Cinder MIDI Input Port"), 
										 MidiReadProc,
										 &_unit,
										 &_port);
		if(s != noErr) return false;
	}
	
	RETURN_BOOL(MIDIPortConnectSource(_port,
									  MIDIGetSource(midiSourceIndex),
									  NULL),
				"connecting MIDI receiver to source");
}

bool MidiReceiver::connectToMidiSource(const std::string &midiSourceName)
{
	vector<string> sources = Midi::getSourceNames();
	bool foundSource = false;
	int sourceIndex;
	for(int i = 0; i < sources.size() && !foundSource; i++) {
		if(midiSourceName == sources.at(i)) {
			foundSource = true;
			sourceIndex = i;
		}
	}
	
	if(foundSource) {
		return connectToMidiSource(sourceIndex);
	} else {
		return false;
	}
}

void MidiReceiver::disconnectFromMidiSource(unsigned long midiSourceIndex)
{
	PRINT_IF_ERR(MIDIPortDisconnectSource(_port, MIDIGetSource(midiSourceIndex)),
				 "disconnecting from MIDI source");
}

void MidiReceiver::routeMidiTo(GenericUnit &unitToRouteTo)
{
	_unit = unitToRouteTo.getUnitRef();
}

#pragma mark - Callbacks

// TODO: Do things with MIDI Input notifications
void MidiInputProc(const MIDINotification * message, void * refCon) { }

void MidiReadProc(const MIDIPacketList * pktlist, void * readProcRefCon, void * srcConnRefCon)
{
	MIDIPacket * packet = (MIDIPacket *)(pktlist->packet);
	AudioUnitRef unit   = *(static_cast<AudioUnitRef *>(readProcRefCon));
	
	if(!unit) return;
	
	for(int i = 0; i < pktlist->numPackets; i++) {	
		MusicDeviceMIDIEvent(*unit, packet->data[0], packet->data[1], packet->data[2], 0);
		packet = MIDIPacketNext(packet);
	}
}
