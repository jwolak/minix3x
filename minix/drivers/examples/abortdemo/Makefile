# Makefile for the abortdemo example service.
PROG=	abortdemo
SRCS=	abortdemo.c

FILES=${PROG}.conf
FILESNAME=${PROG}
FILESDIR= /etc/system.conf.d

DPADD+=	${LIBSYS}
LDADD+=	-lsys

.include <minix.service.mk>
