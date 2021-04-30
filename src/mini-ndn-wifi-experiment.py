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

def write_to_file(filename, _list):
    with open(filename, 'w') as f:
        for item in _list:
            f.write("%s\n" % item)

def start_nodes(ndn):
    # set all the prefixes to multicast strategy
    # advertise prefixes from the respective node
    # create route from each other -- no need because all the prefixes are set to multicast environment
    # run ping server
    sta1_prefixes = e.generate_prefixes('/uofm/',  'abcdefgh', 20)
    # sta2_prefixes = e.generate_prefixes('/mit/',  'ijklmnox', 20)
    # sta3_prefixes = e.generate_prefixes('/ucla/',  'pqrstuvw', 20)

    path_sta1 = os.path.join(ndn.args.workDir , "sta1", "sta1_prefixes")
    path_sta2 = os.path.join(ndn.args.workDir , "sta2", "sta2_prefixes")
    path_sta3 = os.path.join(ndn.args.workDir , "sta3", "sta3_prefixes")

    write_to_file(path_sta1, sta1_prefixes)
    # write_to_file(path_sta2, sta2_prefixes)
    # write_to_file(path_sta3, sta3_prefixes)

    for p in ["/uofm", "/mit", "/ucla"]:
        for node in ndn.net.stations:
            if node.name == "sta1":
                continue
            getPopen(node, "nfdc route add {} udp4://224.0.23.170:56363".format(p))
            # getPopen(sta2, "nfdc route add {} udp4://224.0.23.170:56363".format(p))
            # getPopen(sta3, "nfdc route add {} udp4://224.0.23.170:56363".format(p))
            sleep (0.1)

            # getPopen(sta1, "nfdc strategy set prefix {} strategy /localhost/nfd/strategy/multicast/v=4".format(p))
            # getPopen(sta2, "nfdc strategy set prefix {} strategy /localhost/nfd/strategy/multicast/v=4".format(p))
            getPopen(node, "nfdc strategy set prefix {} strategy /localhost/nfd/strategy/multicast/v=4".format(p))
            sleep(0.1)

    # return path_sta1, path_sta2, path_sta3
def start_producer(producers):
    for p in producers:
        print ("starting producer {}".format(p))
        rl_path = os.path.join("/home/mini-ndn/europa_bkp/mini-ndn/sdulal/multicast-supression-ndn/src", "base.py")
        p.cmd("python {} &> reinforcement.log &".format(rl_path))
        sleep (10) 
        p.cmd("producer uofm &> producer.log &")

def start_consumer(consumers, ndn):
    # all consumer looking for the same content
    file_path =os.path.join(ndn.args.workDir , "sta1", "sta1_prefixes") 
    print ("Filepath: ", file_path)

    for c in consumers:
        name = c.name
        print ("starting consumer {}".format(name))
        # file_path =os.path.join(ndn.args.workDir , "sta1", "sta1_prefixes") 
        # print ("Filepath: ", file_path)
        # before starting consumer we need to start our reinforcement learning module
        # '''
        # RL CODE
        rl_path = os.path.join("/home/mini-ndn/europa_bkp/mini-ndn/sdulal/multicast-supression-ndn/src", "base.py")
        c.cmd("python {} &> reinforcement.log &".format(rl_path))
        sleep (10)
        # '''
    # starting data consumer only after starting rl code, both of the consumer will look for the same data
    for c in consumers:
        c.cmd("consumer {} &> consumer.log &".format(file_path))
        sleep(0.1)

def runExperiment():
    # getPopen("rm -rf /tmp/minindn/*")
    setLogLevel('info')

    info("Starting network")
    ndnwifi = MinindnWifi()
    sta1 = ndnwifi.net["sta1"]
    sta2 = ndnwifi.net["sta2"]
    sta3 = ndnwifi.net["sta3"]
    sta4 = ndnwifi.net["sta4"]
    sta5 = ndnwifi.net["sta5"]
    sta6 = ndnwifi.net["sta6"]
    sta7 = ndnwifi.net["sta7"]

    ndnwifi.start()
    info("Starting NFD")
    sleep(2)
    nfds = AppManager(ndnwifi, ndnwifi.net.stations, Nfd, logLevel='INFO')
    start_nodes(ndnwifi)

    # start consumer
    start_producer([sta1])
    start_consumer([sta2, sta3, sta4, sta5, sta6, sta7], ndnwifi)

    # Start the CLI
    MiniNDNWifiCLI(ndnwifi.net)
    ndnwifi.net.stop()
    ndnwifi.cleanUp()

if __name__ == '__main__':
    try:
        runExperiment()
    except Exception as e:
        MinindnWifi.handleException()
