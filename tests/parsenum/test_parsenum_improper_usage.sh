#!/bin/sh -e

log=abort.log
rm -f $log

# Check assertion failures, separating lines so that we automatically check
# that test_parsenum is giving a non-zero exit code.
( set +e; ! ./test_parsenum 1 ) >$log 2>&1
grep -q "PARSENUM applied to signed integer without specified bounds" $log

( set +e; ! ./test_parsenum 2 ) >$log 2>&1
grep -q "PARSENUM_EX applied to signed integer without specified bounds" $log

( set +e; ! ./test_parsenum 3 ) 2> $log
grep -q "PARSENUM_EX applied to float" $log

( set +e; ! ./test_parsenum 4 ) 2> $log
grep -q "PARSENUM_EX applied to float" $log

rm $log
