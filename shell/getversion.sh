#!/bin/bash
#######################################################################
echo "-------------------------------------------------------------------------------"
if [[ -f /run/fpga_load_ok ]]; then
	pcb=$(devmem 0x280030018)
	let pcb='pcb >> 28'
	pcb=$(((~pcb) & 0x7))
	echo "PCB   : V1.$pcb"
	echo "FPGA  : $(devmem 0x280030000)"
fi

echo "PHY   : $(head -1 /home/o5g/phy/version.txt)"
echo "MAC   : $(head -1 /home/o5g/oam/version)"
echo "APP   : $(head -1 /home/upgrade/version.txt)"
echo ""
echo "UBOOT : $(dd if=/dev/mmcblk0p9 | strings | grep 'U-Boot 2015.07' -m1)"
echo "KERNEL: $(uname -a)"

echo "-------------------------------------------------------------------------------"
#######################################################################
