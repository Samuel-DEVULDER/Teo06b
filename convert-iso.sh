#!/bin/sh 
cr=`printf "\r"`
temp="temp$$"
for i in *.[ch]
do 
	echo "$i.." 
	sed -e "s/$cr//g; y/���/���/;" $i >$temp
	cp $temp $i
	rm $temp 
done
