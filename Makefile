# Reference: http://nuclear.mutantstargoat.com/articles/make/#practical-makefile

CC = g++
LDFLAGS = -ludev -pthread

ps3timeout: main.cpp
	$(CC) -std=c++11 -o $@ main.cpp $(LDFLAGS)

.PHONY: clean
clean:
	rm -f ps3timeout

PREFIX = /usr/local

.PHONY: install
install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin 
	cp -rf ps3timeout $(DESTDIR)$(PREFIX)/bin/ps3timeout
	chmod +x ps3timeout.sh
	cp -rf ps3timeout.sh /etc/init.d/ps3timeout.sh
	update-rc.d ps3timeout.sh defaults

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/ps3timeout
	rm -f /etc/init.d/ps3timeout.sh
	update-rc.d ps3timeout.sh remove
