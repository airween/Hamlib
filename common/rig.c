/* hamlib - Ham Radio Control Libraries
   Copyright (C) 2000 Stephane Fillod
   This file is part of the hamlib package.

   Hamlib is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   Hamlib is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with sane; see the file COPYING.  If not, write to the Free
   Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "rig.h"
#include "riglist.h"
#include "serial.h"


#define DEFAULT_SERIAL_PORT "/dev/ttyS0"

/*
 * It would be nice to have an automatic way of referencing all the backends
 * supported by hamlib. Maybe this array should be placed in a separate file..
 * 
 * The rig_base is a variable length rig_caps* array, NULL terminated
 */

static const struct rig_caps *rig_base[] = { 
	&ft747_caps, &ic706_caps, &ic706mkiig_caps, /* ... */ NULL, };

static const char *rigerror_table[] = {
		"Command completed sucessfully",
		"Invalid parameter",
		"Invalid configuration",
		"Memory shortage",
		"Feature not implemented",
		"Communication timed out",
		"IO error",
		"Internal Hamlib error",
		"Protocol error",
		"Command rejected by the rig",
		"Command performed, but arg truncated, result not guaranteed"
};

/*
 * TODO: check table bounds, use gettext
 */
const char *rigerror(int errnum)
{
			return rigerror_table[errnum];
}


RIG *rig_init(rig_model_t rig_model)
{
		RIG *rig;
		int i;

		/* lookup for this rig */
		for (i=0; rig_base[i]; i++) {
				if (rig_base[i]->rig_model == rig_model)
						break;
		}
		if (rig_base[i] == NULL) {
				/* End of list, rig not supported, sorry! */
				return NULL;
		}

		/*
		 * okay, we've found it. Allocate some memory and set it to zeros,
		 * and especially the initialize the callbacks 
		 */ 
		rig = calloc(1, sizeof(RIG));
		if (rig == NULL) {
				/*
				 * FIXME: how can the caller know it's a memory shortage,
				 * 		  and not "rig not found" ?
				 */
				return NULL;
		}

		/* remember, rig->caps is readonly */
		rig->caps = rig_base[i];

		/*
		 * populate the rig->state
		 * TODO: read the Preferences here! 
		 */

		rig->state.port_type = RIG_PORT_SERIAL; /* default is serial port */
		strncpy(rig->state.rig_path, DEFAULT_SERIAL_PORT, FILPATHLEN);
		rig->state.port_type = RIG_PORT_SERIAL; /* default is serial port */
		rig->state.serial_rate = rig->caps->serial_rate_max;	/* fastest ! */
		rig->state.serial_data_bits = rig->caps->serial_data_bits;
		rig->state.serial_stop_bits = rig->caps->serial_stop_bits;
		rig->state.serial_parity = rig->caps->serial_parity;
		rig->state.serial_handshake = rig->caps->serial_handshake;
		rig->state.timeout = rig->caps->timeout;
		rig->state.retry = rig->caps->retry;
		rig->state.ptt_type = rig->caps->ptt_type;
		rig->state.vfo_comp = 0.0;	/* override it with preferences */

		/* 
		 * let the backend a chance to setup his private data
		 * FIXME: check rig_init() return code
		 */
		if (rig->caps->rig_init != NULL)
				rig->caps->rig_init(rig);	

		return rig;
}

int rig_open(RIG *rig)
{
		int status;

		if (!rig)
				return -RIG_EINVAL;

		switch(rig->state.port_type) {
		case RIG_PORT_SERIAL:
				status = serial_open(&rig->state);
				if (status != 0)
						return status;
				break;

		case RIG_PORT_NETWORK:	/* not implemented yet! */
		default:
				return -RIG_ENIMPL;
		}

		/* 
		 * Maybe the backend has something to initialize
		 * FIXME: check rig_open() return code
		 */
		if (rig->caps->rig_open != NULL)
				rig->caps->rig_open(rig);	

		return RIG_OK;
}

/*
 * Examples of typical rig_* wrapper
 */

/*
 * cmd_set_freq
 *
 */

int rig_set_freq(RIG *rig, freq_t freq)
{
		if (!rig || !rig->caps)
			return -RIG_EINVAL;

		if (rig->state.vfo_comp != 0.0)
				freq = (freq_t)(rig->state.vfo_comp * freq);

		if (rig->caps->set_freq == NULL)
			return -RIG_ENIMPL;	/* not implemented */
		else
			return rig->caps->set_freq(rig, freq);
}

