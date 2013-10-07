redo-ifchange vars
sed -e '1i\
#include "version.h"' \
    -e 's/^/const char */' \
    -e 's/$/;/' \
    vars
