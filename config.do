redo-ifchange autogen.sh *.ac
./autogen.sh > autogen.sh.log 2>&1
./configure > configure.log 2>&1
cat config.h configure Makefile | redo-stamp
