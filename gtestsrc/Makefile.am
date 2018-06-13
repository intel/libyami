AUTOMAKE_OPTIONS = subdir-objects

# libgtest
noinst_LTLIBRARIES = libgtest.la

libgtest_la_SOURCES = \
	gtest/src/gtest-all.cc \
	gtest/src/gtest_main.cc \
	$(NULL)

libgtest_la_CXXFLAGS = \
	-I$(top_srcdir)/gtestsrc/gtest \
	-I$(top_srcdir)/gtestsrc/gtest/include \
	$(AM_CXXFLAGS) \
	$(NULL)

EXTRA_DIST = \
	gtest/docs \
	gtest/include \
	gtest/src/*.cc \
	gtest/src/*.h \
	gtest/README.md \
	gtest/LICENSE \
	$(NULL)
