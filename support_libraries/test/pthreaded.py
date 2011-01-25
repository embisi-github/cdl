#!/usr/bin/env python
import blob
import sys
def f(n):
    print >>sys.stderr,"f:Doing work on inputs, producing new outputs",n
def g(n):
    print >>sys.stderr,"g:Doing different work on inputs, producing new outputs",n
print "Doing stuff"
import time
for j in range(100000):
    fj = lambda:f(j)
    gj = lambda:g(j)
    blob.spawn(callback=fj)
    blob.spawn(callback=gj)
    for i in range(1):
        #time.sleep(0.003)
        #print >>sys.stderr,"Prior to a tick"
        blob.tick()
    blob.finalize()
print >>sys.stderr,"Done"
