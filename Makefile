
testsa: simplefs.c create_format.c app.c
	rm -fr *.o *.a *~ a.out app  vdisk create_format test
	gcc -Wall -c simplefs.c
	ar -cvq libsimplefs.a simplefs.o
	ranlib libsimplefs.a
	gcc -Wall -o create_format  create_format.c   -L. -lsimplefs
	gcc -Wall -o test test.c  -L. -lsimplefs
	./test

testlib: simplefs.c
	rm -f simplefs
	gcc -Wall -o simplefs simplefs.c
	./simplefs

all: libsimplefs.a create_format app

libsimplefs.a: 	simplefs.c
	gcc -Wall -c simplefs.c
	ar -cvq libsimplefs.a simplefs.o
	ranlib libsimplefs.a

create_format: create_format.c
	gcc -Wall -o create_format  create_format.c   -L. -lsimplefs

app: 	app.c
	gcc -Wall -o app app.c  -L. -lsimplefs

clean: 
	rm -fr *.o *.a *~ a.out app  vdisk create_format
