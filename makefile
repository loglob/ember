INSTALL_DIR=/usr/local/bin
LOCAL_INSTALL_DIR=~/bin

ember: main.c
	c99 -Wall -Wextra main.c -o ember

install: ember
	cp ember $(INSTALL_DIR)

install-local: ember
	cp ember $(LOCAL_INSTALL_DIR)

clean:
	rm ember

uninstall:
	rm $(INSTALL_DIR)/ember
