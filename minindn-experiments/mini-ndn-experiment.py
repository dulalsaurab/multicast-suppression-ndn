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

from minindn.helpers.ndnping import NDNPing
from time import sleep
# import environment as e

def registerRouteToAllNeighbors(ndn, host, prefixes):
    for node in ndn.net.hosts:
        for neighbor in node.connectionsTo(host):
            ip = node.IP(neighbor[0])
            Nfdc.createFace(host, ip)
            Nfdc.registerRoute(host, syncPrefix, ip)

def write_to_file(filename, _list):
    with open(filename, 'w') as f:
        for item in _list:
            f.write("%s\n" % item)

def start_nodes(ndn):
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
        for node in ndn.net.hosts:
            # if node.name == "sta1":
                # continue
            getPopen(node, "nfdc strategy set prefix {} strategy /localhost/nfd/strategy/multicast/v=4".format(p))
            sleep(0.1)

    # return path_sta1, path_sta2, path_sta3
# def start_producer(producers, prefix):
#     for p in producers:
#         print ("starting producer {}".format(p))
#         # p.cmd("nlsrc advertise /uofm")
#         sleep (1)
#         p.cmd("producer {} &> producer.log &".format(prefix))

def start_RL(ndn):
        for host in ndn.net.hosts:
            rl_path = os.path.join("/home/mini-ndn/europa_bkp/mini-ndn/sdulal/multicast-supression-ndn/src", "base.py")
            host.cmd("python {} &> reinforcement.log &".format(rl_path))
            sleep (5) 

# def start_consumer(consumers, ndn):
#     # all consumer looking for the same content
#     file_path =os.path.join(ndn.args.workDir , "sta1", "sta1_prefixes") 
#     print ("Filepath: ", file_path)

#     # for c in consumers:
#     #     name = c.name
#     #     print ("starting consumer {}".format(name))
#     #     # file_path =os.path.join(ndn.args.workDir , "sta1", "sta1_prefixes") 
#     #     # print ("Filepath: ", file_path)
#     #     # before starting consumer we need to start our reinforcement learning module
#     #     # '''
#     #     # RL CODE
#     #     rl_path = os.path.join("/home/mini-ndn/europa_bkp/mini-ndn/sdulal/multicast-supression-ndn/src", "base.py")
#     #     c.cmd("python {} &> reinforcement.log &".format(rl_path))
#     #     sleep (10)
#         # '''
#     # starting data consumer only after starting rl code, both of the consumer will look for the same data
#     for c in consumers:
#         c.cmd("consumer {} &> consumer.log &".format(file_path))
#         sleep(0.1)

def trafficServer(prefix, node, args):
    serverConfFile = '{}/{}/server_conf'.format(args.workDir, node.name)
    # Popen(['cp', '/usr/local/etc/ndn/ndn-traffic-server.conf.sample', serverConfFile], stdout=PIPE, stderr=PIPE).communicate()
    info ("Starting traffic server \n")
    with open(serverConfFile, "w") as myfile:
        myfile.write("Name=/{}/{}\n".format(prefix, node.name))
        myfile.write("Content={}{}{}\n".format(prefix, prefix, prefix))

    # c = 10, i.e maximum number of Interests to respond, d = 100, wait 100ms before responding
    cmd = 'ndn-traffic-server -c {} -d {} {} &> traffic-server.log &'.format(100, 100, serverConfFile)
    node.cmd(cmd)
    sleep(2)

def trafficClient(prefixes, node, args):
    info ("Starting ndn traffic client \n")
    # clientConfFile = '{}/{}/client_conf'.format(args.workDir, node.name)
    # # Popen(['cp', '/usr/local/etc/ndn/ndn-traffic-server.conf.sample', serverConfFile], stdout=PIPE, stderr=PIPE).communicate()
    # info ("Starting traffic client \n")
    # with open(clientConfFile, "w") as myfile:
    #     for p in prefixes:
    #         myfile.write("TrafficPercentage=50\n")
    #         myfile.write("Name={}\n".format(p[0]))
    #         myfile.write("ExpectedContent={}\n\n".format(p[1]))

    # c = 10, total number of Interests to be generated each at 100ms interval
    # cmd = 'consumer {} -i {} {} &>  traffic-client.log &'.format(prefixes[] 100, 50, clientConfFile)
    # node.cmd(cmd)

def runExperiment():
    # getPopen("rm -rf /tmp/minindn/*")
    setLogLevel('info')

    info("Starting network")
    ndnwifi = MinindnWifi()
    args = ndnwifi.args

    a = ndnwifi.net["sta1"]
    b = ndnwifi.net["sta2"]
    c = ndnwifi.net["sta3"]
    d = ndnwifi.net["sta4"]
    # e = ndnwifi.net["sta5"]
    
    nodes = {"sta1" : "a", "sta2":"b" , "sta3":"c", "sta4":"d", "sta5":"e"}
    ndnwifi.start()
    info("Starting NFD")
    sleep(2)
    nfds = AppManager(ndnwifi, ndnwifi.net.stations, Nfd, logLevel='INFO')

    # start ping server at each node
    mcast = "224.0.23.170"
    
    for node in ndnwifi.net.stations:
        for p in [x for x in list(nodes.values()) if x !=  nodes[node.name]]:
                Nfdc.registerRoute (node, "/"+p, mcast) 

    # start producer
    for node in ndnwifi.net.stations:
        node.cmd('producer /{}/{} &>  producer.log &'.format(nodes[node.name], node.name))

    # start consumer
    nInterest = 100;
    interval = 50000 # sending interest in 50ms each
    for node in ndnwifi.net.stations:
        for dest in ndnwifi.net.stations:
            if node.name != dest.name: 
                node.cmd('consumer /{}/{} {} {} &>  consumer-{}.log &'.format(nodes[dest.name], dest.name, 
                                                                                                                           nInterest, interval, nodes[dest.name]))
                # sleep(0.07)


    # Start the CLI
    MiniNDNWifiCLI(ndnwifi.net)
    ndnwifi.net.stop()
    ndnwifi.cleanUp()

if __name__ == '__main__':
    try:
        runExperiment()
    except Exception as e:
        MinindnWifi.handleException()
