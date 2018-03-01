AM_CXXFLAGS = -I../../../ $(BOOST_CPPFLAGS) $(POPT_CFLAGS)
AM_LDFLAGS = $(BOOST_LDFLAGS)

bin_PROGRAMS = winUsnJrnlViewer
winUsnJrnlViewer_SOURCES = winUsnJrnlViewer.cpp ../../misc/errMsgs.cpp
# winUsnJrnlViewer_CXXFLAGS = $(POPT_CFLAGS) -I../../../
winUsnJrnlViewer_LDADD = $(POPT_LIBS) ../../../libtimeUtils/build/src/libtimeUtils.a ../../../libWinUsnJrnl/build/src/libWinUsnJrnl.a ../../../libpasswdFile/build/src/libpasswdFile.a ../../../libbinData/build/src/libbinData.a ../../../libdelimText/build/src/libdelimText.a

