all:	stuff

do_work.o: do_work.cpp
	gcc -dynamic -fno-strict-aliasing -mno-fused-madd -fno-common -fwrapv -DDEBUG_ENABLED -g -Wall -I .. \
	  -c do_work.cpp -o do_work.o

stuff: do_work.o
	g++ -o stuff do_work.o ../osx/sl_support.o

#blob.so

pthreaded.o: pthreaded.cpp
	gcc -dynamic -fno-strict-aliasing -mno-fused-madd -fno-common -fwrapv -I/Developer/SDKs/MacOSX10.5.sdk/System/Library/Frameworks/Python.framework/Versions/2.5/include/python2.5 -DDEBUG_ENABLED -g -Wall \
		-I /Users/gavin/Desktop/Development/Mercurial/cyclicity-cdl.hg/support_libraries \
	  -c pthreaded.cpp -o pthreaded.o

blob.so: pthreaded.o
	g++ -L/opt/local/lib -bundle -undefined dynamic_lookup -o blob.so pthreaded.o /Users/gavin/Desktop/Development/Mercurial/cyclicity-cdl.hg/support_libraries/osx/sl_support_with_python.o
