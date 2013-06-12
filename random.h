//
//  random.h
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#ifndef _RANDOM_H
#define _RANDOM_H

#include "geometry.h"

static float randomf(random_state_t *state)
{
	union {
		float f;
		unsigned int ui;
	} ret;

	const uint a = 22222;
	const uint b = 55555;
	*state = (random_state_t)(a * ((state->x) & 0xFFFF) + ((state->x) >> 16), b * ((state->y) & 0xFFFF) + ((state->y) >> 16));
	
	ret.ui = (((state->x) << 16) + (state->y) & 0x007fffff) | 0x40000000;
	
	return (ret.f - 2.f) / 2.f;
}

#endif