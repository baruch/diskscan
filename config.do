redo-ifchange autogen.sh *.ac
./autogen.sh 2>&1 | tee autoconf.log
./configure 2>&1 | tee configure.log
cat config.h configure Makefile | redo-stamp
