#! /bin/bash

function setMode {
	# modes :
	#  0 sensors off
	#  1 both IR and green sensors on
	#  2 green sensor only
	#  3 IR sensor only

	if [ $1 -le 3 ] && [ $1 -ge 0 ] && [ $1 -ne $mode ]; then
		mode=$1
		echo $((mode << 2)) > max44004/main_config
	fi
}

function setGain {
	# gain levels :
	#  0 : 0.03125 lux/unit
	#  1 : 0.0125  lux/unit
	#  2 : 0.5     lux/unit
	#  3 : 4       lux/unit

	if [ $1 -le 3 ] && [ $1 -ge 0 ] && [ $1 -ne $gain ]; then
		gain=$1
		echo $gain > max44004/receiver_config
	fi
}

if [ -L max44004 ]; then
	mainConf=`cat max44004/main_config | head -n1`
	receiverConf=`cat max44004/receiver_config | head -n1`
	mode=$(((mainConf & 0xC)>> 2))
	gain=$((receiverConf & 0x3))

	# reading IR+green (ALS value)
	setMode 1
	# starting with minimal gain
	setGain 0

	sleep 0.15s
	lux=`cat max44004/lux`
	while [ $lux -eq -1 ] && [ $gain -lt 3 ]; do
		setGain $((gain + 1))
		sleep 0.15s
		lux=`cat max44004/lux`
	done

	if [ $lux -lt 0 ]; then
		echo 65535
	else
		factor=-1
		case $gain in
		0)
			factor=0.03215
			;;
		1)
			factor=0.125
			;;
		2)
			factor=0.5
			;;
		3)
			factor=4
			;;
		esac 
		echo `echo "scale=3; $lux * $factor" | bc`
	fi
	# reset gain
	setGain 0
	# turn off sensors
	setMode 0
else
	echo "max44004 symlink required"
fi
