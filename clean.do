exec >&2
redo-always
redo version/clean Documentation/clean
for dir in . cli lib arch; do
        rm -f $dir/*.a $dir/*.o $dir/*.did $dir/.*.d
done
rm -f diskscan tags .do_built arch/arch.c arch/arch.id
rm -rf .do_built.dir .redo
