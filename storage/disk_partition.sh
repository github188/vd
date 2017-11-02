#!/bin/sh

if ! [ -b "$1" ]
then
	echo "$1 is not a block device !"
	exit 1
fi

if [ -n "$3" ]
then

if [ "$2" = "4" ]
then

fdisk -cu "$1" <<End
n
p
1

+$3
n
p
2

+$3
n
p
3

+$3
n
p
4


w
End

elif [ "$2" = "2" ]
then

fdisk -cu "$1" <<End
n
p
1

+$3
n
p
2


w
End

else

echo "Wrong number of arguments !"

fi

else

echo "Wrong arguments!"

fi

exit 0
