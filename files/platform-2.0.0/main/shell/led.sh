#!/bin/sh


GPIO_DIR=/sys/class/gpio
LED_POWER_PIN=419	#EMIO 3
LED_WORK_PIN=416 #418	#EMIO 0
LED_TRAN_PIN=417	#EMIO 1
LED_CHECK_PIN=418 #416	#EMIO 2
LED_RED_PIN=420   #EMIO 4
LED_GREEN_PIN=421   #EMIO 5
LED_BULE_PIN=422   #EMIO 6

log_out()
{
	echo "$1" > /dev/null
}

_onoff()
{
    log_out "check $1 $2"
    if [ "$2" == "on" ]; then
        log_out "echo 1 > $GPIO_DIR/gpio$1/value"
        echo 1 > $GPIO_DIR/gpio$1/value
    else
        log_out "echo 0 > $GPIO_DIR/gpio$1/value"
        echo 0 > $GPIO_DIR/gpio$1/value
    fi
}

_direction_out()
{

	log_out "echo out > $GPIO_DIR/gpio$1/direction"
    echo out > $GPIO_DIR/gpio$1/direction
}

_export()
{
	if [ ! -f $GPIO_DIR/gpio$1 ]; then
		log_out "echo $1 > $GPIO_DIR/export"
        echo $1 > $GPIO_DIR/export
	fi
	
}

case $1 in
        init)
			_export $LED_POWER_PIN
			_export $LED_WORK_PIN
			_export $LED_TRAN_PIN
			_export $LED_CHECK_PIN
			_export $LED_RED_PIN
			_export $LED_GREEN_PIN
			_export $LED_BULE_PIN
            _direction_out $LED_POWER_PIN
            _direction_out $LED_WORK_PIN
            _direction_out $LED_TRAN_PIN
            _direction_out $LED_CHECK_PIN
			_direction_out $LED_RED_PIN
			_direction_out $LED_GREEN_PIN
			_direction_out $LED_BULE_PIN
                ;;
        power)
                _onoff $LED_POWER_PIN $2
                ;;
        work)
                _onoff $LED_WORK_PIN $2
                ;;
        transfer)
                _onoff $LED_TRAN_PIN $2
                ;;
        check)
                _onoff $LED_CHECK_PIN $2
                ;;
		check_ok)
				_onoff $LED_GREEN_PIN on
				_onoff $LED_RED_PIN off
                ;;
		check_faild)
				_onoff $LED_RED_PIN on
				_onoff $LED_GREEN_PIN off
                ;;
        *)
                echo "Usage: $0 {power check transfer check on/off}"
                exit 2
                ;;
esac
