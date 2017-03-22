# AM_CXXFLAGS = -I../../ $(BOOST_CPPFLAGS)
# AM_LDFLAGS = $(BOOST_LDFLAGS)

bin_PROGRAMS = winUsnJrnlViewer
winUsnJrnlViewer_SOURCES = winUsnJrnlViewer.cpp
winUsnJrnlViewer_CXXFLAGS = $(POPT_CFLAGS) -I../../
winUsnJrnlViewer_LDADD = $(POPT_LIBS) ../../libtimeUtils/src/libtimeUtils.a ../../libWinUsnJrnl/src/libWinUsnJrnl.a ../../libpasswdFile/src/libpasswdFile.a ../../libbinData/src/libbinData.a ../../libdelimText/src/libdelimText.a

