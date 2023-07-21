# -*- Mode:python; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
#
# Copyright (C) 2015-2020, The University of Memphis,
#                          Arizona Board of Regents,
#                          Regents of the University of California.
#
# This file is part of Mini-NDN.
# See AUTHORS.md for a complete list of Mini-NDN authors and contributors.
#
# Mini-NDN is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Mini-NDN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Mini-NDN, e.g., in COPYING.md file.
# If not, see <http://www.gnu.org/licenses/>.

from mn_wifi.topo import Topo
from mininet.log import setLogLevel, info
from mininet.node import CPULimitedHost
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc
from minindn.wifi.minindnwifi import MinindnWifi
from minindn.util import MiniNDNWifiCLI, getPopen
from mn_wifi.link import wmediumd, mesh
from mn_wifi.wmediumdConnector import interference


from time import sleep
import subprocess
import shutil
import os
import sys
import random
import itertools


_F_NAME = "transfer"

all_nodes = {
    "sta1": {"position": (10,10,0)},
    "sta2": {"position": (30,10,0)},
    "sta3": {"position": (30,30,0)},
    "sta4": {"position": (10,30,0)},
    "sta5": {"position": (0,0,0)},
    "sta6": {"position": (40,40,0)},
    "sta7": {"position": (0,20,0)},
    "sta8": {"position": (0,40,0)},
    "sta9": {"position": (20,40,0)},
    "sta10": {"position": (40,20,0)},
    "sta11": {"position": (40,0,0)},
    "sta12": {"position": (20,0,0)},
    "sta12": {"position": (20,10,0)}
}

accessPoint = {"ap1": {"position": (20,20,0), "range": 100, "mode":'g'}}
# accessPoint = {"ap1": {"position": (20,20,0), "range": 100}}

# ap1 = net.addAccessPoint('ap1', ssid='ssid-ap1', mode='g', channel='1', position='50,50,0')

def get_first_n_items(dictionary, n):
    return dict(itertools.islice(dictionary.items(), n))


def changeBandwith(node, newBW=5):
  for intf in node.intfList(): # loop on interfaces of node
    info( ' %s:'%intf )
    if intf.link: # get link that connects to interface(if any)
        intfs = [intf.link.intf1, intf.link.intf2] #intfs[0] is source of link and intfs[1] is dst of link
        print (type(intf), type(intfs[0]), type(intfs[1]), dir(intfs[0]))
        try:
            intfs[0].config(bw=newBW)
            intfs[1].config(bw=newBW)
        except Exception as e:
            print (e)
    else:
        info('\n failed to change bandwidth' )

def runTshark(node):
    # node.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 56363 or udp port 6363" -w {}.pcap &> /dev/null &'.format(node.name))
    node.cmd('tshark -o ip.check_checksum:FALSE -ni any -f "udp port 56363 or udp port 6363" -w {}.pcap &> /dev/null &'.format(node.name))

def sendFile(node, prefix, file):
    
    fname = file.split("/")[-1].split(".")[0]
    publised_under = "{}/{}".format(prefix, fname)
    info ("Publishing file: {}, under name: {} \n".format(fname, publised_under))
    # ndn6-file-server /prefix /directory
    cmd = 'ndnputchunks -s 1100 {} < {} > putchunks.log 2>&1 &'.format(publised_under, file)
    # cmd = 'ndn6-file-server {} {} 2> putchunks.log 2>&1 &'.format(publised_under, file)

    node.cmd(cmd)
    # Sleep for appropriate time based on the file size
    sleep(10)

def receiveFile(node, prefix, filename):
    info ("Fething file: {} \n".format(filename))
    # cmd = 'ndncatchunks -v -p cubic --init-cwnd 10 --log-cwnd log-cwnd --log-rtt log-rtt {}/{} > {} 2> catchunks-{}.log &'.format(prefix, _F_NAME, filename, filename)
    cmd = 'ndncatchunks -v -p cubic --log-cwnd log-cwnd --log-rtt log-rtt {}/{} > {} 2> catchunks-{}.log &'.format(prefix, _F_NAME, filename, filename)
    node.cmd(cmd)

