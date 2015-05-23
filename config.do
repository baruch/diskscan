redo-ifchange autogen.sh *.ac
./autogen.sh 1>&2
./configure 1>&2
cat config.h configure Makefile | redo-stamp
