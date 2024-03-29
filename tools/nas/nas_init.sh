#!/bin/bash

echo " * Version: 3.0"

# LOG:
# 3.0:
#   1. first all-in-one script to include: affinity, bonding, nas_init
#   2. PAGE_SIZE automatic set
#   3. LINK_NUM support to identify number of links
#   4. prints cleanup and better flow status report
#   5. Error prints added
#   6. Exit on error
# 2.5:
#   1. XFS mount options updated.
#   2. RAID1 chunk size updated.
#   3. splice enabled for XFS.
# 2.4:
#   1. support RAID-1
#   2. splice enabled by default 
# 2.3:
#   1. fixing the mkdir share for loop
# 2.2:
#   1. automatic size settings for PARTSIZE
# 2.1:
#   1. setting coal to 100 in net queue 0 and fixing for in NETQ


#
# t= topology [sd|rd0|rd1|rd5]
# p= prepare drives (fdisk)
# m= mkfs+mdadm
# b= burn new rootfs to /dev/sda1
# f= file system format (ext4/xfs/btrfs)
# s= for SSD medium
# l= for largepage support (64k)
# h= for HDDs number (4/8) in RAID5 configuration

PREPARE_HDD="no"
ZAP_ROOTFS="no"
MKFS="no"
TOPOLOGY="rd5"
HDD_NUM="4"
ARCH=`uname -m`
FS="ext4"
NFS="no"
SSD="no"
SYSDISKEXIST="no"
CPU_COUNT=`grep -c ^processor /proc/cpuinfo`
COPY_CMD="/bin/cp"
ROOTFS_DEVICE="/dev/nfs"
LARGE_PAGE=`/usr/bin/getconf PAGESIZE`
PARTNUM="1"
LINK_NUM=`dmesg |grep "Giga ports" |awk '{print $2}'`
PARTSIZE="55GB"
NETQ="0"

MNT_DIR="/mnt/public"
mkdir -p $MNT_DIR
chmod 777 $MNT_DIR


ARGV0=$(basename ${0} | cut -f1 -d'.')
STAMP="[$(date +%H:%M-%d%b%Y)]:"
echo -e "Script parameters has been updated! please read help!"

function do_error_hold {
    echo -e "\n*************************** Error ******************************\n"
    echo -e "${STAMP} Error: ${1}"
    echo -ne "Press Enter continue..."
    echo -e "\n****************************************************************\n"
    read TEMP
    exit 2
}

function do_error {
    echo -e "\n*************************** Error ******************************\n"
    echo -e "${STAMP}: ${1}"
    echo -e "\n****************************************************************\n"
    exit 1
}

function do_error_reboot {
    echo -e "\n*************************** Error ******************************\n"
    echo -e "${STAMP}: ${1}"
    echo -e "\n****************************************************************\n"
    reboot
}

if [ "$CPU_COUNT" == "0" ]; then
    CPU_COUNT="1"
fi

# verify supporting arch
case "$ARCH" in
    armv5tel | armv6l | armv7l)	;;
    *)	do_error_hold "Architecture ${ARCH} unsupported" ;;
esac

while getopts "l:pbmzn:sf:t:h:" flag; do
    case "$flag" in
	f)
	    FS=$OPTARG
	    case "$OPTARG" in
		ext4|btrfs|xfs|NONE)	echo "Filesystem: ${OPTARG}" ;;
		*)			do_error "Usage: file-system: ext4|xfs|btrfs" ;;
	    esac
	    ;;
	
	s)	SSD="yes"
	    ;;
	
	b)	ZAP_ROOTFS="yes"
	    ROOTFS_TARBALL="$OPTARG"
	    ;;
	
	m)	MKFS="yes"
	    ;;

	z)	SYSDISKEXIST="yes"
	    ;;
	
	n)	PARTNUM="$OPTARG"
	    ;;
	
	p)	PREPARE_HDD="yes"
	    ;;
	
	l)	LINK_NUM=$OPTARG
	    case "$OPTARG" in
		1|2|4) ;;
		*)	do_error "Usage: links options: 1|2|4" ;;
	    esac
	    ;;
	
	t)	TOPOLOGY=$OPTARG
	    case "$OPTARG" in
	sd|rd0|rd1|rd5) ;;
		*)	do_error "Usage: drive toplogy: sd|rd0|rd1|rd5" ;;
	    esac
	    ;;
	
	h)	HDD_NUM=$OPTARG
	    case "$OPTARG" in
		4|8) ;;
		*)	do_error "Usage: drive toplogy: 4|8" ;;
	    esac
	    ;;
	
	*)	echo "Usage: $0"
	    echo "           -f <ext4|xfs|btrfs|fat32>: file system type ext4, xfs, btrfs or fat32"
	    echo "           -t <sd|rd0|rd1|rd5>: drive topology"
	    echo "           -n <num>: partition number to be mounted"
	    echo "           -m: mkfs/mdadm"
	    echo "           -p: prepare drives"
	    echo "           -h <num>: number of HDDs to use"
	    echo "           -l <num>: number of links to use"
	    echo "           -b <rootfs_tarball_path>:  path to rootfs tarball to be placed on /dev/sda2"
	    exit 1
	    ;;
    esac
