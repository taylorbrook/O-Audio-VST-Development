/*
 * gsm_reset.c -- Ouaricon addition, NOT part of upstream libgsm 1.0.22.
 *
 * Returns an existing state to exactly what gsm_create() leaves it in, with
 * no allocation, so O-Bitrot's AudioProcessor::reset() can clear the codec's
 * LPC/LTP history on the audio thread (v1.17.1, CODE_REVIEW WR-02). The body
 * is gsm_create()'s own initialisation, verbatim, minus the malloc.
 */

#include	"config.h"

#include	<string.h>

#include	"gsm.h"
#include	"private.h"

void ouaricon_gsm_reset (gsm r)
{
	if (!r) return;

	memset((char *)r, 0, sizeof(*r));
	r->nrp = 40;
}
