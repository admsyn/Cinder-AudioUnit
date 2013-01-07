#if !(TARGET_OS_IPHONE)

#include "GenericUnitSubclasses.h"
#include "AudioUnitUtils.h"

using namespace cinder::audiounit;

AudioComponentDescription netSendDesc = {
	kAudioUnitType_Effect,
	kAudioUnitSubType_NetSend,
	kAudioUnitManufacturer_Apple
};

NetSend::NetSend()
{
	_desc = netSendDesc;
	initUnit();
	setName("Cinder");
}

void NetSend::setPort(unsigned int portNumber)
{
	UInt32 pNum = portNumber;
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAUNetSendProperty_PortNum,
									  kAudioUnitScope_Global,
									  0,
									  &pNum,
									  sizeof(pNum)),
				 "setting net send port number");
}

void NetSend::setName(const std::string &name)
{
	CFStringRef serviceName = CFStringCreateWithCString(kCFAllocatorDefault,
														name.c_str(),
														kCFStringEncodingUTF8);
	
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAUNetSendProperty_ServiceName,
									  kAudioUnitScope_Global,
									  0,
									  &serviceName,
									  sizeof(serviceName)),
				 "setting net send service name");
	
	CFRelease(serviceName);
}

void NetSend::setFormat(unsigned int formatIndex)
{
	UInt32 format = formatIndex;
	PRINT_IF_ERR(AudioUnitSetProperty(*_unit,
									  kAUNetSendProperty_TransmissionFormatIndex,
									  kAudioUnitScope_Global,
									  0,
									  &format,
									  sizeof(format)),
				 "setting net send format");
}

#endif //TARGET_OS_IPHONE
