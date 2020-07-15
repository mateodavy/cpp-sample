/*
 * ProcNodeAudio1.cpp
 *
 *  Created on: Nov 1, 2010
 *      Author: mateo
 */

#include "ProcNodeAudio1.h"
#include "ServerRuntime.h"
#include "labmuService.h"

#include "SoundMix.h"


#define PIN_AUDIO_INPUT_12			0
#define PIN_AUDIO_INPUT_34			1
#define PIN_AUDIO_INPUT_TAPE		2
#define PIN_AUDIO_OUTPUT_MAIN		3
#define PIN_AUDIO_OUTPUT_ROOM		4

#define PARAM_GAIN_12				"gain12"
#define PARAM_PAN_12				"pan12"
#define PARAM_GAIN_34				"gain34"
#define PARAM_PAN_34				"pan34"
#define PARAM_GAIN_MAIN				"gainMain"
#define PARAM_GAIN_ROOM				"gainRoom"
#define PARAM_TAPE_TO_MIX_SELECT	"tapeToMix"
#define PARAM_TAPE_TO_ROOM_SELECT	"tapeToRoom"


ProcNodeAudio1::ProcNodeAudio1(std::string name, ns__OBJECT_ID parentID) : ProcNode(name, parentID)
{
}

ProcNodeAudio1::~ProcNodeAudio1()
{
}


// step
bool ProcNodeAudio1::step()
{
	// global track
	getInstance()->getTimeTrack().push("ProcNodeAudio1::step");

	// get params
	Parameter* gain12Param = getParameter(PARAM_GAIN_12);
	Parameter* pan12Param = getParameter(PARAM_PAN_12);
	Parameter* gain34Param = getParameter(PARAM_GAIN_34);
	Parameter* pan34Param = getParameter(PARAM_PAN_34);
	Parameter* gainMainParam = getParameter(PARAM_GAIN_MAIN);
	Parameter* gainRoomParam = getParameter(PARAM_GAIN_ROOM);
	Parameter* tapeToMixSelectParam = getParameter(PARAM_TAPE_TO_MIX_SELECT);
	Parameter* tapeToRoomSelectParam = getParameter(PARAM_TAPE_TO_ROOM_SELECT);

	// checks
	if ((gain12Param == NULL) ||
		(pan12Param == NULL) ||
		(gain34Param == NULL) ||
		(pan34Param == NULL) ||
		(gainMainParam == NULL) ||
		(gainRoomParam == NULL) ||
		(tapeToMixSelectParam == NULL) ||
		(tapeToRoomSelectParam == NULL))
	{
			getInstance()->getTimeTrack().pop();
			return false;
	}

	// get inputs
	AudioBufferPool::Buffer* pAudio12Buffer = getAudioBuffer(PIN_AUDIO_INPUT_12);
	AudioBufferPool::Buffer* pAudio34Buffer = getAudioBuffer(PIN_AUDIO_INPUT_34);
	AudioBufferPool::Buffer* pAudioTapeBuffer = getAudioBuffer(PIN_AUDIO_INPUT_TAPE);

	// get outputs
	AudioBufferPool::Buffer* pAudioMainBuffer = ServerRuntime::getFreeAudioBuffer();
	AudioBufferPool::Buffer* pAudioTempBuffer = ServerRuntime::getFreeAudioBuffer();
	if ((pAudioMainBuffer == NULL) || (pAudioTempBuffer == NULL))
	{
		printf("ProcNodeAudio1::step: failed to get free buffers for processing...\n");
		getInstance()->getTimeTrack().pop();
		return false;
	}

	// temp mix (use for monitoring)
	pAudioTempBuffer->_data->_sound->clear();
	if (pAudio12Buffer != NULL)
	{
		SoundMix::add(*pAudioTempBuffer->_data->_sound, *pAudio12Buffer->_data->_sound, gain12Param->getAsFloat(), pan12Param->getAsFloat());
		pAudioTempBuffer->setDebug(pAudio12Buffer->getDebug());
	}
	if (pAudio34Buffer != NULL)
	{
		SoundMix::add(*pAudioTempBuffer->_data->_sound, *pAudio34Buffer->_data->_sound, gain34Param->getAsFloat(), pan34Param->getAsFloat());
		pAudioTempBuffer->setDebug(pAudioTempBuffer->getDebug() || pAudio34Buffer->getDebug());
	}
	if ((pAudioTapeBuffer != NULL) && tapeToMixSelectParam->getAsBoolean())
		SoundMix::add(*pAudioTempBuffer->_data->_sound, *pAudioTapeBuffer->_data->_sound, 0.0f, 0.0f);

	// main output (apply final gain)
	pAudioMainBuffer->_data->_sound->clear();
	SoundMix::add(*pAudioMainBuffer->_data->_sound, *pAudioTempBuffer->_data->_sound, gainMainParam->getAsFloat(), 0.0f);
	pAudioMainBuffer->setDebug(pAudioTempBuffer->getDebug());
	setAudioBuffer(PIN_AUDIO_OUTPUT_MAIN, pAudioMainBuffer);

#ifdef BYPASS_BUFFERS
	// copy (bypass for now)
	if (pAudio12Buffer != NULL)
		pAudio12Buffer->incrementRefCount();
	setAudioBuffer(PIN_AUDIO_OUTPUT_MAIN, pAudio12Buffer);
	if (pAudio34Buffer != NULL)
		pAudio34Buffer->incrementRefCount();
	setAudioBuffer(PIN_AUDIO_OUTPUT_TAPE, pAudio34Buffer);
#endif

	// room output
	AudioBufferPool::Buffer* pAudioRoomBuffer = ServerRuntime::getFreeAudioBuffer();
	if (pAudioRoomBuffer == NULL)
	{
		printf("ProcNodeAudio1::step: failed to get free buffers for room output...\n");
		getInstance()->getTimeTrack().pop();
		return false;
	}
	pAudioRoomBuffer->setDebug(pAudioMainBuffer->getDebug());
	pAudioRoomBuffer->_data->_sound->clear();
	if (tapeToRoomSelectParam->getAsBoolean())
	{
		if (pAudioTapeBuffer != NULL)
			SoundMix::add(*pAudioRoomBuffer->_data->_sound, *pAudioTapeBuffer->_data->_sound, gainRoomParam->getAsFloat(), 0.0f);
	}
	else
	{
		if (pAudioMainBuffer != NULL)
			SoundMix::add(*pAudioRoomBuffer->_data->_sound, *pAudioMainBuffer->_data->_sound, gainRoomParam->getAsFloat(), 0.0f);
	}
	/* only increment if source pointer is external
	if (pAudioRoomBuffer != NULL)
		pAudioRoomBuffer->incrementRefCount();
	*/
	setAudioBuffer(PIN_AUDIO_OUTPUT_ROOM, pAudioRoomBuffer);

	// release
	pAudioTempBuffer->release();

	// done
	getInstance()->getTimeTrack().pop();
	return true;
}
