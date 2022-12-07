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

capture = {"send": ["onIncomingInterest in=261 interest=/file", "onIncomingInterest in=262 interest=/file"] , "receive": ["onIncomingData matching=/file/"], "indices": [[],[]]}
# w_o_sup_capture = {"send": ["onIncomingInterest in=261 interest=/file", "onIncomingInterest in=262 interest=/file"] , "receive": ["onIncomingData matching=/file/"], "indices": [[],[]]}
# w_o_sup_capture = {"send": ["onIncomingInterest in=261"], "receive": "onIncomingData in=(258", "indices": [[],[]]}
# g_capture = {"wos": w_o_sup_capture, "ws": w_sup_capture}

def get_diff(ts1, ts2):
    return float(ts2) - float(ts1)

def get_name_ts(line, type):
    line = line.split()
    ts = line[0]
    if type == "sent":
        name = line[-1].split("interest=")[1]
    else:
        name = line[-1].split("matching=")[1]
    return ts, name


def process_chunks_log(root_dir, consumers):
    final_result = defaultdict(lambda: defaultdict(list))
    cp_dir_list = defaultdict(list)
    for sup_dir in listdir(root_dir):
        sup_d = root_dir+sup_dir
        result = defaultdict(list)

        for cp_dir in listdir(sup_d):
            take_d = sup_d+"/"+cp_dir
            _c = int([*cp_dir][2])
            cp_dir_list[sup_dir].append(cp_dir)
            consumers = ["sta{}".format(x+1) for x in range(1,_c+1)]
            
            print(take_d)
            exit()

            for take_dir_name in listdir(take_d):
                for c in consumers:
                    chunk_log = "{}/{}/{}/catchunks.log".format(take_d, take_dir_name, c)

                    with open(chunk_log) as f:
                        lines = f.readlines()
                        for line in lines:
                            if "Time elapsed:" in line:
                                result['totaltime'].append(float(line.split()[2]))

                            if "Goodput" in line:
                                result['goodput'].append(float(line.split()[1]))
                            
                            if "Retransmitted segments" in line:
                                result['retrans'].append(float(line.split()[2]))

            for key in result:
                final_result[sup_dir][key].append(statistics.mean(result[key]))
    return final_result

def compute_delay(root_dir, consumers, type):

    final_result = defaultdict(list)

    for take_dir in listdir(root_dir):
        result = defaultdict(list)
        print(take_dir)
        for c in consumers:
            nfd_log = "{}/{}/{}/log/nfd.log".format(root_dir, take_dir, c)
            node_res = defaultdict()
            with open(nfd_log) as f:
                lines = f.readlines()
                ask = defaultdict(list)
                got = defaultdict(list)
                for line in lines:
                    # get send timestamp
                    if any(line.find(x) > -1 for x in capture["send"]):
                        ts, name = get_name_ts(line, "sent")
                        ask[name].append(ts)

                    # get received timestamp
                    if any(line.find(x) > -1 for x in capture["receive"]):
                        ts, name = get_name_ts(line, "received")
                        got[name].append(ts)

                # only consider minimun of ask and maximum of got
                for name in ask:
                    seq = name.split("seg=")[-1]
                    try:
                        tmp = []
                        # ts_ask = max(ask[name])
                        ts_got = min(got[name])

                        for ts in ask[name]:
                            tmp.append(get_diff(ts, ts_got))

                        # delay = get_diff(ts_ask, ts_got)
                        result[seq].append(min(tmp))

                    except Exception as e:
                        print(e, name, ask[name], got[name])
                        # print (seq, name, e)

            # final_result = {}
            for seq in result:
                final_result[seq].append(statistics.mean(result[seq])) # average from all the nodes

    for_return = {}
    for key in final_result:
        for_return[key] = statistics.median(final_result[key])

    return for_return

def get_latency(ws_dir, wos_dir, consumers):

    res_ws = compute_delay(ws_dir, consumers, "ws") 
    res_wos = compute_delay(wos_dir, consumers, "wos")

    res_dict = {}
    for seq in res_ws:
        if seq in res_wos:
            # compute stretch (average of all consumer) ws/wos
            # ws_avg = statistics.mean(res_ws[seq])
            # wos_avg = statistics.mean(res_wos[seq])
            stretch = res_ws[seq]/res_wos[seq]
            try:
                # normalied by RTT
                res_dict[int(seq)] = [int(seq), res_ws[seq]/0.01, res_wos[seq]/0.01, stretch]
            except:
                # removing something that is not seq (/file/temp1/fname/32=metadata') --- this
                pass
    df = pd.DataFrame(res_dict)
    df = df.T
    headers =  ["seq", "ws", "wos", "stretch"]
    df.columns = headers
    df.sort_values("seq", inplace=True)

    _95th_percentile = df.stretch.quantile(.95)
    print(_95th_percentile)

    revise_df = df.loc[df['stretch'] <= _95th_percentile]
    return revise_df

def get_gp_n_time(root, consumers):
    res = process_chunks_log(root, consumers)

    for key in res:
        print ("----------------")
        print (res[key])
        print ("----------------")
        # exit()
    # print(ws_data)
    # wos_data = process_chunks_log(root, consumers)


def main():
    ws_dir = sys.argv[1]
    wos_dir = sys.argv[2]
    
    # exit()
    # _c = int([*ws_dir.split("/")[-2]][2])
    _c = 5
    consumers = ["sta{}".format(x+1) for x in range(1,_c+1)]
    root = ws_dir.split("ws")[0]
    get_gp_n_time(root, consumers)
    # revise_df = get_latency(ws_dir, wos_dir, consumers)
    # revise_df.to_csv("check00", encoding='utf-8')
main()
