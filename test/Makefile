TARGETS = echo-test smtp-test pop3-test

all: $(TARGETS)

%.o: %.cc
	g++ -Iinclude $^ -c -o $@

echo-test: echo-test.o common.o
	g++ $^ -o $@

smtp-test: smtp-test.o common.o
	g++ $^ -o $@

pop3-test: pop3-test.o common.o
	g++ $^ -o $@

clean::
	rm -fv $(TARGETS) *.o *~
