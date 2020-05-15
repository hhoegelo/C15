#! /bin/sh

# first we sleep.
# this gives plently of time for linux to mess with the RTC
# (where does it do that set?) before we touch it. Otherwise it
# occasionally updates it right between when we are doing our sets.

# This also gives you a chance to abort in case something goes wrong
# and avoid a infinite loop


sleep 30

if [ "$(phyreg test 18 13)" = "1" ]; then

        sleep 1

        NOW=$(date +%s)
        echo "chkphy:Rebooting NOW=$NOW"
        echo "Rebooting $NOW" >>/var/tmp/chkphy.log
        # make sure that log message actually gets written before we pull the plug
        sync

        /usr/sbin/bbb-long-reset

        while true;
          do echo waiting to reboot
          sleep 10
        done
fi

echo "chkphy:eth0 good"
