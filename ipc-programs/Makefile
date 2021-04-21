all: test_fork

test_fork: prepare_build test_fork.c
	g++ -g test_fork.c -o build/test_fork

prepare_build:
	mkdir -p build

*.c:
	touch $@

.PHONY: clean
clean:
	rm -r build
