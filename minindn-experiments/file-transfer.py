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

from time import sleep
import subprocess

from mn_wifi.topo import Topo
from mininet.log import setLogLevel, info
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc
from minindn.wifi.minindnwifi import MinindnWifi
from minindn.util import MiniNDNWifiCLI, getPopen
import random

from mn_wifi.link import wmediumd, mesh
# from mn_wifi.cli import CLI
# from mn_wifi.net import Mininet_wifi
from mn_wifi.wmediumdConnector import interference


nodes = {
    "sta1": {"position": (0,0,0)}, 
    "sta2": {"position": (10,0,0)}, 
    "sta3": {"position": (30,0,0)},
    "sta4": {"position": (0,30,0)}, 
    "sta5": {"position": (30,30,0)}, 
    "sta6": {"position": (10,30,0)}, 
    "sta7": {"position": (0,20,0)}, 
    "sta8": {"position": (30,20,0)}
}

accessPoint = {"ap1": {"position": (20,20,0), "range": 100}}

def changeBandwith(node, newBW=5):
  for intf in node.intfList(): # loop on interfaces of node
    #info( ' %s:'%intf )
    if intf.link: # get link that connects to interface(if any)
        intfs = [intf.link.intf1, intf.link.intf2] #intfs[0] is source of link and intfs[1] is dst of link
        try:
            intfs[0].config(bw=newBW)
            intfs[1].config(bw=newBW)
        except Exception as e:
            print (e)
    else:
        info( ' \n' )

def runTshark(node):
    # node.cmd('tshark -o ip.defragment:TRUE -o ip.check_checksum:FALSE -ni any -f "udp port 56363 or udp port 6363" -w {}.pcap &> /dev/null &'.format(node.name))
    node.cmd('tshark -o ip.check_checksum:FALSE -ni any -f "udp port 56363 or udp port 6363" -w {}.pcap &> /dev/null &'.format(node.name))

def sendFile(node, prefix, file):
    info ("File published: {}\n".format(file))

    # ndn6-file-server /prefix /directory
    cmd = 'ndnputchunks {}/{} < {} > putchunks.log 2>&1 &'.format(prefix, "fname", file)
    # cmd = 'ndn6-file-server /{}/{} /{} 2> putchunks.log 2>&1 &'.format(prefix, "fname", file)

    node.cmd(cmd)
    # Sleep for appropriate time based on the file size
    sleep(10)

def receiveFile(node, prefix, filename):
    info ("Fething file: {} \n".format(filename))
    cmd = 'ndncatchunks -v -p cubic --init-cwnd 10 --log-cwnd log-cwnd --log-rtt log-rtt {}/{} > {} 2> catchunks.log &'.format(prefix, "fname", filename)
    node.cmd(cmd)


if __name__ == '__main__':
    setLogLevel('info')

    topo = Topo()

    # this topo setup only works for this experiment, 1 ap and x number of stations
    _s = []
    _ap = []
    for station in nodes:
        _s.append(topo.addStation(name=station, position=nodes[station]['position']))

    for ap in accessPoint:
        _ap.append(topo.addAccessPoint(ap, **accessPoint[ap]))

    for s in _s:
        topo.addLink(s, _ap[0], delay='5ms')

    ndnwifi = MinindnWifi(topo=topo)

    args = ndnwifi.args

    testFile = "/home/map901/sdulal/multicast-suppression-ndn/files/output.dat"
    # testFile = "/home/mini-ndn/europa_bkp/mini-ndn/sdulal_new/multicast-supression-ndn/files/output.dat"
    i = 1024
    for node in ndnwifi.net.stations:
        runTshark(node) # run tshark on nodes

        i = i*2
        cmd = "echo {} > {}".format(i, "seed")
        node.cmd(cmd)
        node.cmd("sysctl net.ipv4.ipfrag_time = 10")
        node.cmd("sysctl net.ipv4.ipfrag_high_thresh = 26214400")
    sleep(1)

    nodes = {"sta1" : "/file/temp1", "sta2":"b" , "sta3":"/file/temp2", "sta4":"d", "sta5":"e"}
    ndnwifi.start()
    info("Starting NFD")
    nfds = AppManager(ndnwifi, ndnwifi.net.stations, Nfd, logLevel='DEBUG')
    sleep(2)

    mcast = "224.0.23.170"
    producers = [ndnwifi.net["sta1"]] #, ndnwifi.net["sta3"]]
    consumers = [y for y in ndnwifi.net.stations if y.name not in [x.name for x in producers]]
    print ("consumer", consumers)
    for c in consumers:
        Nfdc.registerRoute (c, "/file", mcast)
    sleep(2)

    for p in producers:
        sendFile(p, nodes[p.name], testFile)

    for c in consumers:
        # sleep(random.uniform(0, 1) % 0.02) # sleep at max 20ms
        for p in producers:
            receiveFile(c, nodes[p.name], p.name+".txt")

    sleep(1)

    # cmd1 = "tc qdisc show";
    # cmd1 = "tc qdisc show";
    # sta1 = ndnwifi.net["sta1"]
    # sta2 = ndnwifi.net["sta2"]

    # print(sta1.cmd(cmd1))
    # changeBandwith(sta1, 6)
    # print(sta1.cmd(cmd1))

    MiniNDNWifiCLI(ndnwifi.net)
    ndnwifi.stop()
