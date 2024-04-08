SHELL := /bin/bash

all: app view slave

testfiles:
	mkdir -p files
	@cd files; \
	for ((i=1; i<=5; i++)); do \
		dd if=/dev/urandom of=file$$i bs=$$((RANDOM % 300))M count=1; \
		ln -s file$$i link$${i}_1; \
		ln -s file$$i link$${i}_2; \
		ln -s file$$i link$${i}_3; \
		ln -s file$$i link$${i}_4; \
		ln -s file$$i link$${i}_6; \
	done;

analyze: pvs valgrind compare

app: app.c
	gcc -Wall $< -g -o $@

view: view.c
	gcc -Wall $< -g -o $@

slave: slave.c
	gcc -Wall $< -g -o $@

pvs: clean pvs_test

pvs_test:
	@tput setaf 4
	@echo -e "\n\nRUNNING PVS-STUDIO TEST\n\n"
	@tput sgr0
	@pvs-studio-analyzer trace -- make
	@pvs-studio-analyzer analyze
	@rm -f strace_out
	@plog-converter -a '64:1,2,3;GA:1,2,3;OP:1,2,3' -t tasklist -o report.tasks PVS-Studio.log
	@rm -f PVS-Studio.log

	@if [ -s "report.tasks" ]; then \
		tput setaf 1; \
		echo -e "\n\nPVS-STUDIO ENCOUNTERED AN ERROR\n"; \
		cat report.tasks; \
	else \
		tput setaf 2; \
		echo -e "\n\nPVS-STUDIO TEST FINISHED SUCCESSFULLY\n"; \
	fi
	@tput sgr0

valgrind:
	@tput setaf 4
	@echo -e "\nRUNNING VALGRIND TEST FOR APP/SLAVE"
	@tput sgr0
	@VAL1RES=$$(valgrind --leak-check=full --show-leak-kinds=all -s --trace-children=yes --trace-children-skip=/usr/bin/md5sum,/bin/sh --log-fd=9 ./app ./files/* 9>&1 1>/dev/null | cat); \
	VAL1GREP=$$(echo "$$VAL1RES" | grep -e "ERROR SUMMARY: [1-9][0-9]*" | cat);\
	if [ ! -z "$$VAL1GREP" ]; then \
		tput setaf 1; \
		echo -e "\n\nVALGRIND ENCOUNTERED AN ERROR IN APP-SLAVE\n"; \
		echo -e "$$VAL1RES"; \
	else \
		tput setaf 2; \
		echo -e "\n\nVALGRIND APP-SLAVE TEST FINISHED SUCCESSFULLY\n"; \
	fi
	@tput setaf 4
	@echo -e "\nRUNNING VALGRIND TEST FOR VIEW"
	@tput sgr0	
	@./app ./files/* > /dev/null &
	@VAL2RES=$$(echo "/app_shm" | valgrind --leak-check=full --show-leak-kinds=all -s --trace-children=yes --trace-children-skip=/usr/bin/md5sum,/bin/sh --log-fd=9 ./view 9>&1 1>/dev/null | cat); \
	VAL2GREP=$$(echo "$$VAL2RES" | grep -e "ERROR SUMMARY: [1-9][0-9]*" | cat);\
	if [ ! -z "$$VAL2GREP" ]; then \
		tput setaf 1; \
		echo -e "\n\nVALGRIND ENCOUNTERED AN ERROR IN VIEW\n"; \
		echo -e $$VAL2RES; \
	else \
		tput setaf 2; \
		echo -e "\n\nVALGRIND VIEW TEST FINISHED SUCCESSFULLY\n"; \
	fi
	@tput sgr0	

compare:
	@tput setaf 4
	@echo -e "\nRUNNING DIFF"
	@tput sgr0	
	@DIFFRES=$$(diff <(./app files/* | ./view | sort | sed 's/ - [0-9]\+$$//g')  <(md5sum files/* | while read md5 file; do echo "$$file - $$md5"; done | sort) | cat);\
	if [ ! -z "$$DIFFRES" ]; then \
		tput setaf 1; \
		echo -e "\n\nnDIFF HAS FOUND SOME DIFFERENCES\n"; \
		echo -e "$$DIFFRES"; \
	else \
		tput setaf 2; \
		echo -e "\n\nDIFF DIDN'T FIND ANY DIFFERENCES\n"; \
	fi
	@tput sgr0	
	
clean:
	rm -f app view slave

.PHONY: all clean pvs pvs_test valgrind compare testfiles


