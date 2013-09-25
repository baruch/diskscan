exec >&2
redo-always
redo version/clean Documentation/clean
for dir in . cli lib version; do
        rm -f $dir/*.a $dir/*.o $dir/*.did $dir/.*.d
done
rm -rf diskscan tags .do_built .do_built.dir .redo
