# ember
Ember (EMBedding helpER) turns files into char arrays and packs them into C headers

## how to use
run
```ember```
to get an overview on usage.

## how to build
running
```make build```
will compile the ember executable.
```make install```
will try to install into /usr/local/bin/ and
```make install-local```
will try to install into ~/bin/.
To install into a custom directory, run
```make INSTALL_DIR="{custom directory}" install```
