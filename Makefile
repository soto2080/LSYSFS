COMPILER = gcc
FILESYSTEM_FILES = lsysfs.c

build: $(FILESYSTEM_FILES)
	$(COMPILER) -D_GNU_SOURCE $(FILESYSTEM_FILES) -o lsysfs `pkg-config fuse --cflags --libs`
	echo 'To Mount: ./lsysfs -f [mount point]'

clean:
	rm lsysfs
