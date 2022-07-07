#!/bin/sh

while [ 1 ]
do
	echo "timeout..........."
	speed="1000Mb/s"
	ret=`sudo  ethtool enaphyt4i1 |grep "Speed"|cut -d ":" -f 2`
	echo "ret = $ret" 
	if [ $ret != $speed ]	
	then		
  		ehco "sudo ethtool -s enaphyt4i1 autoneg off speed 1000 duplex full"
		sudo ethtool -s enaphyt4i1 autoneg off speed 1000 duplex full
		sudo ifconfig enaphyt4i1 down
		sudo ifconfig enaphyt4i1 up
	fi
	sleep 2
done
