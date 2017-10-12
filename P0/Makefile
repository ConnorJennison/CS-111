SHELL := /bin/bash

.SILENT:

all:
	gcc -Wall -Wextra lab0.c -o lab0

dist:
	tar -czf lab0-904474415.tar.gz lab0.c Makefile *.png README

clean:
	echo "*****Cleaning up files.*****"
	rm -f lab0.o lab0 *.tar.gz *.txt

check: all
	echo " " ;\
	echo "#######################################" ;\
	echo "Test 1: Input Redirection" ;\
	echo "This is a test to test program functionality" >input.txt ;\
	./lab0 <input.txt >output.txt ;\
	cmp -s input.txt output.txt ;\
	if [[ $$? -eq 0 ]]; then \
		echo "Test 1 Succeeded: Redirection works as intended" ; \
	else \
		echo "Test 1 Failed: Redirection doesn't work as intended" ; \
	fi;

	echo "######################################" ;\
	echo "Test 2: Normal Input and Output" ;\
	./lab0 --input input.txt --output output.txt ;\
	cmp -s input.txt output.txt ;\
	if [[ $$? -eq 0 ]]; then \
		echo "Test 2 Succeeded: Normal Input and Output work as intended" ; \
	else \
		echo "Test 2 Failed: Normal Input and Output options do not work as intended" ; \
	fi;

	echo "#####################################" ;\
	echo "Test 3: Bad Input Given" ;\
	./lab0 --input FileDoesNotExist.txt --output output.txt ;\
	if [[ $$? -eq 2 ]]; then \
		echo "Test 3 Succeeded: Correct exit value of 2 given" ;\
	else \
		echo "Test 3 Failed: Wrong exit value returned" ;\
	fi;

	echo "#####################################" ;\
	echo "Test 4: Bad Output Given" ;\
	touch BADFILE.txt ;\
	chmod 000 BADFILE.txt ;\
	./lab0 --input input.txt --output BADFILE.txt ;\
	if [[ $$? -eq 3 ]]; then \
		echo "Test 4 Succeeded: Correct exit value of 3 given" ;\
	else \
		echo "Test 4 Failed: Wrong exit value returned" ;\
	fi;

	echo "#####################################" ;\
	echo "Test 5: Catch SegFault" ;\
	./lab0 --catch --segfault ;\
	if [[ $$? -eq 4 ]]; then \
		echo "Test 5 Succeeded: Segfault caught" ;\
	else \
		echo "Test 5 Failed: Segfault not caught" ;\
	fi;

	echo "####################################" ;\
	echo "Test 6: Crash with SegFault" ;\
	./lab0 --segfault ;\
	if [[ $$? -eq 139 ]] ; then \
		echo "Test 6 Succeeded: Program successfully crashed" ;\
	else \
		echo "Test 6 Failed: Program not crashed with segfault" ;\
	fi;

	echo "###################################" ;\
	echo "Test 7: No input file given" ;\
	./lab0 --input ;\
	if [[ $$? -eq 1 ]] ; then \
		echo "Test 7 Succeeded: No input file handled properly" ;\
	else \
		echo "Test 7 Failed: No input file not handled properly" ;\
	fi;

	echo "###################################" ;\
	echo "Test 8: No output file given" ;\
	./lab0 --output ;\
	if [[ $$? -eq 1 ]] ; then \
		echo "Test 8 Succeeded: No output file handled properly" ;\
	else \
		echo "Test 8 Failed: No output file not handled properly" ;\
	fi;

	echo "###################################" ;\
	echo " " ;\
	echo "All tests completed" ;\
	echo " " ;\
	echo "###################################" ;\

debug:
	gcc -Wall -Wextra -g lab0.c -o lab0
