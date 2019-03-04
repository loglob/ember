INSTALL_DIR=/usr/local/bin/
LOCAL_INSTALL_DIR=~/bin/

ember: main.c
	gcc -std=c99 -Wall -Wextra main.c -o ember

install: ember
	ep ember $(INSTALL_DIR)

install-local: ember
	cp ember $(LOCAL_INSTALL_DIR)

clean:
	rm ember