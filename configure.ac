#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Get version from git
m4_define([pkg_version], m4_esyscmd([sh build-aux/git-version-gen]))
m4_ifset([pkg_version], [], [ m4_define([pkg_version], [UNKNOWN]) ])
m4_if(pkg_version, [UNKNOWN],
    [ m4_fatal([unable to obtain package version; please check `git-version-gen' output]) ])

#
# Autotools setup
#
AC_PREREQ([2.60])
AC_INIT([Astra SM], [pkg_version])

AC_CONFIG_SRCDIR([src/astra.h])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AC_CANONICAL_SYSTEM
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.11 foreign dist-xz no-dist-gzip subdir-objects nostdinc -Wall -Werror])
AM_MAINTAINER_MODE([enable])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_PRESERVE_HELP_ORDER

# Checks for programs
AC_LANG(C)
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PROG_SED

# Initialize libtool
LT_INIT([disable-shared static no-pic])
LT_LIB_DLLOAD
LT_LIB_M

# Set build compiler options
AX_PROG_CC_FOR_BUILD
AM_PROG_CC_C_O

# Check C99/C++11 support
compiler_lang=""

case "${host_os}" in
    mingw*|cygwin*)
        AX_CHECK_CFLAG([-std=gnu99], [], [compiler_lang=C], [
            AX_CHECK_CFLAG([-std=gnu++11], [], [compiler_lang=C++]) ])
        ;;

    *)
        AX_CHECK_CFLAG([-std=c99], [], [compiler_lang=C], [
            AX_CHECK_CFLAG([-std=iso9899:1999], [], [compiler_lang=C], [
                AX_CHECK_CFLAG([-std=c++11], [], [compiler_lang=C++]) ]) ])
        ;;
esac

AS_IF([test -z "${compiler_lang}"], [
    AC_MSG_ERROR([$CC doesn't support C nor C++]) ])

AS_IF([test "x${compiler_lang}" = "xC++"], [
    AX_CHECK_CFLAGS([-fkeep-inline-functions])
])

#
# Checks for typedefs, structures, and compiler characteristics
#
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT64_T
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_TYPES([_Bool])

# in case endian.h is absent
AC_MSG_CHECKING([for endian.h presence and usability])
AC_PREPROC_IFELSE([
    AC_LANG_PROGRAM([
        #include <endian.h>
        #include <sys/types.h>
        #if (!defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN) || !defined(BIG_ENDIAN))
        #   error "BYTE_ORDER defines absent"
        #endif
        #if (BYTE_ORDER != LITTLE_ENDIAN && BYTE_ORDER != BIG_ENDIAN)
        #   error "BYTE_ORDER has invalid value"
        #endif
    ])
], [
    AC_MSG_RESULT([yes])
], [
    AC_MSG_RESULT([no])

    # let's make our own!
    AC_DEFINE([LITTLE_ENDIAN], [1234], [Little endian byte order])
    AC_DEFINE([BIG_ENDIAN], [4321], [Big endian byte order])

    AC_C_BIGENDIAN([ac_cv_c_bigendian="yes"], [ac_cv_c_bigendian="no"], [], [
        AC_MSG_ERROR([universal endianness not supported; please preset ac_cv_c_bigendian=no]) ])

    AS_IF([test "x${ac_cv_c_bigendian}" = "xyes"], [
        AC_DEFINE([BYTE_ORDER], [BIG_ENDIAN],
            [Define this machine's endianness])
    ], [
        AC_DEFINE([BYTE_ORDER], [LITTLE_ENDIAN],
            [Define this machine's endianness])
    ])
])

# use 64 bit file offsets on 32 bit systems
AC_SYS_LARGEFILE

#
# Platform-dependent options
#
case "${host_os}" in
    mingw*)
        SYS=win32

        AC_DEFINE([_WIN32_WINNT],
            [0x0501], [Require at least Windows XP])
        # TODO: add option to set winver
        #    WSAPoll() needs at least 0x0600

        # check winmm (needed for timer resolution APIs)
        AC_CHECK_LIB([winmm], [main],
            [], AC_MSG_ERROR([cannot link with winmm.dll]) )

        # check winsock
        AC_CHECK_LIB([ws2_32], [main],
            [], AC_MSG_ERROR([cannot link with ws2_32.dll]) )

        AC_CHECK_DECL([WSAStartup], [], [
            AC_MSG_ERROR([missing required library function])
        ], [[
            #include <winsock2.h>
        ]])
    ;;

    *)
        SYS=posix

        AC_CHECK_DECLS([environ])
        AC_CHECK_MEMBERS([struct sockaddr_in.sin_len], [], [], [[
            #include <sys/types.h>
            #include <netinet/in.h>
        ]])

        AX_PTHREAD([
            LIBS="$PTHREAD_LIBS $LIBS"
            CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
            CC="$PTHREAD_CC"
        ], [
            AC_MSG_ERROR([missing pthread support])
        ])
    ;;
esac

AM_CONDITIONAL(HAVE_WIN32, [test "${SYS}" = "win32"])
AM_CONDITIONAL(HAVE_POSIX, [test "${SYS}" = "posix"])

#
# Choose event mechanism
#
event_mechanism=""
# TODO: add option to set this manually

# epoll (Linux-specific)
AS_IF([test "x${event_mechanism}" = "x"], [
    epoll_failed="no"
    AC_CHECK_HEADER([sys/epoll.h], [
        AC_CHECK_FUNCS([epoll_create epoll_ctl epoll_wait], [],
            [ epoll_failed="yes" ])
    ], [
        epoll_failed="yes"
    ])
    AS_IF([test "x${epoll_failed}" = "xno"], [
        event_mechanism="epoll"
        AC_CHECK_FUNCS([epoll_create1])
        AC_DEFINE([WITH_EPOLL],
            [1], [Define to use epoll for event notification])
    ])
])

# kqueue (various BSD)
AS_IF([test "x${event_mechanism}" = "x"], [
    kqueue_failed="no"
    AC_CHECK_HEADER([sys/event.h], [
        AC_CHECK_FUNCS([kqueue kevent], [],
            [ kqueue_failed="yes" ])
    ], [
        kqueue_failed="yes"
    ])
    AS_IF([test "x${kqueue_failed}" = "xno"], [
        event_mechanism="kqueue"
        AC_CHECK_FUNCS([kqueue1])
        AC_DEFINE([WITH_KQUEUE],
            [1], [Define to use kqueue for event notification])
    ])
])

# poll
AS_IF([test "x${event_mechanism}" = "x"], [
    poll_failed="no"
    AC_CHECK_HEADERS([poll.h])
    AC_CHECK_FUNCS([poll], [], [
        # Windows Vista introduced its own poll() version
        AC_CHECK_DECL([WSAPoll], [
            AC_DEFINE([HAVE_WSAPOLL],
                [1], [Define to 1 if you have the `WSAPoll' function.])
        ], [
            poll_failed="yes"
        ], [[
            #include <winsock2.h>
        ]])
    ])
    AS_IF([test "x${poll_failed}" = "xno"], [
        event_mechanism="poll"
        AC_DEFINE([WITH_POLL],
            [1], [Define to use poll() for event notification])
    ])
])

