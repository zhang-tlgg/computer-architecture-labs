export CHECK=true

if ($CHECK) then
    ./checker -p -f ./test/selection_sort -c ../checkfiles/selection_sort.chk
    ./checker -p -f ./test/ntt -c ../checkfiles/ntt.chk
fi;
