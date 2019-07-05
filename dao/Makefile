DIRS = mxml-3.0 json
all:
	for i in $(DIRS) ; do make -C $$i $@ || exit $?; done

clean:
	for i in $(DIRS) ; do make -C $$i clean ; done