/*
 * cmd_get_freq
 *
 */

int rig_get_freq(RIG *rig, freq_t *freq)
{
		if (!rig || !rig->caps || !freq)
			return -RIG_EINVAL;

		if (rig->caps->get_freq == NULL)
			return -RIG_ENIMPL;	/* not implemented */
		else
			return rig->caps->get_freq(rig, freq);
}


/*
 * cmd_set_mode
 *
 */

int rig_set_mode(RIG *rig, rmode_t mode)
{
		if (!rig || !rig->caps)
			return -RIG_EINVAL;

		if (rig->caps->set_mode == NULL)
			return -RIG_ENIMPL;	/* not implemented */
		else
			return rig->caps->set_mode(rig, mode);
}

/*
 * cmd_get_mode
 *
 */

int rig_get_mode(RIG *rig, rmode_t *mode)
{
		if (!rig || !rig->caps || !mode)
			return -RIG_EINVAL;

		if (rig->caps->get_mode == NULL)
			return -RIG_ENIMPL;	/* not implemented */
		else
			return rig->caps->get_mode(rig, mode);
}


/*
 * cmd_set_vfo
 *
 */

int rig_set_vfo(RIG *rig, vfo_t vfo)
{
		if (!rig || !rig->caps)
			return -RIG_EINVAL;

		if (rig->caps->set_vfo == NULL)
			return -RIG_ENIMPL;	/* not implemented */
		else
			return rig->caps->set_vfo(rig, vfo);
}

/*
 * cmd_get_vfo
 *
 */

int rig_get_vfo(RIG *rig, vfo_t *vfo)
{
		if (!rig || !rig->caps || !vfo)
			return -RIG_EINVAL;

		if (rig->caps->get_vfo == NULL)
			return -RIG_ENIMPL;	/* not implemented */
		else
			return rig->caps->get_vfo(rig, vfo);
}


/*
 * more rig_* to come -- FS
 *
 */



/*
 * Close port
 */
int rig_close(RIG *rig)
{
		if (rig == NULL || rig->caps)
				return -RIG_EINVAL;

		/*
		 * Let the backend say 73s to the rig
		 */
		if (rig->caps->rig_close)
				rig->caps->rig_close(rig);

		if (rig->state.fd != -1) {
				close(rig->state.fd);
				rig->state.fd = -1;
		}

		return RIG_OK;
}

/*
 * Release a rig struct which port has already been closed
 */
int rig_cleanup(RIG *rig)
{
		if (rig == NULL || rig->caps)
				return -RIG_EINVAL;

		/*
		 * basically free up the priv struct 
		 */
		if (rig->caps->rig_cleanup)
				rig->caps->rig_cleanup(rig);

		free(rig);

		return RIG_OK;
}




/* CAUTION: this is really Experimental, It never worked!!
 * try to guess a rig, can be very buggy! (but fun if it works!)
 * FIXME: finish me and handle nicely errors
 */
RIG *rig_probe(const char *port_path)
{
		RIG *rig;
		int i;

		for (i = 0; rig_base[i]; i++) {
			if (rig_base[i]->rig_probe != NULL) {
				rig = rig_init(rig_base[i]->rig_model);
				strncpy(rig->state.rig_path, port_path, FILPATHLEN);
				rig_open(rig);
				if (rig && rig_base[i]->rig_probe(rig) == 0) {
					return rig;
				} else {
					rig_close(rig);
					rig_cleanup(rig);
				}
			}
		}
		return NULL;
}


/*
 * "macro" to check if a rig has a function,
 * example:  if (rig_has_func(my_rig, RIG_FUNC_FAGC)) disp_fagc_button();
 */
int rig_has_func(RIG *rig, unsigned long func)
{
		if (!rig || !rig->caps)
				return -1;

		return (rig->caps->has_func & func);
}


/*
 * Gets rig capabilities.
 * 
 */

const struct rig_caps *rig_get_caps(rig_model_t rig_model) {

	int i;
	const struct rig_caps *rc;

	/* lookup for this rig */
	for (i=0; rig_base[i]; i++) {
			if (rig_base[i]->rig_model == rig_model)
					break;
	}
	if (rig_base[i] == NULL) {
			/* rig not supported, sorry! */
			return NULL;
	}

	rc = rig_base[i];
	printf("rig = %s \n", rc->model_name);

	printf("rig serial_rate_min = = %u \n", rc->serial_rate_min);
	printf("rig serial_rate_max = = %u \n", rc->serial_rate_max);

	return  rc;
}

