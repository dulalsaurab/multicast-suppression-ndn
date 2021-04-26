#!/home/mini-ndn/miniconda/envs/tf/bin/python python
import sys
import os
from time import sleep
from random import randrange
from sys import exit
from subprocess import Popen, PIPE, call
import subprocess
from traceback import format_exc

from mininet.log import setLogLevel, info
from minindn.minindn import Minindn
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc
from minindn.apps.nlsr import Nlsr
from minindn.helpers.ndn_routing_helper import NdnRoutingHelper
from minindn.wifi.minindnwifi import MinindnWifi
from minindn.util import MiniNDNWifiCLI, getPopen

from minindn.helpers.ndnpingclient import NDNPingClient
from time import sleep

import environment as e

def start_nodes(sta1, sta2, sta3):
    # set all the prefixes to multicast strategy
    # advertise prefixes from the respective node
    # create route from each other -- no need because all the prefixes are set to multicast environment
    # run ping server
    sta1_prefixes = e.generate_prefixes('/uofm/',  'abcdefgh', 5)
    sta2_prefixes = e.generate_prefixes('/mit/',  'ijklmnox', 5)
    sta3_prefixes = e.generate_prefixes('/ucla/',  'pqrstuvw', 5)

    getPopen(sta1, "nfdc strategy set prefix /uofm strategy /localhost/nfd/strategy/multicast/v=4")
    getPopen(sta2, "nfdc strategy set prefix /mit strategy /localhost/nfd/strategy/multicast/v=4") 
    getPopen(sta3, "nfdc strategy set prefix /ucla strategy /localhost/nfd/strategy/multicast/v=4")
    sleep(2)

    # start ping server for all the prefix in the node 
    for p in sta1_prefixes:
        sleep(0.1)
        getPopen(sta1, "ndnpingserver {} &".format(p))
    
    for p in sta2_prefixes:
        sleep(0.1)
        getPopen(sta2, "ndnpingserver {} &".format(p))

    for p in sta3_prefixes:
        sleep(0.1)
        getPopen(sta3, "ndnpingserver {} &".format(p))

    sleep (5) #sleep to start all the pingservers

def runExperiment():
    setLogLevel('info')

    info("Starting network")
    ndnwifi = MinindnWifi()
    sta1 = ndnwifi.net["sta1"]
    sta2 = ndnwifi.net["sta2"]
    sta3 = ndnwifi.net["sta3"]

    ndnwifi.start()
    info("Starting NFD")
    sleep(2)
    nfds = AppManager(ndnwifi, ndnwifi.net.stations, Nfd)

    start_nodes(sta1, sta2, sta3)
    # Start the CLI
    MiniNDNWifiCLI(ndnwifi.net)
    ndnwifi.net.stop()
    ndnwifi.cleanUp()

if __name__ == '__main__':
    try:
        runExperiment()
    except Exception as e:
        MinindnWifi.handleException()