# select
AS_IF([test "x${event_mechanism}" = "x"], [
    # no need to check the function, it's present on every system
    AC_CHECK_HEADERS([sys/select.h])
    event_mechanism="select"
    AC_DEFINE([WITH_SELECT],
        [1], [Define to use select() for event notification])
])

AC_MSG_NOTICE([using ${event_mechanism} for event notification])

#
# Checks for headers and functions common to all platforms
#
# optional headers
AC_CHECK_HEADERS([netinet/sctp.h sys/queue.h])

# optional functions
#   pread(), strndup(), strnlen(): replaceables
#   posix_memalign(): used by stream/file
#   accept4(): used by core/socket
#   mkostemp(): used for creating pidfiles
AC_CHECK_FUNCS([pread strndup strnlen posix_memalign accept4 mkostemp pthread_mutex_timedlock])

# getifaddrs(): used by utils.c
AC_CHECK_FUNCS([getifaddrs],
    [], [ AC_MSG_WARN([no getifaddrs(); utils.ifaddrs() will be unavailable]) ])

# clock_gettime(): used by clock.c
LIBRT=""
ac_cv_func_clock_gettime="no"
AX_SAVE_FLAGS
AC_SEARCH_LIBS([clock_gettime], [rt], [
    AS_IF([test "$LIBS" != "$SAVED_LIBS"], [ LIBRT="-lrt" ])

    ac_cv_func_clock_gettime="yes"
    AC_DEFINE([HAVE_CLOCK_GETTIME],
        [1], [Define to 1 if you have the `clock_gettime' function.])
])
AX_RESTORE_FLAGS

# POSIX AIO: used by `file'
have_aio="no"
AX_SAVE_FLAGS
AC_CHECK_HEADERS([aio.h], [
    AC_SEARCH_LIBS([aio_write], [rt], [
        AS_IF([test "$LIBS" != "$SAVED_LIBS"], [ LIBRT="-lrt" ])

        have_aio="yes"
        AC_DEFINE([HAVE_AIO], [1],
            [Define to 1 if you have the POSIX AIO interface.])
    ])
])
AX_RESTORE_FLAGS
AC_SUBST(LIBRT)

#
# Checks for external libraries
#
AX_HELP_SECTION([External libraries:])

# lua
AX_EXTLIB_PARAM(lua,
    [use external Lua library instead of the bundled version])

AC_ARG_VAR([LUAC], [Lua compiler command])
AC_ARG_WITH([lua-compiler],
    AC_HELP_STRING([--with-lua-compiler=PATH], [path to Lua compiler (luac)]))

AS_IF([test "x${with_lua_compiler}" = "xyes" -o -z "${with_lua_compiler}"], [
    AS_IF([test -z "${LUAC}"], [
        # default value
        LUAC="luac luac53 luac52"
    ])
], [
    AS_IF([test "x${with_lua_compiler}" != "xno"], [
        # custom compiler path
        LUAC="${with_lua_compiler}"
    ], [
        # disabled explicitly
        LUAC=""
    ])
])

have_lua="no"
AS_IF([test "x${with_lua}" != "xno"], [
    AX_SAVE_FLAGS
    CFLAGS="$CFLAGS $LUA_CFLAGS"
    # lua wants -lm on linux
    LIBS="$LIBS $LIBM $LUA_LIBS"
    AC_CHECK_HEADERS([lua.h], [
        AC_SEARCH_LIBS([luaL_len], [lua lua53 lua5.3 lua-5.3 lua52 lua5.2 lua-5.2], [
            have_lua="yes"
            LUA_LIBS="${LUA_LIBS} ${LIBM} ${LIBADD_DLOPEN} ${ac_res}"

            AS_IF([test "x${with_lua_compiler}" != "xno"], [
                AC_PATH_PROGS([LUAC], [${LUAC}])
                AS_IF([test -z "${LUAC}"], [
                    AC_MSG_ERROR([Couldn't find Lua compiler; pass --without-lua-compiler to disable this check]) ])
            ])
        ])
    ])
    AX_RESTORE_FLAGS
    AS_IF([test "x${have_lua}" = "xno"], [
        AC_MSG_WARN([Astra requires Lua library version 5.2 or higher])
        AS_IF([test "x${with_lua}" != "xyes"], [
            AC_MSG_WARN([Bundled version will be built and statically linked])
        ], [
            AC_MSG_ERROR([Pass --without-lua to build bundled version])
        ])
    ])
])

AS_IF([test "x${have_lua}" = "xno"], [
    LUA_CFLAGS="-I\$(top_srcdir)/lua"
    LUA_LIBS="\$(top_builddir)/lua/liblua.la"

    AS_IF([test "x${with_lua_compiler}" != "xno"], [
        LUAC="\$(top_builddir)/lua/luac\$(EXEEXT)"
    ])
])

AS_IF([test "x${cross_compiling}" = "xyes" -a -n "${LUAC}"], [
    AS_IF([test -z "${with_lua_compiler}"], [
        AC_MSG_WARN([Cross compilation detected; disabling Lua compiler])
        LUAC=""
    ], [
        AC_MSG_ERROR([Cannot cross-compile Lua scripts; pass --without-lua-compiler to disable this check])
    ])
])

AM_CONDITIONAL([HAVE_INTERNAL_LUA], [test "x${have_lua}" = "xno"])
AM_CONDITIONAL([HAVE_LUA_COMPILER], [test -n "${LUAC}"])

# ffmpeg
AC_ARG_VAR([FFMPEG], [path to ffmpeg program])
AC_ARG_WITH([ffmpeg],
    AC_HELP_STRING([--with-ffmpeg=PATH], [path to ffmpeg program]))

AS_IF([test "x${with_ffmpeg}" != "xno"], [
    AS_IF([test "x${with_ffmpeg}" = "xyes" -o -z "${with_ffmpeg}"], [
        AS_IF([test -z "${FFMPEG}"], [ FFMPEG="ffmpeg" ])
    ], [
        FFMPEG="${with_ffmpeg}"
    ])
    AC_PATH_PROGS([FFMPEG], [${FFMPEG}])

    AS_IF([test -z "${FFMPEG}"], [
        AS_IF([test -n "${with_ffmpeg}"],
            [ AC_MSG_ERROR([Couldn't find ffmpeg program; pass --without-ffmpeg to disable this check]) ],
            [ AC_MSG_WARN([Couldn't find ffmpeg program; test patterns will not be built]) ]) ])
], [
    FFMPEG=""
])

AM_CONDITIONAL([HAVE_FFMPEG], [test -n "${FFMPEG}"])

# libaio
AX_EXTLIB_PARAM(libaio,
    [enable libaio for file I/O (Linux only)] )

have_libaio="no"
AS_IF([test "x${have_aio}" = "xyes"], [
    AS_IF([test "x${with_libaio}" != "xno"], [
        AX_SAVE_FLAGS
        CFLAGS="$CFLAGS $LIBAIO_CFLAGS"
        LIBS="$LIBS $LIBAIO_LIBS"
        AC_CHECK_HEADERS([libaio.h], [
            AC_CHECK_LIB([aio], [io_queue_init], [
                AC_DEFINE([HAVE_LIBAIO], [1], [Define to 1 if you have libaio])

                have_libaio="yes"
                LIBAIO_LIBS="${LIBAIO_LIBS} -laio"
            ])
        ])
        AX_RESTORE_FLAGS
        AS_IF([test "x${have_libaio}" = "xno"], [
            AS_IF([test "x${with_libaio}" = "xyes"],
                [ AC_MSG_ERROR([could not find libaio; pass --disable-libaio to disable this check]) ],
                [ AC_MSG_WARN([could not find libaio; Linux AIO support will be unavailable]) ]) ])
    ])
], [
    AS_IF([test "x${with_libaio}" = "xyes"], [
        AC_MSG_ERROR([libaio is not supported on this platform]) ])
])
AM_CONDITIONAL([HAVE_LIBAIO], [test "x${have_libaio}" = "xyes"])

# libdvbcsa
AX_EXTLIB_PARAM(dvbcsa,
    [enable CSA (de)scrambling using libdvbcsa])

have_dvbcsa="no"
AS_IF([test "x${with_dvbcsa}" != "xno"], [
    AX_SAVE_FLAGS
    CFLAGS="$CFLAGS $DVBCSA_CFLAGS"
    LIBS="$LIBS $DVBCSA_LIBS"
    AC_CHECK_HEADERS([dvbcsa/dvbcsa.h], [
        AC_CHECK_LIB([dvbcsa], [dvbcsa_bs_key_alloc], [
            AC_DEFINE([HAVE_DVBCSA], [1], [Define to 1 if you have libdvbcsa])

            have_dvbcsa="yes"
            DVBCSA_LIBS="${DVBCSA_LIBS} -ldvbcsa"
        ])
    ])
    AX_RESTORE_FLAGS
    AS_IF([test "x${have_dvbcsa}" = "xno"], [
        AS_IF([test "x${with_dvbcsa}" = "xyes"],
            [ AC_MSG_ERROR([could not find dvbcsa; pass --disable-dvbcsa to disable this check]) ],
            [ AC_MSG_WARN([could not find dvbcsa; CSA support will be unavailable]) ]) ])
])
AM_CONDITIONAL([HAVE_DVBCSA], [test "x${have_dvbcsa}" = "xyes"])

# OpenSSL (libcrypto)
AX_EXTLIB_OPTIONAL(libcrypto,
    [enable linking with OpenSSL crypto library])

AS_IF([test "x${with_libcrypto_includes}" != "xno"],
    [LIBCRYPTO_CFLAGS="-I${with_libcrypto_includes}"])
AS_IF([test "x${with_libcrypto_libs}" != "xno"],
    [LIBCRYPTO_LIBS="-L${with_libcrypto_libs} -lcrypto"])

have_libcrypto="no"
AS_IF([test "x${with_libcrypto}" != "xno"], [
    AX_SAVE_FLAGS
    PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto], [
        CFLAGS="$CFLAGS $LIBCRYPTO_CFLAGS"
        LIBS="$LIBS $LIBCRYPTO_LIBS"
        AC_CHECK_HEADERS([openssl/des.h], [
            AC_CHECK_LIB([crypto], [DES_random_key], [
                AC_DEFINE([HAVE_LIBCRYPTO], [1],
                    [Define to 1 if you have OpenSSL libcrypto])

                have_libcrypto="yes"
            ], [
                # linking check failed
                LIBCRYPTO_PKG_ERRORS="could not link with libcrypto"
            ])
        ], [
            # header check failed
            LIBCRYPTO_PKG_ERRORS="could not find libcrypto headers"
        ])
    ], [
        # pkgconfig call failed
        LIBCRYPTO_PKG_ERRORS="could not find libcrypto; refer to config.log for details"
    ])
    AX_RESTORE_FLAGS
    AS_IF([test "x${have_libcrypto}" = "xno"], [
        AS_IF([test "x${with_libcrypto}" = "xyes"],
            [ AC_MSG_ERROR([${LIBCRYPTO_PKG_ERRORS}]) ],
            [ AC_MSG_WARN([${LIBCRYPTO_PKG_ERRORS}]) ]) ])
])
AM_CONDITIONAL([HAVE_LIBCRYPTO], [test "x${have_libcrypto}" = "xyes"])

# Check
AX_EXTLIB_OPTIONAL(check, [enable unit testing using Check])

AS_IF([test "x${with_check_includes}" != "xno"],
    [CHECK_CFLAGS="-I${with_check_includes}"])
AS_IF([test "x${with_check_libs}" != "xno"],
    [CHECK_LIBS="-L${with_check_libs} -lcheck"])

have_check="no"
AS_IF([test "x${with_check}" != "xno"], [
    AX_SAVE_FLAGS
    PKG_CHECK_MODULES([CHECK], [check], [
        CFLAGS="$CFLAGS $CHECK_CFLAGS"
        AC_CHECK_HEADERS([check.h], [
            have_check="yes"
        ], [
            # header check failed
            CHECK_PKG_ERRORS="could not find check headers"
        ])
    ], [
        # pkgconfig call failed
        CHECK_PKG_ERRORS="could not find check; refer to config.log for details"
    ])
    AX_RESTORE_FLAGS
    AS_IF([test "x${have_check}" = "xno"], [
        AS_IF([test "x${with_check}" = "xyes"],
            [ AC_MSG_ERROR([${CHECK_PKG_ERRORS}]) ],
            [ AC_MSG_WARN([${CHECK_PKG_ERRORS}]) ]) ])
])
AM_CONDITIONAL([HAVE_CHECK], [test "x${have_check}" = "xyes"])

# Linux DVB API
have_dvbapi="no"
AC_MSG_CHECKING([for Linux DVB API version 5 or higher])
AC_PREPROC_IFELSE([
    AC_LANG_PROGRAM([
        #include <sys/ioctl.h>
        #include <linux/dvb/version.h>
        #include <linux/dvb/frontend.h>
        #include <linux/dvb/dmx.h>
        #include <linux/dvb/ca.h>
        #if DVB_API_VERSION < 5
        #error "DVB API is too old"
        #endif
    ])
], [
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_DVBAPI], [1], [Define to 1 if you have a usable DVB API])

    # DVB-T2 (DVB API >= 5.3)
    have_dvbapi_t2="no"
    AC_MSG_CHECKING([for DVB-T2 support])
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([[
            #include <linux/dvb/frontend.h>
        ]], [[
            fe_delivery_system_t sys = SYS_DVBT2;
            fe_transmit_mode_t tm;
            tm = TRANSMISSION_MODE_1K;
            tm = TRANSMISSION_MODE_16K;
            tm = TRANSMISSION_MODE_32K;
        ]])
    ], [
        AC_MSG_RESULT([yes])
        AC_DEFINE([HAVE_DVBAPI_T2], [1], [Define to 1 if your system supports DVB-T2])
        have_dvbapi_t2="yes"
    ], [
        AC_MSG_RESULT([no])
    ])
    AS_IF([test "x${have_dvbapi_t2}" = "xno"], [
        AC_MSG_WARN([DVB API 5.3 or higher required for DVB-T2 support]) ])

    # DVB-C annex A/C defines (DVB API >= 5.6)
    AC_MSG_CHECKING([DVB-C annex A/C definitions])
    AC_COMPILE_IFELSE([
        AC_LANG_PROGRAM([[
            #include <linux/dvb/frontend.h>
        ]], [[
            fe_delivery_system_t sys;
            sys = SYS_DVBC_ANNEX_A;
            sys = SYS_DVBC_ANNEX_C;
        ]])
    ], [
        AC_MSG_RESULT([separate A and C])
        AC_DEFINE([HAVE_DVBAPI_C_ANNEX_AC], [1],
            [Define to 1 if you have separate constants for DVB-C annex A and C])
    ], [
        AC_MSG_RESULT([single constant])
    ])

    # Network header (not present on FreeBSD)
    AC_CHECK_HEADERS([linux/dvb/net.h])

    have_dvbapi="yes"
], [
    AC_MSG_RESULT([no])
])
AS_IF([test "x${have_dvbapi}" = "xno"], [
    AC_MSG_WARN([no DVB API found; DVB functionality will be unavailable]) ])

AM_CONDITIONAL([HAVE_DVBAPI], [test "x${have_dvbapi}" = "xyes"])

#
# Optional features
#
AX_HELP_SECTION([Features and modules:])

# Built-in scripts
AC_ARG_ENABLE([inscript], AC_HELP_STRING([--disable-inscript],
    [don't build Lua scripts into binary (enabled)]))

AS_IF([test "x${enable_inscript}" != "xno"], [
    AC_DEFINE([HAVE_INSCRIPT], [1],
        [Define to 1 if built-in script module is enabled])
])
AM_CONDITIONAL([HAVE_INSCRIPT], [test "x${enable_inscript}" != "xno"])

# IGMP emulation
AC_ARG_ENABLE([igmp-emulation], AC_HELP_STRING([--enable-igmp-emulation],
    [send IGMP using raw sockets (disabled, needs CAP_NET_RAW)]))

AS_IF([test "x${enable_igmp_emulation}" = "xyes"], [
    AC_DEFINE([IGMP_EMULATION], [1],
        [Define to 1 to use IGMP emulation via raw sockets])
])
AM_CONDITIONAL([IGMP_EMULATION], [test "x${enable_igmp_emulation}" != "xno"])

# Streaming modules
stream_modules="analyze channel transmit"

# asi
AX_STREAM_MODULE(asi, [DVB-ASI input module],
    [test "x${SYS}" = "xposix"], [not available on Windows])

# biss_encrypt
AX_STREAM_MODULE(biss, [BISS software scrambler],
    [test "x${have_dvbcsa}" = "xyes"], [libdvbcsa is unavailable])

# dvb
AX_STREAM_MODULE(dvb, [DVB input module],
    [test "x${have_dvbapi}" = "xyes"], [Linux DVB API is unavailable])

# file
AX_STREAM_MODULE(file, [file I/O module])

# http
AX_STREAM_MODULE(http, [HTTP server and client])

# pipe
AX_STREAM_MODULE(pipe, [external process module])

# remux
AX_STREAM_MODULE(remux, [remultiplexer module])

# softcam
AX_STREAM_MODULE(softcam, [software descrambler],
    [test "x${have_dvbcsa}" = "xyes"], [libdvbcsa is unavailable])

# udp
AX_STREAM_MODULE(udp, [UDP receive and transmit module])

#
# Developer options
#
AX_HELP_SECTION([Developer options:])
kill_optflags="no"

# Debug version
AC_ARG_ENABLE([debug],
    AC_HELP_STRING([--enable-debug], [build a debug version]))

AS_IF([test "x${enable_debug}" = "xyes"], [
    AC_DEFINE([DEBUG], [1], [Define when building a debug version])

    # add debug info
    AX_CHECK_CFLAGS([-g3], [], [], [
        # try to fall back to -g
        AX_CHECK_CFLAGS([-g], [], [], [
            AC_MSG_ERROR([$CC doesn't support required debug flag; pass --disable-debug to disable this check])
        ])
    ])

    # non-essential debug flags
    AX_CHECK_CFLAGS([-ggdb -ggdb3 -fno-builtin])
    kill_optflags="yes"
])

# Test coverage
AC_ARG_ENABLE([gcov],
    AC_HELP_STRING([--enable-gcov], [enable test coverage analysis]))

AS_IF([test "x${enable_gcov}" = "xyes"], [
    AX_CHECK_CFLAGS([-ftest-coverage -fprofile-arcs], [], [], [
        AC_MSG_ERROR([$CC doesn't support gcov; pass --disable-gcov to disable this check])
    ])

    LIBS="$LIBS -lgcov"
    kill_optflags="yes"
])

# Profiling
AC_ARG_ENABLE([gprof],
    AC_HELP_STRING([--enable-gprof], [enable profiling with gprof]))

AS_IF([test "x${enable_gprof}" = "xyes"], [
    AX_CHECK_CFLAGS([-pg], [], [], [
        AC_MSG_ERROR([$CC doesn't support gprof; pass --disable-gprof to disable this check])
    ])
])

# Generate preprocessed files
AC_ARG_ENABLE([preprocess],
    AC_HELP_STRING([--enable-preprocess], [generate preprocessed files (*.i)]))

AM_CONDITIONAL([HAVE_PREPROCESSED], [test "x${enable_preprocess}" = "xyes"])

# Remove optimization flags
AS_IF([test "x${kill_optflags}" = "xyes"], [
    CFLAGS=$(echo "$CFLAGS" | $SED -r -e 's/@<:@ @:>@*(-O@<:@^ @:>@*)@<:@ @:>@*/ /g')
    AX_CHECK_CFLAG([-O0])
], [
    AX_CHECK_CFLAG([-fomit-frame-pointer])
    AX_CHECK_CFLAG([-funsafe-loop-optimizations])
    AX_CHECK_CFLAG([-ffast-math])
])

# Lots of warnings
WARN_CFLAGS=""
AC_ARG_ENABLE([warnings],
    AC_HELP_STRING([--enable-warnings], [turn on additional compiler warnings]))

# these are always on
AX_CHECK_CFLAG([-Wall], [WARN_CFLAGS])
AX_CHECK_CFLAG([-Wextra], [WARN_CFLAGS])

AS_IF([test "x${enable_warnings}" = "xyes"], [
    # older GCC versions don't support -Wpedantic
    AX_CHECK_CFLAG([-Wpedantic], [WARN_CFLAGS], [], [
        AX_CHECK_CFLAGS([-pedantic], [WARN_CFLAGS]) ])

    AX_CHECK_CFLAG([-Waggregate-return], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wbad-function-cast], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wclobbered], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wdouble-promotion], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wfloat-equal], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Winit-self], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wmissing-declarations], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wmissing-include-dirs], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wmissing-prototypes], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wnested-externs], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wold-style-definition], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wpointer-arith], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wstack-protector], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wstrict-aliasing], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wstrict-prototypes], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wsync-nand], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wtrampolines], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wundef], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wuninitialized], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wunused], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wuseless-cast], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wvector-operation-performance], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wwrite-strings], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wformat=2], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wc++-compat], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wdisabled-optimization], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Winvalid-pch], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wshadow], [WARN_CFLAGS])

    # OS-specific
    case "$host_os" in
        openbsd*)
            ;;
        *)
            AX_CHECK_CFLAG([-Wredundant-decls], [WARN_CFLAGS])
    esac

    # GCC-specific
    AX_CHECK_CFLAG([-Wjump-misses-init], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wlogical-op], [WARN_CFLAGS])

    # Clang-specific
    AX_CHECK_CFLAG([-Wgnu], [WARN_CFLAGS])
])

