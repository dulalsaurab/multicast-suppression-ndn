import sys
from os import listdir
from os.path import isfile, join
from os import walk
from tkinter import N
from xml.dom import InvalidCharacterErr
from collections import defaultdict
from matplotlib.pyplot import axis
import pandas as pd
import statistics

# consumers = ["sta2", "sta3"]
# consumers = ["sta2", "sta3", "sta4"]
consumers = ["sta2", "sta3", "sta4", "sta5"]

w_sup_capture = {"send": ["is in flight, drop the forwarding", "not in flight, waiting"] , "receive": "Multicast data received:", "indices": [[0,4], [0,-1]]}
w_o_sup_capture = {"send": ["onOutgoingInterest out=258"], "receive": "onIncomingData in=(258", "indices": [[],[]]}
g_capture = {"wos": w_o_sup_capture, "ws": w_sup_capture}

def get_diff(ts1, ts2):
    return float(ts2) - float(ts1)

def get_name_ts_ws(line, indices):
    line = line.split()
    ts = line[indices[0]]
    name = line[indices[1]]
    return ts, name

def get_name_ts_wos(line):
    line = line.split()
    ts = line[0]
    try:
        name = line[-1].split("interest=")[1]
    except:
        name = line[-1].split("data=")[1]
    return ts, name

def get_name_ts(line, type, indices=""):
    if type == "ws":
        return get_name_ts_ws(line, indices)
    elif type == "wos":
        return get_name_ts_wos(line)


def compute_delay(root_dir, type):
    result = defaultdict()
    capture = g_capture[type]

    for c in consumers:
        nfd_log = "{}/{}/log/nfd.log".format(root_dir, c)
        node_res = []
        with open(nfd_log) as f:
            lines = f.readlines()
            ask = {}
            got = {}
            for line in lines:
                # get send timestamp
                # if capture["send"] in line:
                if any(line.find(x) > -1 for x in capture["send"]):
                    ts, name = get_name_ts(line, type, capture["indices"][0])
                    ask[name] = ts

                # get received timestamp
                if capture["receive"] in line:
                    ts, name = get_name_ts(line, type, capture["indices"][1])
                    got[name] = ts

            for name in ask:
                seq = name.split("seg=")[-1]

                if seq not in result:
                    result[seq] = []

                try:
                    result[seq].append(get_diff(ask[name], got[name]))
                except Exception as e:
                    result[seq].append(-1)

    return result
    # df = pd.DataFrame(result)
    # df = df.T
    # headers =  ["B", "C", "D"]
    # df.columns = headers
    # df['avg'] = df[["B", "C", "D"]].mean(axis=1)
    # return df

def main():
    ws_dir = sys.argv[1]
    wos_dir = sys.argv[2]
    # type = sys.argv[3]
    res_ws = compute_delay(ws_dir, "ws") 
    res_wos = compute_delay(wos_dir, "wos")
    
    res_dict = {}
    for seq in res_ws:
        if seq in res_wos:
            # compute stretch (average of all consumer) ws/wos
            ws_avg = statistics.mean(res_ws[seq])
            wos_avg = statistics.mean(res_wos[seq])
            stretch = ws_avg/wos_avg
            try:
                res_dict[int(seq)] = [int(seq), ws_avg, wos_avg, stretch]
            except:
                # removing something that is not seq (/file/temp1/fname/32=metadata') --- this
                pass
    df = pd.DataFrame(res_dict)
    df = df.T
    headers =  ["seq", "ws", "wos", "stretch"]
    df.columns = headers
    df.sort_values("seq", inplace=True)
    df.to_csv("check00", encoding='utf-8')

main()