done

echo -ne "************** System info ***************\n"
echo -ne "    Topology:      "
case "$TOPOLOGY" in
    sd)	echo -ne "Single drive\n" ;;
    rd0)	echo -ne "RAID0\n" ;;
    rd1)	echo -ne "RAID1\n" ;;
    rd5)	echo -ne "RAID5\n" ;;
    *)	do_error "Invalid Topology" ;;
esac

echo -ne "    Filesystem:    $FS\n"
echo -ne "    Arch:          $ARCH\n"
echo -ne "    Cores:         $CPU_COUNT\n"
echo -ne "    Links:         $LINK_NUM\n"
echo -ne "    Page Size:     $LARGE_PAGE\n"
echo -ne "    Disk mount:    $SYSDISKEXIST\n"
echo -ne "    * Option above will used for the system configuration\n      if you like to change them, please pass different parameters to the nas_init.sh\n"
if [ "$MKFS" == "yes" ]; then
    case "$FS" in
	xfs)	[ ! -e "$(which mkfs.xfs)" ]	&&	do_error "missing mkfs.xfs in rootfs (aptitude install xfsprogs)" ;;
	ext4)	[ ! -e "$(which mkfs.ext4)" ]	&&	do_error "missing mkfs.ext4 in rootfs (aptitude install e2fsprogs)" ;;
	fat32)	[ ! -e "$(which mkdosfs)" ]	&&	do_error "missing mkdosfs in rootfs (aptitude install dosfstools)" ;;
	btrfs)	[ ! -e "$(which mkfs.btrfs)" ]	&&	do_error "missing mkfs.btrfs in rootfs (aptitude install btrfs-tools)" ;;
	NONE)						echo "no filesystem specified" ;;
	*)						do_error "no valid filesystem specified" ;;
    esac
fi
echo -ne "******************************************\n"
[[ "$TOPOLOGY" == "rd0" || "$TOPOLOGY" == "rd1" || "$TOPOLOGY" == "rd5" ]] && [ ! -e "$(which mdadm)" ] && do_error "missing mdadm in rootfs (aptitude install mdadm)"

#if [[ -e "$(which smbd)" && -e "$(which nmbd)" ]]; then
#    echo -n "* Starting Samba daemons"
#    $(which smbd) && $(which nmbd)
#else

echo -ne " * Stopping SAMBA processes:   "
if [ "$(pidof smbd)" ]; then
    killall smbd
fi

if [ "$(pidof nmbd)" ]; then
    killall nmbd
fi

sleep 2

if [ `ps -ef |grep smb |grep -v grep |wc -l` != 0 ]; then
    do_error "Unable to stop Samba processes"
fi
echo -ne "[Done]\n"

echo -ne " * Unmounting $MNT_DIR:            "
if [ `mount | grep $MNT_DIR | grep -v grep | wc -l` != 0 ]; then
    umount $MNT_DIR
fi

sleep 2

#if [ `mount | grep mnt | grep -v grep | wc -l` != 0 ]; then
#    do_error "Unable to unmount /mnt"
#fi
echo -ne "[Done]\n"

#[ -e /dev/md_d0 ] && mdadm --manage --stop /dev/md_d0
#[ -e /dev/md_d1 ] && mdadm --manage --stop /dev/md_d1

if [ "$TOPOLOGY" == "sd" ]; then
    if [ "$SYSDISKEXIST" == "yes" ]; then
	DRIVES="b"
    else
	DRIVES="a"
    fi
    PARTSIZE="55GB"
