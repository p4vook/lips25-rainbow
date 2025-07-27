SUBDIRS = 2_partite

all clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir $@; done

.PHONY: all clean
