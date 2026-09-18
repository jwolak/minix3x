/*-
 * BSD 3-Clause License
 *
 * Copyrights 2026, Janusz Wolak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <minix/config.h>
#include <minix/sysutil.h>

#include "announce.h"

void display_minix_startup_banner(struct announceType *self)
{
	(void)self;
	/* Display the MINIX startup banner. */
	printf("\nMINIX %s. "
#ifdef PAE
	       "(PAE) "
#endif
#ifdef _VCS_REVISION
	       "(" _VCS_REVISION ")\n"
#endif
	       "Copyright 2016, Vrije Universiteit, Amsterdam, The Netherlands\n",
	       OS_RELEASE);
	printf("%s", "MINIX is open source software, see http://www.minix3.org\n");
}

static struct announceType newAnnounce(void)
{
	struct announceType announce_;
	announce_.display_minix_startup_banner = display_minix_startup_banner;

	return announce_;
}

const struct announceClass announceType = {.new = newAnnounce};