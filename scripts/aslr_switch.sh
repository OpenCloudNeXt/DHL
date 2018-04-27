#this script is used to enable/disable the Address Space Layout Randomization (ASLR)

printf "Do you want to enable or disable the ASLR ? 1:enable   2:disable \n"
read -p "(default:2):" answer
[ -z ${answer} ] && answer="2"
if [ "${answer}" == "1" ]; then
	sysctl -w kernel.randomize_va_space=2
	echo
	echo -e "ASLR has been enabled"
	echo
else
	sysctl -w kernel.randomize_va_space=0
	echo
	echo -e "ASLR has been disabled"
	echo
fi

#echo "the value of 'kernel.randomize_va_space' is :"
#sysctl -n kernel.randomize_va_space