if __name__ == '__main__':

    setLogLevel('info')
    subprocess.run(["mn", "-c"])
    sleep (1)
    shutil.rmtree('/tmp/minindn/', ignore_errors=True)
    os.makedirs("/tmp/minindn/", exist_ok=True)

    topo = Topo()

    if len(sys.argv) < 2:
        print ("Missing argument for the number of consumers")
        exit()
    
    number_of_consumers = int(sys.argv[1])

    nodes = get_first_n_items(all_nodes, number_of_consumers+1)

    # this topo setup only works for this experiment, 1 ap and x number of stations
    _s = []
    _ap = []
    for station in nodes:
        _s.append(topo.addStation(name=station, position=nodes[station]['position']))

    for ap in accessPoint:
        _ap.append(topo.addAccessPoint(ap, **accessPoint[ap]))

    optsforlink1  = {'bw':54, 'delay':'5ms', 'loss':0, 'mode': 'g'}

    for s in _s:
        topo.addLink(s, _ap[0], **optsforlink1)
        # topo.addLink(s, _ap[0], delay='5ms')

    ndnwifi = MinindnWifi(topo=topo)

    args = ndnwifi.args
    args.ifb = True
    args.host = CPULimitedHost
    
    testFile = "/home/map901/sdulal/multicast-suppression-ndn/files/{}.dat".format(_F_NAME)
    # testFile = "/home/mini-ndn/europa_bkp/mini-ndn/sdulal_new/multicast-supression-ndn/files/output.dat"
    i = 1024
    for node in ndnwifi.net.stations:
        # runTshark(node) # run tshark on nodes

        i = i*2
        cmd = "echo {} > {}".format(i, "seed")
        node.cmd(cmd)
        node.cmd("sysctl net.ipv4.ipfrag_time = 10")
        node.cmd("sysctl net.ipv4.ipfrag_high_thresh = 26214400")
    sleep(1)

    producers_prefix = {"sta1" : "/producer/sta1"} #, "sta2":"/producer/sta2"} # , "sta3":"/producer/sta3", "sta4":"/producer/sta4"}

    ndnwifi.start()
    info("Starting NFD")
    nfds = AppManager(ndnwifi, ndnwifi.net.stations, Nfd, logLevel='DEBUG')
    sleep(2)

    mcast = "224.0.23.170"

    producers = [ndnwifi.net[x] for x in producers_prefix.keys()]
    # producers = [ndnwifi.net["sta1"]]
    consumers = [y for y in ndnwifi.net.stations if y.name not in [x.name for x in producers]]

    print ("producers: ", producers)
    print ("consumers: ", consumers)

    for c in consumers:
        Nfdc.registerRoute (c, "/producer", mcast)
        sleep(2)

    for p in producers:
        changeBandwith(p, 1)

    for p in producers:
        # changeBandwith(p, 1)
        # sleep (1)
        sendFile(p, producers_prefix[p.name], testFile)


    # ------------ consumer grouping start ------------------------------
    # random.shuffle(consumers)
    # consumerGroupA = consumers[:2]
    # consumerGroupB = consumers[2:] if len(consumers) > 2 else []
    
    # g = [consumerGroupA, consumerGroupB]
    
    # for ind, p in enumerate(producers):
    #     _consumers = g[ind]
    #     for c in _consumers:
    #         receiveFile(c, producers_prefix[p.name], p.name+".txt")


    # ------------ consumer slow start ------------------------------
    # random.shuffle(consumers)
    # consumerGroupA = consumers[:2]
    # consumerGroupB = consumers[2:] if len(consumers) > 2 else []
    # g = [consumerGroupA, consumerGroupB]
    
    # for p in producers:
    #     for c in consumers[0:len(consumers)-1]:
    #         receiveFile(c, producers_prefix[p.name], p.name+".txt")
    
    # sleep (3)
    # receiveFile(consumers[len(consumers)-1], "/producer/sta1", "sta1.txt")
    # ------------ consumer slow start end ------------------------------


    # for p in producers:
    #     for c in consumers:
    #         receiveFile(c, producers_prefix[p.name], p.name+".txt")
    #     sleep(5)

    for c in consumers:
        # sleep(random.uniform(0, 1) % 0.02) # sleep at max 20ms
        for p in producers:
            receiveFile(c, producers_prefix[p.name], p.name+".txt")

    sleep(60)

    # cmd1 = "tc qdisc show";
    # # cmd1 = "tc qdisc show";
    # sta1 = ndnwifi.net["sta1"]
    # # sta2 = ndnwifi.net["sta2"]

    # print("tc qdisc show before", sta1.cmd(cmd1))


    # print("tc after", sta1.cmd(cmd1))


    # MiniNDNWifiCLI(ndnwifi.net)
    ndnwifi.stop()