elif [ "$TOPOLOGY" == "rd0" ]; then
    if [ "$SYSDISKEXIST" == "yes" ]; then
	DRIVES="b c"
    else
	DRIVES="a b"
    fi
    PARTSIZE="30GB"
elif [ "$TOPOLOGY" == "rd1" ]; then
    if [ "$SYSDISKEXIST" == "yes" ]; then
	DRIVES="b c"
    else
	DRIVES="a b"
    fi
    PARTSIZE="55GB"
elif [ "$TOPOLOGY" == "rd5" ]; then
    if [ "$HDD_NUM" == "8" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e f g h i"
	else
	    DRIVES="a b c d e f g h"
	fi
	PARTSIZE="10GB"
    elif [ "$HDD_NUM" == "4" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e"
	else
	    DRIVES="a b c d"
	fi
	PARTSIZE="20GB"
    fi
fi

# Checking drives existence 
#for drive in `echo $DRIVES`; do 
 #   if [ `fdisk -l |grep "Disk /dev/sd$drive" |grep -v grep |wc -l` == 0 ]; then
#	do_error "Drives assigned (/dev/sd$drive) is not persent!"
 #   fi
#done

echo -ne " * Preparing disks partitions: "
if [ "$PREPARE_HDD" == "yes" ]; then
    [ ! -e "$(which fdisk)" ] && do_error "missing fdisk in rootfs"
    
    set -o verbose
    if [ "$SSD" == "no" ]; then
	for drive in `echo $DRIVES`; do echo -e "c\no\nn\np\n1\n4096\n+${PARTSIZE}\nt\n83\nw\n" | fdisk -u /dev/sd${drive}; done
    else
	for drive in `echo $DRIVES`; do echo -e "c\no\nn\np\n1\n63\n+${PARTSIZE}\nt\n83\nw\n" | fdisk  -H 224 -S 56 /dev/sd${drive}; done
#		for drive in `echo $DRIVES`; do echo -e "c\no\nn\np\n1\n2\n+${PARTSIZE}\nt\n83\nw\n" | fdisk -H 32 -S 32 /dev/sd${drive}; done
    fi
    set +o verbose
    fdisk -ul
    
    blockdev --rereadpt /dev/sd${drive} || do_error_reboot "The partition table has been altered, please reboot device and run nas_init again"
fi
echo -ne "[Done]\n"

if [ "$ZAP_ROOTFS" == "yes" ]; then
    [ ! -e "$ROOTFS_TARBALL" ] && do_error "missing rootfs tarball, use option -b <rootfs_tarball_path>"
    
    mkfs.ext3 ${ROOTFS_DEVICE}
    mount ${ROOTFS_DEVICE} $MNT_DIR
    cd $MNT_DIR
    $COPY_CMD "$ROOTFS_TARBALL" | tar xpjf -

	# overwrite tarball files with NFS based files
	#cp /etc/hostname           /mnt/etc/hostname
	#cp /usr/sbin/nas_init.sh   /mnt/usr/sbin/
	#cp /etc/network/interfaces /mnt/etc/network/interfaces
    cd /root
    umount $MNT_DIR
    sleep 2
fi
echo -ne " * Network setup:              "
if [ "$LINK_NUM" == "2" ]; then
    set -o verbose
    ifconfig eth0 0.0.0.0 down
    ifconfig eth1 0.0.0.0 down

    ifconfig bond0 192.168.0.5 netmask 255.255.255.0 up
    ifenslave bond0 eth0 eth1

    ifconfig bond0 down
    echo balance-alb > /sys/class/net/bond0/bonding/mode
    ifconfig bond0 up
    set +o verbose
elif [ "$LINK_NUM" == "4" ]; then
    set -o verbose
    ifconfig eth0 0.0.0.0 down
    ifconfig eth1 0.0.0.0 down
    ifconfig eth2 0.0.0.0 down
    ifconfig eth3 0.0.0.0 down

    ifconfig bond0 192.168.0.5 netmask 255.255.255.0 up
    ifenslave bond0 eth0 eth1 eth2 eth3

    ifconfig bond0 down
    echo balance-alb > /sys/class/net/bond0/bonding/mode
    ifconfig bond0 up
    set +o verbose
elif [ "$LINK_NUM" == "1" ]; then
    set -o verbose
    ifconfig eth0 192.168.0.5 netmask 255.255.255.0 up
    set +o verbose
fi
echo -ne "[Done]\n"

echo -ne " * Network optimization:       "

if [ "$ARCH" == "armv5tel" ]; then
# KW section, LSP_5.1.3
    set -o verbose
    mv_eth_tool -txdone 8
    mv_eth_tool -txen 0 1
    mv_eth_tool -reuse 1
    mv_eth_tool -recycle 1
    mv_eth_tool -lro 0 1
    mv_eth_tool -rxcoal 0 160
    set +o verbose
elif [[ "$ARCH" == "armv6l" || "$ARCH" == "armv7l" ]]; then
# AXP / KW40
    set -o verbose
    echo 1 > /sys/devices/platform/neta/gbe/skb
    for (( j=0; j<$LINK_NUM; j++ )); do
	ethtool  --offload eth$j gso on
        ethtool  --offload eth$j tso on

	# following offload lower NAS BM, we do not use it
	#ethtool  --offload eth$j gro on

        # set RX coalecing to zero on all port 0 queues
	for (( i=0; i<=$NETQ; i++ )); do
	    echo "$j $i 100" > /sys/devices/platform/neta/gbe/rxq_time_coal
	done
	    
	ethtool -k eth$j > /dev/null
    done
    set +o verbose
else
    do_error "Unsupported ARCH=$ARCH"
fi
echo -ne "[Done]\n"
sleep 2

if [ "$TOPOLOGY" == "rd5" ]; then
    echo -ne " * Starting RAID5 build:       "
    for drive in `echo $DRIVES`; do PARTITIONS="${PARTITIONS} /dev/sd${drive}${PARTNUM}"; done

    set -o verbose

    for drive in `echo $DRIVES`; do echo -e 1024 > /sys/block/sd${drive}/queue/read_ahead_kb; done
    
    if [ "$MKFS" == "yes" ]; then
	[ -e /dev/md0 ] && mdadm --manage --stop /dev/md0

	for partition in `echo $DRIVES`; do mdadm --zero-superblock /dev/sd${partition}1; done

	if [ "$SSD" == "no" ]; then
	    echo "y" | mdadm --create -c 128 /dev/md0 --level=5 -n $HDD_NUM --force $PARTITIONS
	else
			# most SSD use eraseblock of 512, so for performance reasons we use it
	    echo "y" | mdadm --create -c 512 /dev/md0 --level=5 -n $HDD_NUM --force $PARTITIONS
	fi

	sleep 2

	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to create RAID device"
	fi

	if [ "$FS" == "ext4" ]; then
	    if [ "$SSD" == "no" ]; then
		if [ "$HDD_NUM" == "8" ]; then
		    mkfs.ext4 -j -m 0 -T largefile -b 4096 -E stride=32,stripe-width=224 -F /dev/md0
		elif [ "$HDD_NUM" == "4" ]; then
		    mkfs.ext4 -j -m 0 -T largefile -b 4096 -E stride=32,stripe-width=96 -F /dev/md0
		fi
	    else
		mkfs.ext4 -j -m 0 -T largefile -b 4096 -E stride=128,stripe-width=384 -F /dev/md0
	    fi
	elif [ "$FS" == "xfs" ]; then
	    mkfs.xfs -f /dev/md0
	elif [ "$FS" == "btrfs" ]; then
	    mkfs.btrfs /dev/md0
	fi
    else
		# need to reassemble the raid
	mdadm --assemble /dev/md0 --force $PARTITIONS

	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to assemble RAID device"
	fi
    fi

    if [ "$FS" == "ext4" ]; then
	mount -t ext4 /dev/md0 $MNT_DIR -o noatime,data=writeback,barrier=0,nobh
    elif [ "$FS" == "xfs" ]; then
	mount -t xfs /dev/md0 $MNT_DIR -o noatime,nodirspread
    elif [ "$FS" == "btrfs" ]; then
	mount -t btrfs /dev/md0 $MNT_DIR -o noatime
    fi

    if [ `mount | grep $MNT_DIR | grep -v grep | wc -l` == 0 ]; then
	do_error "Unable to mount FS"
    fi

    if [ "$LARGE_PAGE" == "65536" ]; then
	echo 256 > /sys/block/md0/md/stripe_cache_size
    else
	echo 4096 > /sys/block/md0/md/stripe_cache_size
    fi
    /bin/echo -e 4096 > /sys/block/md0/queue/read_ahead_kb

    for drive in `echo $DRIVES`; do echo noop > /sys/block/sd${drive}/queue/scheduler; done
    echo 4 > /proc/sys/vm/dirty_ratio
    echo 2 > /proc/sys/vm/dirty_background_ratio
    set +o verbose
    echo -ne "[Done]\n"

elif [ "$TOPOLOGY" == "rd1" ]; then
    echo -ne " * Starting RAID1 build:       "
    for drive in `echo $DRIVES`; do PARTITIONS="${PARTITIONS} /dev/sd${drive}${PARTNUM}"; done

    set -o verbose

    for drive in `echo $DRIVES`; do echo -e 1024 > /sys/block/sd${drive}/queue/read_ahead_kb; done

    if [ "$MKFS" == "yes" ]; then
	[ -e /dev/md0 ] && mdadm --manage --stop /dev/md0

	for partition in `echo $DRIVES`; do mdadm --zero-superblock /dev/sd${partition}1; done
	echo "y" | mdadm --create -c 128 /dev/md0 --level=1 -n 2 --force $PARTITIONS

	sleep 2

	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to create RAID device"
	fi

	if [ "$LARGE_PAGE" == "65536" ]; then
	    echo 256 > /sys/block/md0/md/stripe_cache_size
	else
	    echo 4096 > /sys/block/md0/md/stripe_cache_size
	fi

	if [ "$FS" == "ext4" ];	then
	    mkfs.ext4 -j -m 0 -T largefile -b 4096 -E stride=32,stripe-width=32 -F /dev/md0
	elif [ "$FS" == "btrfs" ]; then
	    mkfs.btrfs -m raid1 -d raid1 /dev/md0
	fi
    else
		# need to reassemble the raid
	mdadm --assemble /dev/md0 --force $PARTITIONS

	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to assemble RAID device"
	fi
    fi

    if [ "$FS" == "ext4" ]; then
	mount -t ext4 /dev/md0 $MNT_DIR -o noatime,data=writeback,barrier=0
    elif [ "$FS" == "btrfs" ]; then
	mount -t btrfs /dev/md0 $MNT_DIR -o noatime
    fi

    if [ `mount | grep $MNT_DIR | grep -v grep | wc -l` == 0 ]; then
	do_error "Unable to mount FS"
    fi

    for drive in `echo $DRIVES`; do /bin/echo -e 1024 > /sys/block/sd${drive}/queue/read_ahead_kb; done
    /bin/echo -e 2048 > /sys/block/md0/queue/read_ahead_kb
    echo 4 > /proc/sys/vm/dirty_ratio
    echo 2 > /proc/sys/vm/dirty_background_ratio
    set +o verbose
    echo -ne "[Done]\n"

elif [ "$TOPOLOGY" == "rd0" ]; then
    echo -ne " * Starting RAID0 build:       "

    for drive in `echo $DRIVES`; do PARTITIONS="${PARTITIONS} /dev/sd${drive}${PARTNUM}"; done

    set -o verbose

    for drive in `echo $DRIVES`; do echo -e 1024 > /sys/block/sd${drive}/queue/read_ahead_kb; done

    if [ "$MKFS" == "yes" ]; then
	if [ -e /dev/md0 ]; then
	    mdadm --manage --stop /dev/md0
	fi
	for partition in `echo $DRIVES`; do mdadm --zero-superblock /dev/sd${partition}1; done
	if [ "$SSD" == "no" ]; then
	    echo "y" | mdadm --create -c 256 /dev/md0 --level=0 -n 2 --force $PARTITIONS
	else
	    echo "y" | mdadm --create -c 256 /dev/md0 --level=0 -n 2 --force $PARTITIONS
	fi
	    
	sleep 2

	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to create RAID device"
	fi
	  
	echo 256 > /sys/block/md0/md/stripe_cache_size
	if [ "$FS" == "ext4" ]; then
	    if [ "$SSD" == "no" ]; then
		mkfs.ext4 -j -m 0 -T largefile -b 4096 -E stride=64,stripe-width=128 -F /dev/md0
	    else
		mkfs.ext4 -j -m 0 -T largefile -b 4096 -E stride=64,stripe-width=128 -F /dev/md0
	    fi
	elif [ "$FS" == "btrfs" ]; then
	    mkfs.btrfs -m raid0 -d raid0 /dev/md0
	fi
    else
	mdadm --assemble /dev/md0 --force $PARTITIONS
	
	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to create RAID device"
	fi
    fi


    if [ "$FS" == "ext4" ]; then
	mount -t ext4 /dev/md0 $MNT_DIR -o noatime,data=writeback,barrier=0
    elif [ "$FS" == "btrfs" ]; then
	mount -t ext4 /dev/md0 $MNT_DIR -o noatime
    fi

    if [ `mount | grep $MNT_DIR | grep -v grep | wc -l` == 0 ]; then
	do_error "Unable to mount FS"
    fi

    for drive in `echo $DRIVES`; do /bin/echo -e 1024 > /sys/block/sd${drive}/queue/read_ahead_kb; done
    echo -e 4096 > /sys/block/md0/queue/read_ahead_kb

    set +o verbose
    echo -ne "[Done]\n"
elif [ "$TOPOLOGY" == "sd" ]; then
    PARTITIONS="/dev/sd${DRIVES}${PARTNUM}"
    echo -ne " * Starting single disk:       "
    set -o verbose
    
    echo -e 1024 > /sys/block/sd${DRIVES}/queue/read_ahead_kb

    if [ "$FS" == "fat32" ]; then
		# wait for DAS recongnition by udev
	sleep 5
	if [ "$MKFS" == "yes" ]; then
	    [ -e /dev/md0 ] && mdadm --manage --stop /dev/md0
	    mkdosfs -s 64 -S 4096 -F 32 $PARTITIONS
	fi
	mount -t vfat -o umask=000,rw,noatime,nosuid,nodev,noexec,async $PARTITIONS $MNT_DIR
    elif [ "$FS" == "ext4" ]; then
	if [ "$MKFS" == "yes" ]; then
	    [ -e /dev/md0 ] && mdadm --manage --stop /dev/md0
	    
	    mkfs.ext4 -j -m 0 -b 4096 -O large_file,extents -F $PARTITIONS
	fi
	mount -t ext4 $PARTITIONS $MNT_DIR -o noatime,data=writeback,barrier=0
    elif [ "$FS" == "btrfs" ]; then
	if [ "$MKFS" == "yes" ]; then
	    [ -e /dev/md0 ] && mdadm --manage --stop /dev/md0
	    mkfs.btrfs -m single -d single $PARTITIONS
	fi
	
	mount -t btrfs $PARTITIONS $MNT_DIR -o noatime
    else
	if [ "$MKFS" == "yes" ]; then
	    [ -e /dev/md0 ] && mdadm --manage --stop /dev/md0
	    mkfs.xfs -f $PARTITIONS
	fi
	mount -t xfs $PARTITIONS $MNT_DIR -o noatime,nodirspread
    fi

    if [ `mount | grep $MNT_DIR | grep -v grep | wc -l` == 0 ]; then
	do_error "Unable to mount FS"
    fi

    set +o verbose
    echo -ne "[Done]\n"
fi

sleep 2

chmod 777 /mnt/
mkdir -p /mnt/usb
chmod 777 /mnt/usb

for (( i=0; i<4; i++ )); do
    mkdir -p /mnt/public/share$i
    chmod 777 /mnt/public/share$i
done

# Samba
if [ "$FS" != "NONE" ]; then
    echo -ne " * Starting Samba daemons:     "
    if [[ -e "$(which smbd)" && -e "$(which nmbd)" ]]; then
	chmod 0755 /var/lock
	rm -rf /etc/smb.conf
	touch  /etc/smb.conf
	echo '[global]' 				>>  /etc/smb.conf
	echo ' netbios name = debian-armada'		>>  /etc/smb.conf
	echo ' workgroup = WORKGROUP'			>>  /etc/smb.conf
	echo ' server string = debian-armada'		>>  /etc/smb.conf
	echo ' encrypt passwords = yes'			>>  /etc/smb.conf
	echo ' security = user'				>>  /etc/smb.conf
	echo ' map to guest = bad password'		>>  /etc/smb.conf
	echo ' use mmap = yes'				>>  /etc/smb.conf
	echo ' use sendfile = yes'			>>  /etc/smb.conf
	echo ' dns proxy = no'				>>  /etc/smb.conf
	echo ' max log size = 200'			>>  /etc/smb.conf
	echo ' log level = 0'				>>  /etc/smb.conf
	echo ' socket options = IPTOS_LOWDELAY TCP_NODELAY SO_SNDBUF=131072 SO_RCVBUF=131072' >>  /etc/smb.conf
	echo ' local master = no'			>>  /etc/smb.conf
	echo ' dns proxy = no'				>>  /etc/smb.conf
	echo ' ldap ssl = no'				>>  /etc/smb.conf
	echo ' create mask = 0666'			>>  /etc/smb.conf
	echo ' directory mask = 0777'			>>  /etc/smb.conf
	echo ' show add printer wizard = No'		>>  /etc/smb.conf
	echo ' printcap name = /dev/null'		>>  /etc/smb.conf
	echo ' load printers = no'			>>  /etc/smb.conf
	echo ' disable spoolss = Yes'			>>  /etc/smb.conf
	echo ' max xmit = 131072'			>>  /etc/smb.conf
	echo ' disable netbios = yes'			>>  /etc/smb.conf
	echo ' csc policy = disable'			>>  /etc/smb.conf
	if [ "$FS" == "btrfs" ]; then
			# crash identified with these FS
	    echo '# min receivefile size = 128k'	>>  /etc/smb.conf
	    echo '# strict allocate = yes'		>>  /etc/smb.conf
	else
	    echo ' min receivefile size = 128k'	>>  /etc/smb.conf
	    echo ' strict allocate = yes'		>>  /etc/smb.conf
	fi
	echo ' allocation roundup size = 10485760'	>>  /etc/smb.conf
	echo ''						>>  /etc/smb.conf
	echo '[public]'					>>  /etc/smb.conf
	echo ' comment = my public share'		>>  /etc/smb.conf
	echo ' path = /mnt/public'			>>  /etc/smb.conf
	echo ' writeable = yes'				>>  /etc/smb.conf
	echo ' printable = no'				>>  /etc/smb.conf
	echo ' public = yes'				>>  /etc/smb.conf
	echo ''						>>  /etc/smb.conf
	echo ''						>>  /etc/smb.conf
	echo '[usb]'					>>  /etc/smb.conf
	echo ' comment = usb share'			>>  /etc/smb.conf
	echo ' path = /mnt/usb'				>>  /etc/smb.conf
	echo ' writeable = yes'				>>  /etc/smb.conf
	echo ' printable = no'				>>  /etc/smb.conf
	echo ' public = yes'				>>  /etc/smb.conf
	echo ''						>>  /etc/smb.conf

	rm -rf /var/log/log.smbd
	rm -rf /var/log/log.nmbd
	$(which nmbd) -D -s /etc/smb.conf
	$(which smbd) -D -s /etc/smb.conf
	sleep 1
	echo -ne "[Done]\n"
    else
	sleep 1
	echo "[Skip]\n"
    fi
fi

echo -ne " * Setting up affinity:        "
if [[ "$ARCH" == "armv6l" || "$ARCH" == "armv7l" ]]; then
    if [ "$CPU_COUNT" = "4" ]; then
	set -o verbose
	
		# XOR Engines
	echo 1 > /proc/irq/51/smp_affinity
	echo 2 > /proc/irq/52/smp_affinity
	echo 4 > /proc/irq/94/smp_affinity
	echo 8 > /proc/irq/95/smp_affinity

		# ETH
	echo 1 > /proc/irq/8/smp_affinity
	echo 2 > /proc/irq/10/smp_affinity
	echo 4 > /proc/irq/12/smp_affinity
	echo 8 > /proc/irq/14/smp_affinity
	
		# SATA
	echo 2 > /proc/irq/55/smp_affinity

	# PCI-E SATA controller
	echo 4 > /proc/irq/99/smp_affinity
	echo 8 > /proc/irq/103/smp_affinity

	set +o verbose
    elif [ "$CPU_COUNT" == "2" ]; then
	set -o verbose
	
		# XOR Engines
	echo 1 > /proc/irq/51/smp_affinity
	echo 2 > /proc/irq/52/smp_affinity
	echo 1 > /proc/irq/94/smp_affinity
	echo 2 > /proc/irq/95/smp_affinity

		# ETH
	echo 1 > /proc/irq/8/smp_affinity
	echo 2 > /proc/irq/10/smp_affinity
	
		# SATA
	echo 2 > /proc/irq/55/smp_affinity
	
		# PCI-E SATA controller
	echo 2  > /proc/irq/99/smp_affinity
	
	set +o verbose
    fi
fi
echo -ne "[Done]\n"
case "$TOPOLOGY" in
    rd1 | rd5)
		#watch "cat /proc/mdstat|grep finish"
	;;
esac

echo -ne "\n==============================================\n"