AS_IF([test "x${compiler_lang}" = "xC++"], [
    AX_CHECK_CFLAG([-Wno-variadic-macros], [WARN_CFLAGS]) ])

# Less important warnings
AC_ARG_ENABLE([suggestions],
    AC_HELP_STRING([--enable-suggestions], [turn on non-vital warnings (e.g. -Wsuggest-*)]))

AS_IF([test "x${enable_suggestions}" = "xyes"], [
    AX_CHECK_CFLAG([-Wsuggest-attribute=pure], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wsuggest-attribute=const], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wsuggest-attribute=noreturn], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wsuggest-attribute=format], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wmissing-format-attribute], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wunsafe-loop-optimizations], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wstrict-overflow=5], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-Wcast-align], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-fipa-pure-const], [WARN_CFLAGS])
    AX_CHECK_CFLAG([-fstrict-overflow])
], [
    AX_CHECK_CFLAG([-fwrapv])
])

# -Werror build
AC_ARG_ENABLE([werror],
    AC_HELP_STRING([--enable-werror], [turn on -Werror]))

AS_IF([test "x${enable_werror}" = "xyes"], [
    AX_CHECK_CFLAGS([-Werror], [WARN_CFLAGS])
])

AC_SUBST(WARN_CFLAGS)

# Other flags
AX_CHECK_CFLAG([-fstrict-aliasing])

