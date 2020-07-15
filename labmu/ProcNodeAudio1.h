/*
 * ProcNodeAudio1.h
 *
 *  Created on: Nov 1, 2010
 *      Author: mateo
 */

#ifndef PROCNODEAUDIO1_H_
#define PROCNODEAUDIO1_H_

#include "ServerObject.h"


class ProcNodeAudio1: public ProcNode
{
public:
		 ProcNodeAudio1(std::string name, ns__OBJECT_ID parentID);
virtual ~ProcNodeAudio1();

		// step
		bool	step();
};

#endif /* PROCNODEAUDIO1_H_ */
