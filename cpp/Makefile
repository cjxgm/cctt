all: build-cctt
clean:
	rm -rf build/
run: all
	cd build && ./cctt

build-cctt: | build/
	cd build && cmake ..
	$(MAKE) -C build cctt

%/:
	mkdir -p $@

