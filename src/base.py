#!/home/mini-ndn/miniconda/envs/tf/bin/python python
import os
import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from sys import argv, exit
import io

# import gym
import numpy as np
from agent import Agent
# from utils import plot_learning_curve
# from gym import wrappers
import environment as e
from environment import _Embedding, MulticastEnvironment
import tensorflow as tf

fifo_suppression_value = 'fifo_suppression_value' #we don't care if this file is chaged because we write to this file
fifo_object_details = 'fifo_object_details'
num = 0

class FileModifiedHandler(FileSystemEventHandler):
    def __init__(self, path, file_name, callback):
        self.file_name = file_name
        self.callback = callback

        # set observer to watch for changes in the directory
        self.observer = Observer()
        self.observer.schedule(self, path, recursive=False)
        self.observer.start()
        self.observer.join()

    def on_modified(self, event): 
        # only act on the change that we're looking for
        if not event.is_directory and event.src_path.endswith(self.file_name):
            self.observer.stop() # stop watching
            self.callback() # call callback

def fifo_write(action):
    print ("Sending suppression time to nfd")
    if action < 0: 
        action = 0
    else:
        action = int(action*100)
    data = "|"+str(action)+"|"
    file2= os.path.join(".", fifo_suppression_value)
    try:
        os.mkfifo(file2)
    except FileExistsError:
        pass
    with open(file2, 'wb') as f:
        f.write(data.encode())
        f.close()

class Memory:
  def __init__(self):
    self.states = []
    self.actions = []
    self.rewards = []
    
  def store(self, state, action, reward):
    self.states.append(state)
    self.actions.append(action)
    self.rewards.append(reward)
    
  def clear(self):
    self.states = []
    self.actions = []
    self.rewards = []

class MulticastSuppression:
    def __init__(self, fifo_object_details, fifo_suppression_value, alpha=1e-5, n_actions=2, default_timer=10):
        self.evn = MulticastEnvironment('multi-cast')
        self.objects_A = e.generate_prefixes('/uofm/',  'abcdefgh')
        self.objects_B = e.generate_prefixes('/mit/',  'ijklmnox')
        self.objects_B = e.generate_prefixes('/ucla/',  'pqrstuvw')
        self.doc_dict = e.doc_dict
        self.agent = Agent(alpha, n_actions)
        self.embed = _Embedding(len(self.doc_dict.values()))
        self.prev_timer = default_timer #this time is in ms
        self.fifo_suppression_value = fifo_suppression_value
        self.fifo_object_details = fifo_object_details
        try:
            os.mkfifo(self.fifo_suppression_value)
            os.mkfifo(self.fifo_object_details)
        except FileExistsError:
            pass
    
    def embedding_handler(self, object_name, duplicates, prev_timer):
        # duplicates, delay_timer
        observation = list(self.doc_dict.values())[0]
        print (observation)
        observation = self.embed.get_embedding(observation)
        observation = tf.math.reduce_mean(observation, axis=0)
        observation = tf.concat((observation, [duplicates, prev_timer]), axis=0)
        return observation

if __name__ == "__main__":

    mcast_sup = MulticastSuppression(fifo_object_details, fifo_suppression_value)
    memory = Memory() #store each step in the memory
    done = False
    duplicates = 0
    object_name = None
    while (1):
        try:
            with open ("fifo_object_details", 'rb') as f:
                content = f.read().decode('utf-8', 'ignore')
                content = content.split("|")
                object_name = content[1]
                duplicates = int(content[2])
                print ("object name: ", object_name, "duplicates: ", duplicates)
        except Exception as e:
            print (e)
        observation = mcast_sup.embedding_handler(object_name, duplicates, mcast_sup.prev_timer)
        action = mcast_sup.agent.choose_action(observation)
        fifo_write(action)
