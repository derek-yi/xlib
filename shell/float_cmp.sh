
##aaa=3.0

# || [$(`echo "$1 > 4.0" |bc`) -eq 1]

if [[ ($(bc <<< "$1 < 2.0") -eq 1) || ($(bc <<< "$1 > 4.0") -eq 1) ]]; then
	echo "Error: Frequency [$1] is out of range."
else
	echo "set PA frequency...$ADRV9009_ARGV"
fi
