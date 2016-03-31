hgmv=$1

FROM="JOINCACHE"
TO="COMMON"

LIBS="SourcePyramid
SourceLevel"

for lib in ${LIBS}
do
    if [ "$hgmv" == "1" ] ; then
        hg mv lib/${FROM}/${lib}.pm lib/${TO}/${lib}.pm
    fi
    echo -e "sed -i 's/${FROM}::${lib}/${TO}::${lib}/g' tests/perlunit/*/*.t"
    sed -i "s/${FROM}::${lib}/${TO}::${lib}/g" tests/perlunit/*/*.t
    echo -e "sed -i 's/${FROM}::${lib}/${TO}::${lib}/g' pyramide/lib/*/*.pm"
    sed -i "s/${FROM}::${lib}/${TO}::${lib}/g" lib/*/*.pm
    echo -e "sed -i 's/${FROM}::${lib}/${TO}::${lib}/g' pyramide/bin/*.pl.in"
    sed -i "s/${FROM}::${lib}/${TO}::${lib}/g" bin/*.pl.in
done

