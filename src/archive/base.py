import os
import time
from sys import argv, exit
import io

import numpy as np
from agent import Agent
import environment as e
from environment import _Embedding, MulticastEnvironment
import tensorflow as tf

fifo_suppression_value = 'fifo_suppression_value'
fifo_object_details = 'fifo_object_details'
num = 0

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
    self.prev_observation = None
    self.prev_timestamp = time.time()
    self.prev_duplicate = 5 #just assigning some random number
    self.prev_suppression_time = None
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
        self.env = MulticastEnvironment('multi-cast')
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
        observation = self.doc_dict[object_name]
        observation = self.embed.get_embedding(observation)
        observation = tf.math.reduce_mean(observation, axis=0)
        observation = tf.concat((observation, [100+duplicates, 200+prev_timer]), axis=0)
        print ("Object name: ", object_name,  "Embedding: ", observation)
        return observation

if __name__ == "__main__":

    mcast_sup = MulticastSuppression(fifo_object_details, fifo_suppression_value)
    memory = Memory() #store each step in the memory
    done = False
    duplicates = 0
    object_name = None
    score = 0
    counter = 0
    memory.prev_suppression_time = int(mcast_sup.env.sample_action()*100)
    while(1):
        print ("The reinforcement is starting........")
        try:
            with open ("fifo_object_details", 'rb') as f:
                content = f.read().decode('utf-8', 'ignore')
                content = content.split("|")
                object_name_temp = content[1].split("/")
                object_name = "/{}/{}".format(object_name_temp[1], object_name_temp[2])
                duplicates = abs(int(content[2]))
                print ("object name: ", object_name, "duplicates: ", duplicates)
        except Exception as e:
            print (e)
        observation_ = mcast_sup.embedding_handler(object_name, duplicates, memory.prev_suppression_time)
        reward = mcast_sup.env._get_reward(duplicates, memory.prev_timestamp)
        action = mcast_sup.agent.choose_action(observation_)
        print ("Action value: ", action)
        score += reward
        if (counter > 0):
             # if not load_checkpoint:
            mcast_sup.agent.learn(memory.prev_observation, reward, observation_, done)
        counter = counter+1
        # memoryobservation = observation_ 
        memory.prev_timestamp = time.time()
        memory.prev_duplicate = duplicates
        memory.prev_observation = observation_
        memory.store(observation_, action, reward)
        memory.prev_suppression_time = int(action*100)
        print ("Counter:", counter, "Score: ", score,  "Reward: ", reward)
        fifo_write(action)
       