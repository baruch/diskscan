exec >&2
redo-always
redo version/clean
for dir in . cli lib; do
        rm -f $dir/*.a $dir/*.o $dir/*.did
done
rm -f diskscan tags
