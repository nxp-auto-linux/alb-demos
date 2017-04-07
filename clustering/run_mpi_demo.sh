# Copyright 2017 NXP
#!/bin/bash

# IP of the boards we'll be running MPI on
PCIE1=192.168.10.2
PCIE2=192.168.10.3
PCIE3=192.168.10.4
# In summary:
MPI_MASTER=${PCIE1}
declare -a MPI_HOSTS=($PCIE1 $PCIE2 $PCIE3)
declare -a LIVE_HOSTS=($MPI_MASTER)

NUM_CPUS=4
MAX_PROCESSES=0	# to be updated in set_up() routine

MPI_HOSTS_FILE=_hosts
MPI_ROOT=/home/root
# Will be evaluated 
MPI_CMD=""

SSH_OPTS="-q -o ""ConnectTimeout=2"" -o ""StrictHostKeyChecking=no"""
################################################################################

function global_set_up()
{
	# Clear "demo_***:<date>***" leftovers from the previous tests
	rm -f demo*board* result_so_far.ppm solved.ppm c.png

    # ...but we still need an empty stub for the convenience of our Python scripts
    touch demo_1_board

    # screen resolution magik
    fbset "1280x720-60"
}

function iter_set_up()
{
    MAX_PROCESSES=0
    LIVE_HOSTS=($MPI_MASTER)

	for host in ${MPI_HOSTS[@]}; do
		# Clear arp caches
		if [ $USER == root ]; then
			arp -d $host
		else
			sudo arp -d $host
		fi

		# Kill any remaining processes from previous runs
		ssh $SSH_OPTS $host killall -q mpiexec mpi_demo
	done

	# Check which hosts are alive. While at it, build the _hosts
	# configuration file. Create a new file on each interation.
	echo \# `date` > ${MPI_HOSTS_FILE}
	echo ${MPI_MASTER} >> ${MPI_HOSTS_FILE}
	for host in ${MPI_HOSTS[@]}; do
		if [ $host == ${MPI_MASTER} ]; then
			echo "*** Host $host is MPI master ***"
			# it's already been added
			continue
		fi

		if check_host "root@${host}"; then
			echo "*** Host $host is alive ***"
			LIVE_HOSTS+=(${host})
			# Update the _hosts file
			echo \# `date` >> ${MPI_HOSTS_FILE}
			echo ${host} >> ${MPI_HOSTS_FILE}
		else
			echo "--- Host $host is not alive ---"
		fi
	done
	# We must allow 1 CPU slack for the master process
	MAX_PROCESSES=$((${#LIVE_HOSTS[@]} * ${NUM_CPUS} - 1))
}

function iter_tear_down()
{
	# Kill any possibly remaining processes
	for host in ${LIVE_HOSTS[@]}; do
		ssh $SSH_OPTS killall -q mpiexec mpi_demo
	done
}

# Check if host ${1} is alive by attempting a simple remote command over ssh
function check_host()
{
	local host=$1

	ssh $SSH_OPTS $host echo || return 1
	return 0
}

# Compose a MPI command to run remotely. This function is basically only a
# convenient mechanism for deferred evaluation of ${MPI_CMD}.
function mpi_command()
{
	local __np=$1
	local __ifile=$2
	local __size=$3
	local __hosts=""

	for host in ${LIVE_HOSTS[@]}; do
		if [ "${host}" == "$MPI_MASTER" ]; then
			__hosts+=${host}
		else
			__hosts+=,${host}
		fi
	done
	
	MPI_CMD="time mpiexec -n ${__np} -host ${__hosts} -wdir /home/root ./mpi_demo ${__ifile} ${__size}"
}

################################################################################

function run_demo()
{
    iter_set_up
    # MAX_PROCESSES has been determined by set_up()
    i=${MAX_PROCESSES}
    # add 1 for the master process
   	np=$((1 + i))
    # Only measure with the boards fully loaded
   	if [ $((np % 4)) != 0 ]; then
        return
   	fi

    # compose ${MPI_CMD}
    mpi_command $np input.ppm 100	# hard-coded input file and stride
    # do the actual dew
    echo bash -lc "${MPI_CMD}"
    bash -lc "${MPI_CMD}"

    iter_tear_down
}

global_set_up
while [ 1 ]; do
    run_demo
    sleep 5
done
