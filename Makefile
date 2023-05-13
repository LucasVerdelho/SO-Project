CFLAGS = -Wall -Wextra -Werror


all: create_tmp_folder bin/tracer bin/monitor

create_tmp_folder:
	mkdir -p tmp

bin/tracer: src/tracer.c
	gcc $(CFLAGS) -o $@ $<

bin/monitor: src/monitor.c
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -rf bin/* tmp/*
	

