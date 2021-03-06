#!/bin/bash -ue

START=`date +%s`

echo clean start
./clean.sh >/dev/null 2>&1 | :
echo clean end

echo st1 start
./example_st1.sh
echo st1 end

echo st2 start
./example_st2.sh
sleep 5 # wait conf file
echo st2 end

echo st3 start
./example_st3.sh
sleep 5 # XXX: TODO
echo st3 end

echo st4c start
./example_st4c.sh
sleep 5 # XXX: TODO
echo st4c end

echo st4d start
./example_st4d.sh
sleep 5 # XXX: TODO
echo st4d end

echo st4e start
./example_st4e.sh
sleep 5 # XXX: TODO
echo st4e end

echo st4f start
./example_st4f.sh
sleep 5 # XXX: TODO
echo st4f end

echo st5 start
./example_st5.sh
sleep 5 # XXX: TODO
echo st5 end

echo clean start
./clean.sh
echo clean end

END=`date +%s`
ELAPSED=`expr $END - $START` 
echo "$ELAPSED seconds"
