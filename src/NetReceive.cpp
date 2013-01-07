#if !(TARGET_OS_IPHONE)

#include "GenericUnitSubclasses.h"
#include "AudioUnitUtils.h"
#include <sstream>

using namespace cinder::audiounit;
using namespace std;

AudioComponentDescription netReceiveDesc = {
	kAudioUnitType_Generator,
	kAudioUnitSubType_NetReceive,
	kAudioUnitManufacturer_Apple
};

NetReceive::NetReceive()
{
	_desc = netReceiveDesc;
	initUnit();
}

void NetReceive::connectToHost(const std::string &address, unsigned long port)
{
	stringstream ss;
	ss << address << ":" << port;
	CFStringRef hostName = CFStringCreateWithCString(kCFAllocatorDefault,
													 ss.str().c_str(),
													 kCFStringEncodingUTF8);
	
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAUNetReceiveProperty_Hostname,
									  kAudioUnitScope_Global,
									  0,
									  &hostName,
									  sizeof(hostName)),
				 "setting net receive host name");
	
	// setting net send disconnect to 0 to connect net receive because that makes sense
	UInt32 connect = 0;
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAUNetSendProperty_Disconnect,
									  kAudioUnitScope_Global,
									  0,
									  &connect,
									  sizeof(connect)),
				 "connecting net receive");
	
	CFRelease(hostName);
}

void NetReceive::disconnect()
{
	UInt32 disconnect = 1;
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAUNetSendProperty_Disconnect,
									  kAudioUnitScope_Global,
									  0,
									  &disconnect,
									  sizeof(disconnect)),
				 "disconnecting net receive");
}

#endif //TARGET_OS_IPHONE
