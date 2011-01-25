all:	blob.so

pthreaded.o: pthreaded.cpp
	gcc-4.0 -dynamic -fno-strict-aliasing -mno-fused-madd -fno-common -fwrapv -I/Developer/SDKs/MacOSX10.5.sdk/System/Library/Frameworks/Python.framework/Versions/2.5/include/python2.5 -DDEBUG_ENABLED -g -Wall \
		-I /Users/gavin/Desktop/Development/Mercurial/cyclicity-cdl.hg/support_libraries \
	  -c pthreaded.cpp -o pthreaded.o

blob.so: pthreaded.o
	g++-4.0 -L/opt/local/lib -bundle -undefined dynamic_lookup -o blob.so pthreaded.o /Users/gavin/Desktop/Development/Mercurial/cyclicity-cdl.hg/support_libraries/osx/sl_support_with_python.o
