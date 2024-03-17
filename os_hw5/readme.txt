Posix semaphore is used in the `sender.c` and thus linkage of pthread is needed

Both receiver.c and sender.c are already compiled for the Ubuntu 20.04 but can be recompiled using:

gcc ./receiver.c -o receiver -O2

gcc ./sender.c -o sender -lpthread -O2
