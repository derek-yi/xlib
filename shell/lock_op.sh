## https://blog.csdn.net/jsyxcjw/article/details/48830701

###############################################################
## check lock
###############################################################
LOCK_NAME="/run/fpga_op.lock"
wait_cnt=0
set -o noclobber
(echo "$$" > "$LOCK_NAME") 2> /dev/null
until (( $? == 0 || $wait_cnt > 40 ))
do
    sleep 0.05
    #echo -e "c\c"
	wait_cnt=$(($wait_cnt + 1))
	(echo "$$" > "$LOCK_NAME") 2> /dev/null
done
trap 'rm -f "$LOCK_NAME"; exit $?' INT TERM EXIT

###############################################################
## critical process
###############################################################
if [[ $1 == "power_down_nxp" ]]; then
    power_down_nxp
elif [[ $1 == "power_up_nxp" ]]; then
    power_up_nxp
elif [[ $1 == "set_led_ds37" ]]; then
    set_led_ds37 $2
elif [[ $1 == "set_led_ds39" ]]; then
    set_led_ds39 $2	
fi



###############################################################
## cleanup lock
###############################################################
rm -f $LOCK_NAME
trap - INT TERM EXIT
set +o noclobber


