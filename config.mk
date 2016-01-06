NAME = dwmstatus
VERSION = 1.0

# Customize below to fit your system

# paths
PREFIX = /usr
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 -lasound

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\"
#CFLAGS = -march=x86-64 -mtune=native -g -std=c99 -pedantic -Wall -O0 ${INCS} ${CPPFLAGS}
CFLAGS = -march=corei7-avx -mtune=corei7-avx -O2 -pipe -fstack-protector --param=ssp-buffer-size=4 ${INCS} ${CPPFLAGS}

#CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -g ${LIBS}
#LDFLAGS = -s ${LIBS}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}

# compiler and linker
CC = cc
