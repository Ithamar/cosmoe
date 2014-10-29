# check for the common mistake of not being root
# this will change one day, but for now root is required
if [ `whoami` != "root" ]
then
    echo "ERROR: You must execute this script as root."
    echo "       Read the README file for more information."
    exit 1
fi

# remove stale shared memory segments
clean_shm.sh

# Using devfs without devfsd
#if [ ! -r /dev/fb0 ]; then
#	echo " *** /dev/fb0 does not exist..."
#	echo " *** Linking /dev/fb/0 /dev/fb0"
#	echo
#	ln -s /dev/fb/0 /dev/fb0
#fi
if [ -f /var/run/gpm.pid ]; then
	echo " *** GPM is running, please stop it before continuing"
	echo
	exit
fi

# start appserver, registrar and a demo app
#appserver > server.out 2>&1 &
appserver &
sleep 2

registrar > registrar.out 2>&1 &
sleep 1

cterm > cterm.out 2>&1
#guido > guido.out 2>&1
#desktop > client.out 2>&1

# if we arrived here, the application has terminated
#killall guido
killall cterm
killall registrar
killall appserver
sleep 1

# if something won't die normally, try harder to kill it
#killall -9 guido
killall -9 cterm
killall -9 registrar
killall -9 appserver

# remove stale shared memory segments
clean_shm.sh
