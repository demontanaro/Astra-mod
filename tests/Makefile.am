AM_CFLAGS = \
    $(DEFS) $(WARN_CFLAGS) \
    $(CHECK_CFLAGS) $(LUA_CFLAGS) \
    -I$(top_builddir)/src \
    -I$(top_srcdir)/src

AM_LDADD = \
    $(CHECK_LIBS) $(LUA_LIBS) \
    $(top_builddir)/src/libastra.la \
    $(top_builddir)/src/libstream.la

#
# Unit tests
#
if HAVE_CHECK
# TODO: fix output redirection on Windows
if HAVE_POSIX
AM_TESTS_ENVIRONMENT = export OUTPUT_REDIRECT_FD=9;
AM_TESTS_FD_REDIRECT = 9>&2
endif

check_PROGRAMS = unit_tests test_slave
TESTS = unit_tests
CLEANFILES = libastra.log

unit_tests_LDADD = $(AM_LDADD)
unit_tests_DEPENDENCIES = test_slave$(EXEEXT)
unit_tests_SOURCES = \
    unit_tests.c \
    unit_tests.h \
    core_alloc.c \
    core_child.c \
    core_clock.c \
    core_list.c \
    core_mainloop.c \
    core_spawn.c \
    core_thread.c \
    core_timer.c

test_slave_SOURCES = test_slave.c
test_slave_CFLAGS = $(AM_CFLAGS)
endif

#
# Test programs
#
noinst_PROGRAMS = t2mi_decap spammer

t2mi_decap_SOURCES = t2mi_decap.c
t2mi_decap_CFLAGS = $(AM_CFLAGS)
t2mi_decap_LDADD = $(AM_LDADD)

spammer_SOURCES = spammer.c
spammer_CFLAGS = $(AM_CFLAGS)
spammer_LDADD = \
    $(top_builddir)/src/libastra.la
