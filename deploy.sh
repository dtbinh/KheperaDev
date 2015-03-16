#!/bin/bash
# dependencies: arm-angstrom-linux-gnueabi-gcc, libkorebot, sshpass.
#
# Sets ENV variables (see env.sh)
# compiles and sends KH3SOURCE
# with name KH3PROGRAM to Khepera III
# located on KH3IP
# with user=KH3USER pass=KH3PASS
KH3SOURCE=kh3control.c
KH3PROGRAM=kh3control
KH3IP=192.168.141.100
KH3USER=root
KH3PASS=toor
#prepare environment variables
. ./env.sh
if [ $? -eq 0 ]; 
	then
		echo ENV variables set OK
		#compile with cross compiler
		arm-angstrom-linux-gnueabi-gcc $KH3SOURCE -o $KH3PROGRAM -I $INCPATH -L $LIBPATH -lkorebot -L $LIBKOREBOT\_ROOT/build-korebot-2.6/lib -lkorebot
		if [ $? -eq 0 ];
			then 
				echo compile $KH3SOURCE to $KH3PROGRAM OK
				# scp file to Khepera
				sshpass -p $KH3PASS scp $KH3PROGRAM $KH3USER@$KH3IP:/home/root
				if [ $? -eq 0  ];
					then
						echo scp $KH3PROGRAM to $KH3IP OK
						#remove compiled program to avoid masking updates
						rm $KH3PROGRAM
				fi

		fi
fi