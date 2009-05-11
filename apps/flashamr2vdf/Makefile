PROGRAMS = flashamr2vdf flashvdfcreate

all:
	for i in $(PROGRAMS); do $(MAKE) -f Makefile.$$i all; done

dep:
	for i in $(PROGRAMS); do $(MAKE) -f Makefile.$$i all; done

clean:
	for i in $(PROGRAMS); do $(MAKE) -f Makefile.$$i clean; done

install:
	for i in $(PROGRAMS); do $(MAKE) -f Makefile.$$i install; done

install-dep:
	for i in $(PROGRAMS); do $(MAKE) -f Makefile.$$i install-dep; done

clobber:
	for i in $(PROGRAMS); do $(MAKE) -f Makefile.$$i clobber; done

headers:
	for i in $(PROGRAMS); do $(MAKE) -f Makefile.$$i headers; done

doc:
	for i in $(PROGRAMS); do $(MAKE) -f Makefile.$$i doc; done
