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

from mininet.log import setLogLevel, info
import mn_wifi
from mn_wifi.topo import Topo
from mn_wifi.topo import LinearWirelessTopo
# from mn_wifi.topo import Topo as topo

from minindn.wifi.minindnwifi import MinindnWifi
from minindn.util import MiniNDNWifiCLI, getPopen
from minindn.apps.app_manager import AppManager
from minindn.apps.nfd import Nfd
from minindn.helpers.nfdc import Nfdc

from mn_wifi.link import wmediumd, mesh
from mn_wifi.wmediumdConnector import interference

from time import sleep
# This experiment uses the singleap topology and is intended to be a basic
# test case where we see if two nodes can send interests to each other.
def runExperiment():
    setLogLevel('info')
    
    topo = Topo()

    sta1 = topo.addStation(name="sta1", position=('0,0,0'))
    sta2 = topo.addStation(name="sta2", position='30,0,0')
    sta3 = topo.addStation(name="sta3", position='10,0,0')
    ap1 = topo.addAccessPoint('ap1', position='0,0,0')

    params = {'delay': '5ms'}
    topo.addLink(sta1, ap1, delay='10ms')
    topo.addLink(sta2, ap1, delay='10ms')
    topo.addLink(sta3, ap1, delay='10ms')
    # topo.addLink(sta3, ap1, delay='5ms')
    
    ndnwifi = MinindnWifi(link=wmediumd, topo=topo)
    args = ndnwifi.args

    info("Starting NFD")
    nfds = AppManager(ndnwifi, ndnwifi.net.stations, Nfd, logLevel='DEBUG')
    sleep(2)
    
    MiniNDNWifiCLI(ndnwifi.net)
    ndnwifi.stop()



# def topology():
#     "Create a network."
#     net = Mininet_wifi(link=wmediumd, wmediumd_mode=interference)

#     info("*** Creating nodes\n")
#     sta1 = net.addStation('sta1', mac='00:00:00:00:00:11', position='1,1,0')
#     sta2 = net.addStation('sta2', mac='00:00:00:00:00:12', position='31,11,0')
#     ap1 = net.addAccessPoint('ap1', wlans=2, ssid='ssid1', position='10,10,0')
#     ap2 = net.addAccessPoint('ap2', wlans=2, ssid='ssid2', position='30,10,0')
#     c0 = net.addController('c0')

#     info("*** Configuring nodes\n")
#     net.configureNodes()

#     info("*** Associating Stations\n")
#     net.addLink(sta1, ap1)
#     net.addLink(sta2, ap2)
#     net.addLink(ap1, intf='ap1-wlan2', cls=mesh, ssid='mesh-ssid', channel=5)
#     net.addLink(ap2, intf='ap2-wlan2', cls=mesh, ssid='mesh-ssid', channel=5)

#     info("*** Starting network\n")
#     net.build()
#     c0.start()
#     ap1.start([c0])
#     ap2.start([c0])

#     info("*** Running CLI\n")
#     CLI(net)

#     info("*** Stopping network\n")
#     net.stop()


if __name__ == '__main__':
    try:
        runExperiment()
    except Exception as e:
        MinindnWifi.handleException()