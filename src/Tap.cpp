#include "AudioUnitTap.h"
#include "AudioUnitUtils.h"
#include "TPCircularBuffer/TPCircularBuffer.h"

using namespace cinder::audiounit;
using namespace std;

static OSStatus RenderAndCopy(void * inRefCon,
							  AudioUnitRenderActionFlags * ioActionFlags,
							  const AudioTimeStamp * inTimeStamp,
							  UInt32 inBusNumber,
							  UInt32 inNumberFrames,
							  AudioBufferList * ioData);

static OSStatus SilentRenderCallback(void * inRefCon,
									 AudioUnitRenderActionFlags * ioActionFlags,
									 const AudioTimeStamp *	inTimeStamp,
									 UInt32 inBusNumber,
									 UInt32	inNumberFrames,
									 AudioBufferList * ioData);

typedef enum
{
	TapSourceNone,
	TapSourceUnit,
	TapSourceCallback
}
TapSourceType;

struct TapContext
{
	TapSourceType sourceType;
	GenericUnit * sourceUnit;
	UInt32 sourceBus;
	AURenderCallbackStruct sourceCallback;
	vector<TPCircularBuffer> circularBuffers;
	UInt32 samplesToTrack;
	
	void setCircularBufferCount(UInt32 bufferCount) {
		for(int i = 0; i < circularBuffers.size(); i++) {
			TPCircularBufferCleanup(&circularBuffers[i]);
		}
		
		circularBuffers.resize(bufferCount);
		
		for(int i = 0; i < circularBuffers.size(); i++) {
			TPCircularBufferInit(&circularBuffers[i], samplesToTrack * sizeof(AudioUnitSampleType));
		}
	}
};

struct Tap::TapImpl
{
	TapContext ctx;
};

Tap::Tap(unsigned int samplesToTrack) : _impl(new TapImpl)
{
	_impl->ctx.samplesToTrack = samplesToTrack;
	_impl->ctx.sourceType = TapSourceNone;
	// TODO: allow non-0 source bus
	_impl->ctx.sourceBus  = 0;
}

Tap::~Tap()
{
	for(int i = 0; i < _impl->ctx.circularBuffers.size(); i++) {
		TPCircularBufferCleanup(&_impl->ctx.circularBuffers[i]);
	}
}

#pragma mark - Connections

GenericUnit& Tap::connectTo(GenericUnit &destination, UInt32 destinationBus, UInt32 sourceBus)
{
	if(_impl->ctx.sourceType == TapSourceNone || !_impl->ctx.sourceUnit) {
		std::cout << "Tap can't be connected without a source" << std::endl;
		AURenderCallbackStruct silentCallback = {SilentRenderCallback};
		destination.setRenderCallback(silentCallback);
		return destination;
	}
	
	// connect as normal, in case there are expected side effects
//	_impl->ctx.sourceUnit->connectTo(destination, destinationBus, sourceBus);
	
	AURenderCallbackStruct callback = {RenderAndCopy, &_impl->ctx};
	destination.setRenderCallback(callback, destinationBus);
	return destination;
}

void Tap::setSource(GenericUnit * source)
{
	_impl->ctx.sourceUnit = source;
	_impl->ctx.sourceType = TapSourceUnit;
	
	AudioStreamBasicDescription ASBD = {0};
	UInt32 ASBD_size = sizeof(ASBD);
	
	PRINT_IF_ERR(AudioUnitGetProperty(source->getUnit(),
									  kAudioUnitProperty_StreamFormat,
									  kAudioUnitScope_Output,
									  0,
									  &ASBD,
									  &ASBD_size),
				 "getting tap source's ASBD");
	
	_impl->ctx.setCircularBufferCount(ASBD.mChannelsPerFrame);
}

void Tap::setSource(AURenderCallbackStruct callback, UInt32 channels)
{
	_impl->ctx.sourceCallback = callback;
	_impl->ctx.sourceType = TapSourceCallback;
	
	_impl->ctx.setCircularBufferCount(channels);
}

