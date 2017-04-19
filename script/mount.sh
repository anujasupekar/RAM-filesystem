path=$1
size=$2
if [[ -z $2 ]]; then
	size=16
fi

./ramdisk $path $size
