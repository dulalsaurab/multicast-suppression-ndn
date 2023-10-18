
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

# rl_dir="/home/Desktop/saurabLatestJuly/new-suppression-rl"
# rl_dir="/home/bidhya/Desktop/saurabLatestJuly/new-suppression-rl"
# rl_dir="/home/bidhya/Desktop/saurabLatestJuly/bidhya-rl"
# rl_dir = "/home/bidhya/Desktop/saurabLatestJuly/without-rl"
rl_dir = "/home/bidhya/Desktop/saurabLatestJuly/simple-embedding-rl"


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

def write_to_file(filename, _list):
  with open(filename, 'w') as f:
    for item in _list:
      f.write("%s\n" % item)

# def start_RL(ndn):
#   for host in ndn.net.hosts:
#     rl_path = os.path.join("/home/mini-ndn/europa_bkp/mini-ndn/sdulal/multicast-supression-ndn/src", "base.py")
#     host.cmd("python {} &> reinforcement.log &".format(rl_path))
#     sleep (5) 


# takes all_nodes dictionary and number of consumers and returns the a new dictionary containing the first n items from the input dictionary

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
    subprocess.run(["mn", "-c"]) # ensures that any existing Mininet network is terminated before the script proceeds with setting up a new network topology.
    sleep (1)
    shutil.rmtree('/tmp/minindn/', ignore_errors=True) # utility functions in python to remove a directory tree
    os.makedirs("/tmp/minindn/", exist_ok=True)

    topo = Topo() # creates an instance of the Topo class, 
    #which is provided by the Mininet-WiFi library. The Topo class is used to define the network topology in a Mininet-WiFi simulation.

    if len(sys.argv) < 2:
        print ("Missing argument for the number of consumers")
        exit()
    
    number_of_consumers = int(sys.argv[1])

    nodes = get_first_n_items(all_nodes, number_of_consumers+1) # get n+1 nodes from all nodes

    # this topo setup only works for this experiment, 1 ap and x number of stations
    _s = []
    _ap = []
    for station in nodes:
        _s.append(topo.addStation(name=station, position=nodes[station]['position']))

    for ap in accessPoint:
        _ap.append(topo.addAccessPoint(ap, **accessPoint[ap]))

    optsforlink1  = {'bw':54, 'delay':'5ms', 'loss':0, 'mode': 'g'} # bandwidth of link, delay of the link, packet loss percentage

    for s in _s:
        topo.addLink(s, _ap[0], **optsforlink1) #adds a wireless link between the current station s and the first access point (_ap[0]).
        # topo.addLink(s, _ap[0], delay='5ms')

        #creates a network topology with wireless stations and an access point. 
        # It then adds wireless links between each station and the first access point, with link characteristics defined by the optsforlink1 dictionary



    # creating instance of MinindnWifi with wireless network topology defined by the topo object.
    ndnwifi = MinindnWifi(topo=topo)

    args = ndnwifi.args
    args.ifb = True # enabling itermediate functional block which is linux kernel module used for traffic shaping and quality of service puropose
    args.host = CPULimitedHost # host used in network emulation is a host class that limits the CPU usaage of the emulated hosts, 
    #useful for simulating scenarios with resource-constrained devices.
    
    testFile = "/home/bidhya/Desktop/saurabLatestJuly/files/{}.dat".format(_F_NAME)
    # testFile = "/home/mini-ndn/europa_bkp/mini-ndn/sdulal_new/multicast-supression-ndn/files/output.dat"
    i = 1024
    for node in ndnwifi.net.stations:
        # runTshark(node) # run tshark on nodes
        i = i*2
        cmd = "echo {} > {}".format(i, "seed") #creates a shell command that writes the value of i to a file named "seed". The file "seed" will have the value of i (e.g., 2048, 4096, 8192, etc.).
        node.cmd(cmd) #execute the command on the station. This allows running Linux shell commands on the emulated hosts.
        node.cmd("sysctl net.ipv4.ipfrag_time = 10") #sets the value of the net.ipv4.ipfrag_time parameter to 10 on the station.
        node.cmd("sysctl net.ipv4.ipfrag_high_thresh = 26214400") #sets the value of the net.ipv4.ipfrag_high_thresh parameter to 26214400 on the station. This parameter is related to the high threshold for IPv4 packet fragmentation
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

    rl_path = os.path.join(rl_dir, "base.py")
    print("RL path: ", rl_path)

    for c in consumers:
        Nfdc.registerRoute (c, "/producer", mcast)
        sleep(2)
        # c.cmd("python {} &> reinforcement.log &".format(rl_path))
        c.cmd("/usr/bin/python {} > reinforcement-{}.log 2>&1 &".format(rl_path, c.name))
        sleep(10)

    for p in producers:
        # changeBandwith(p, 1)
        # sleep (1)
        print ("starting producer {}".format(p))
        p.cmd("/usr/bin/python {} > reinforcement-{}.log 2>&1 &".format(rl_path, p.name))

        sleep(10)
        sendFile(p, producers_prefix[p.name], testFile)

    for c in consumers:
        # sleep(random.uniform(0, 1) % 0.02) # sleep at max 20ms
        for p in producers:
            receiveFile(c, producers_prefix[p.name], p.name+".txt")


    MiniNDNWifiCLI(ndnwifi.net)
    # sleep(60)
    ndnwifi.stop()
