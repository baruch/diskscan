redo-ifchange vars
sed -e '1i#include "version.h"' \
    -e 's/^/char */' \
    -e 's/$/;/' \
    vars
