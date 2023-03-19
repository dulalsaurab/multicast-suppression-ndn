
import os
import time
from sys import argv, exit
import io

import numpy as np
from agent import Agent
import environment as e
from environment import _Embedding, MulticastEnvironment
import tensorflow as tf

fifo_suppression_value = 'fifo_suppression_value' #we don't care if this file is chaged because we write to this file
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

'''
The Memory class is used to store the observations, actions, and rewards of the agent at each step during training. 
It has a store() method which takes as input the current state, the action taken by the agent, and the reward obtained, 
and stores these values in separate lists self.states, self.actions, and self.rewards, respectively.

The clear() method is used to clear the Memory object, which is useful when we want to start a new episode of training
'''
class Memory:
  def __init__(self):

  	# prefix_dict = {<preifx_name>: [srtt, ewma]}
  	self.prefix_dict = {}
    self.prev_observation = None
    self.prev_suppression_timer = 1

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
        self.agent = Agent(alpha, n_actions)
        self.embed = _Embedding()
        self.prev_suppression_timer = default_timer #this time is in ms
        self.fifo_suppression_value = fifo_suppression_value
        self.fifo_object_details = fifo_object_details
        try:
            os.mkfifo(self.fifo_suppression_value)
            os.mkfifo(self.fifo_object_details)
        except FileExistsError:
            pass
    

# INCOMPLETE
if __name__ == "__main__":

    mcast_sup = MulticastSuppression(fifo_object_details, fifo_suppression_value)
    memory = Memory()
    
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
                # get name_prefix, srtt, rtt (if available), ewma

                   
        except Exception as e:
            print (e)

        new_observation = mcast_sup.embed.get_embedding(name_prefix, ewma, srtt, rtt)

        reward = mcast_sup.env._get_reward(name_prefix, ewma, srtt, rtt)
        action = mcast_sup.agent.choose_action(new_observation)
        print ("Suppression time value from the action: ", action)
        score += reward

        if (counter > 0):
             # if not load_checkpoint:
            mcast_sup.agent.learn(memory.prev_observation, reward, new_observation, done)

        counter = counter+1

        # update memory
 		memory.store(new_observation, action, reward)
        memory.prefix_dict[name_prefix] = [srtt, ewma]
        memory.prev_observation = new_observation
        memory.prev_suppression_time = action

        print ("Counter:", counter, "Score: ", score,  "Reward: ", reward)
        fifo_write(action)