# Makefile generation
AC_CONFIG_FILES([
    Makefile
    lua/Makefile
    scripts/Makefile
    tools/Makefile
    src/Makefile
    tests/Makefile
    testcards/Makefile
])
AC_OUTPUT

# Print configuration summary
echo
echo "Configuration summary"
echo "-----------------------"
echo "Version:               ${PACKAGE_STRING}"
echo
echo "Compiler:              ${CC}"
echo "Building for:          ${host_os}"
echo "Event mechanism:       ${event_mechanism}"

# large file support
echo -n "Large files:           "
AS_IF([test "x${enable_largefile}" != "xno"], [echo "yes"], [echo "no"])
echo

# lua (lib/compiler)
echo -n "Lua library:           "
AS_IF([test "x${have_lua}" = "xyes"], [echo "external"], [echo "internal"])

echo -n "Lua compiler:          "
AS_IF([test -n "${LUAC}"], [echo "${LUAC}"], [echo "no"])

# ffmpeg
echo -n "ffmpeg program:        "
AS_IF([test -n "${FFMPEG}"], [echo "${FFMPEG}"], [echo "no"])

# libdvbcsa
echo -n "libdvbcsa:             "
AS_IF([test "x${have_dvbcsa}" = "xyes"], [echo "yes"], [echo "no"])

