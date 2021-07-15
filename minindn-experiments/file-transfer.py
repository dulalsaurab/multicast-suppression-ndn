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
from mininet.log import setLogLevel, info
from minindn.util import MiniNDNCLI
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc
from minindn.wifi.minindnwifi import MinindnWifi
from minindn.util import MiniNDNWifiCLI, getPopen
import random

def sendFile(node, prefix, file):
    info ("File published:", file)
    cmd = 'ndnputchunks {}/{} < {} > putchunks.log 2>&1 &'.format(prefix, "fname", file)
    node.cmd(cmd)
    # Sleep for appropriate time based on the file size
    sleep(5)

def receiveFile(node, prefix, filename):
    info ("Fething file: ", filename)
    cmd = 'ndncatchunks {}/{} > {} 2> catchunks.log &'.format(prefix, "fname", filename)
    node.cmd(cmd)

if __name__ == '__main__':
    setLogLevel('info')
    ndnwifi = MinindnWifi()
    args = ndnwifi.args
    testFile = "/home/mini-ndn/europa_bkp/mini-ndn/ndndump.txt"
    a = ndnwifi.net["sta1"]
    b = ndnwifi.net["sta2"]
    c = ndnwifi.net["sta3"]
    d = ndnwifi.net["sta4"]
    e = ndnwifi.net["sta5"]

    nodes = {"sta1" : "/file/temp1", "sta2":"b" , "sta3":"/file/temp2", "sta4":"d", "sta5":"e"}
    ndnwifi.start()
    info("Starting NFD")
    sleep(2)
    nfds = AppManager(ndnwifi, ndnwifi.net.stations, Nfd, logLevel='INFO')

    # start ping server at each node
    mcast = "224.0.23.170"
    producers = [ndnwifi.net["sta1"], ndnwifi.net["sta3"]]
    consumers = [y  for y in ndnwifi.net.stations if y.name not in [x.name for x in producers]]
    
    for c in consumers:
        Nfdc.registerRoute (c, "/file", mcast)

    for p in producers:
        sendFile(p, nodes[p.name], testFile)

    for c in consumers:
        sleep(random.uniform(0, 1))
        for p in producers:
            receiveFile(c, nodes[p.name], p.name+".txt")

    MiniNDNCLI(ndnwifi.net)
    ndnwifi.stop()