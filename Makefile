CFLAGS = -Wall -Wextra -Werror


all: create_tmp_folder create_bin_folder bin/tracer bin/monitor

create_tmp_folder:
	mkdir -p tmp
	
create_bin_folder:
	mkdir -p bin

bin/tracer: src/tracer.c
	gcc $(CFLAGS) -o $@ $<

bin/monitor: src/monitor.c
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -rf bin/* tmp/* bin tmp
	