# libcrypto
echo -n "libcrypto:             "
AS_IF([test "x${have_libcrypto}" = "xyes"], [echo "yes"], [echo "no"])

# aio (linux/posix)
echo -n "Asynchronous I/O:      "
AS_IF([test "x${have_aio}" = "xyes"], [
    AS_IF([test "x${have_libaio}" = "xyes"],
        [echo "yes (Linux)"], [echo "yes (POSIX)"])
], [
    echo "no"
])

# dvb api
echo -n "Linux DVB API:         "
AS_IF([test "x${have_dvbapi}" = "xyes"], [
    AS_IF([test "x${have_dvbapi_t2}" = "xyes"],
        [echo "yes"], [echo "old version (no DVB-T2)"])
], [
    echo "no"
])

# inscript
echo -n "Built-in scripts:      "
AS_IF([test "x${enable_inscript}" != "xno"], [
    AS_IF([test -n "${LUAC}"],
        [ echo "yes (compiled)" ], [ echo "yes (plaintext)" ])
], [
    echo "no"
])

# igmp emulation
echo -n "IGMP emulation:        "
AS_IF([test "x${enable_igmp_emulation}" = "xyes"],
    [echo "yes"], [echo "no"])

# check
echo -n "Check (unit tests):    "
AS_IF([test "x${have_check}" = "xyes"],
    [echo "yes"], [echo "no"])

# streaming module list
echo
echo -n "Streaming modules:     "
first="yes"
for mod in ${stream_modules}; do
    if [ test "x${first}" != "xyes" ]; then
        echo -n "                       "
    fi
    echo "${mod}"
    first="no"
done
echo

# debug build
echo -n "Debug build:           "
AS_IF([test "x${enable_debug}" = "xyes"], [echo "yes"], [echo "no"])

# extra warnings
echo -n "Extra warnings:        "
AS_IF([test "x${enable_warnings}" = "xyes"], [
    echo -n "yes"
    AS_IF([test "x${enable_suggestions}" = "xyes"], [
        echo -n " (w/suggestions)"
    ])
    echo
], [
    echo "no"
])

# test coverage
echo -n "Test coverage:         "
AS_IF([test "x${enable_gcov}" = "xyes"], [echo "yes"], [echo "no"])

# profiling
echo -n "Profiling:             "
AS_IF([test "x${enable_gprof}" = "xyes"], [echo "yes"], [echo "no"])