#pragma mark - Getting samples

void ExtractSamplesFromCircularBuffer(TapSampleBuffer &outBuffer, TPCircularBuffer * circularBuffer)
{
	if(circularBuffer == nullptr) {
		outBuffer.clear();
	} else {
		int32_t circBufferSize;
		AudioUnitSampleType * circBufferTail = (AudioUnitSampleType *)TPCircularBufferTail(circularBuffer, &circBufferSize);
		AudioUnitSampleType * circBufferHead = circBufferTail + (circBufferSize / sizeof(AudioUnitSampleType));
		
		outBuffer.assign(circBufferTail, circBufferHead);
	}
}

void Tap::getSamples(TapSampleBuffer &buffer)
{
	ExtractSamplesFromCircularBuffer(buffer, &_impl->ctx.circularBuffers.front());
}

void Tap::getSamples(std::vector<TapSampleBuffer> &buffers)
{
	const size_t buffersToCopy = min(buffers.size(), _impl->ctx.circularBuffers.size());
	
	for(int i = 0; i < buffersToCopy; i++) {
		ExtractSamplesFromCircularBuffer(buffers[i], &_impl->ctx.circularBuffers[i]);
	}
}

#pragma mark - Render callbacks

inline void CopyAudioBufferIntoCircularBuffer(TPCircularBuffer * circBuffer, const AudioBuffer &audioBuffer)
{
	int32_t availableBytesInCircBuffer;
	TPCircularBufferHead(circBuffer, &availableBytesInCircBuffer);
	
	if(availableBytesInCircBuffer < audioBuffer.mDataByteSize) {
		TPCircularBufferConsume(circBuffer, audioBuffer.mDataByteSize - availableBytesInCircBuffer);
	}

	TPCircularBufferProduceBytes(circBuffer, audioBuffer.mData, audioBuffer.mDataByteSize);
}

OSStatus RenderAndCopy(void * inRefCon,
					   AudioUnitRenderActionFlags * ioActionFlags,
					   const AudioTimeStamp * inTimeStamp,
					   UInt32 inBusNumber,
					   UInt32 inNumberFrames,
					   AudioBufferList * ioData)
{
	TapContext * ctx = static_cast<TapContext *>(inRefCon);
	
	OSStatus status;
	
	if(ctx->sourceType == TapSourceUnit && ctx->sourceUnit->getUnit()) {
		status = ctx->sourceUnit->render(ioActionFlags, inTimeStamp, ctx->sourceBus, inNumberFrames, ioData);
	} else if(ctx->sourceType == TapSourceCallback) {
		status = (ctx->sourceCallback.inputProc)(ctx->sourceCallback.inputProcRefCon,
												 ioActionFlags,
												 inTimeStamp,
												 ctx->sourceBus,
												 inNumberFrames,
												 ioData);
	} else {
		// if we don't have a source, render silence (or else you'll get an extremely loud
		// buzzing noise when we attempt to render a NULL unit. Ow.)
		status = SilentRenderCallback(inRefCon, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
	}
	
	if(status == noErr) {
		const size_t buffersToCopy = min(ctx->circularBuffers.size(), ioData->mNumberBuffers);
		
		for(int i = 0; i < buffersToCopy; i++) {
			CopyAudioBufferIntoCircularBuffer(&ctx->circularBuffers[i], ioData->mBuffers[i]);
		}
	}
	
	return status;
}

OSStatus SilentRenderCallback(void * inRefCon,
							  AudioUnitRenderActionFlags * ioActionFlags,
							  const AudioTimeStamp * inTimeStamp,
							  UInt32 inBusNumber,
							  UInt32 inNumberFrames,
							  AudioBufferList * ioData)
{
	for(int i = 0; i < ioData->mNumberBuffers; i++) {
		memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[0].mDataByteSize);
	}
	
	*ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
	
	return noErr;
}
