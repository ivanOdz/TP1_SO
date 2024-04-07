SHELL := /bin/bash

all: app view slave

testfiles:
	mkdir -p files
	cd files; \
	for ((i=1; i<=5; i++)); do \
		dd if=/dev/urandom of=file$$i bs=$$((RANDOM % 300))M count=1; \
		ln -s file$$i link$${i}_1; \
		ln -s file$$i link$${i}_2; \
		ln -s file$$i link$${i}_3; \
		ln -s file$$i link$${i}_4; \
		ln -s file$$i link$${i}_6; \
	done;

analyze: clean pvs clean all valgrind compare

app: app.c
	gcc -Wall $< -g -o $@

view: view.c
	gcc -Wall $< -g -o $@

slave: slave.c
	gcc -Wall $< -g -o $@

pvs:
	@echo -e "\n\nRUNNING PVS-STUDIO TEST\n\n\n"
	pvs-studio-analyzer trace -- make
	pvs-studio-analyzer analyze
	plog-converter -a '64:1,2,3;GA:1,2,3;OP:1,2,3' -t tasklist -o report.tasks PVS-Studio.log
	rm -f PVS-Studio.log
	@echo -e "\n\n\nPVS-STUDIO TEST FINISHED\n"

valgrind:
	@echo -e "\n\nRUNNING VALGRIND TEST\n\n\n"
	valgrind --leak-check=full --show-leak-kinds=all -s --trace-children=yes --trace-children-skip=/usr/bin/md5sum,/bin/sh ./app ./files/*
	./app ./files/*&
	valgrind --leak-check=full --show-leak-kinds=all -s --trace-children=yes --trace-children-skip=/usr/bin/md5sum,/bin/sh --log-fd=9 ./view /app_shm 9>&1 1>/dev/null
	@echo -e "\n\n\nVALGRIND TEST FINISHED\n"

compare:
	@echo -e "\n\nRUNNING DIFF\n\n\n"
	/usr/bin/md5sum ./files/** | while read md5 file; do echo "$$file - $$md5"; done | sort > outputReal.txt
	sort results.txt | sed 's/ - [0-9]\+$$//g' > outputSorted.txt
	diff outputReal.txt outputSorted.txt
	rm -f outputReal.txt outputSorted.txt results.txt
	@echo -e "\n\n\nDIFF ENDED\n\n"
clean:
	rm -f app view slave

.PHONY: all clean pvs valgrind compare testfiles


