
import os
import time
from sys import argv, exit
import string
import json

import numpy as np
from agentTest import Agent
import environment as e
from environment import _Embedding, MulticastEnvironment
import tensorflow as tf
from pathlib import Path as p


fifo_suppression_value = 'fifo_suppression_value' #we don't care if this file is chaged because we write to this file
fifo_object_details = 'fifo_object_details'
num = 0

# def fifo_write(action):
#   print ("Sending suppression time to nfd")
#   if action < 0: 
#     action = 0
#   else:
#     action = int(action*100)
#   data = "|"+str(action)+"|"
#   file2= os.path.join(".", fifo_suppression_value)
#   try:
#     os.mkfifo(file2)
#   except FileExistsError:
#     pass
#   with open(file2, 'wb') as f:
#     f.write(data.encode())
#     f.close()

'''
The Memory class is used to store the observations, actions, and rewards of the agent at each step during training. 
It has a store() method which takes as input the current state, the action taken by the agent, and the reward obtained, 
and stores these values in separate lists self.states, self.actions, and self.rewards, respectively.

The clear() method is used to clear the Memory object, which is useful when we want to start a new episode of training
'''
# class Memory:
#   def __init__(self):

#     # prefix_dict = {<preifx_name>: [srtt, ewma]}
#     self.prefix_dict = {}
#     self.prev_observation = None
#     self.prev_suppression_timer = 1

#     self.states = []
#     self.actions = []
#     self.rewards = []
    
#   def store(self, state, action, reward):
#     self.states.append(state)
#     self.actions.append(action)
#     self.rewards.append(reward)
    
#   def clear(self):
#     self.states = []
#     self.actions = []
#     self.rewards = []


# class MulticastSuppression:
#   def __init__(self, fifo_object_details, fifo_suppression_value, alpha=1e-5, n_actions=4, default_timer=10):
#     self.env = MulticastEnvironment('multi-cast')
#     self.agent = Agent(n_actions=n_actions)
#     self.embed = _Embedding(number_of_document=200)
#     self.prev_suppression_timer = default_timer #this time is in ms
#     self.fifo_suppression_value = fifo_suppression_value
#     self.fifo_object_details = fifo_object_details
#     try:
#         os.mkfifo(self.fifo_suppression_value)
#         os.mkfifo(self.fifo_object_details)
#     except FileExistsError:
#         pass


def generate_positional_one_hot_vector(text):
  alphabet = string.ascii_lowercase  # Get all lowercase alphabets
  digits = string.digits  # Get all digits
  symbols = "!@#$%^&*()_-+={}[]|\:;'<>?,./\""
  vocab = alphabet + digits + symbols
    
  # Create a dictionary to map characters in the vocabulary to unique indices
  char_to_index = {char: idx for idx, char in enumerate(vocab)}
    
  # Create the one-hot vector with positional encoding
  one_hot_vector = []
  for idx, char in enumerate(text.lower()):
    if char in char_to_index:
      # Get the index of the character in the vocabulary
      char_idx = char_to_index[char]
            
    # Append the character index and the position index to the one-hot vector
      one_hot_vector.append(char_idx)
      one_hot_vector.append(idx)
    
  return one_hot_vector
    

# INCOMPLETE
if __name__ == "__main__":
  # mcast_sup = MulticastSuppression(fifo_object_details, fifo_suppression_value)
  print ("The reinforcement is starting........")
  print("*********", p.cwd(), "*************") # for debugging

  try:
    os.mkfifo(fifo_object_details, mode=0o777)
  except OSError as oe:
    if oe.errno != errno.EEXIST:
      print("some shit happened")
      raise

  counter = 0 # this is only for testing, modify accordingly
  while counter < 50:
    try:
      read_pipe = os.open(fifo_object_details, os.O_RDONLY)
      bytes = os.read(read_pipe, 1024)
      if len(bytes) == 0:
        print("lenght of byte is zero")
        break
      states_string = bytes.decode()
      states_dict = json.loads(states_string)
      states_values = [str(value) for value in states_dict.values()]
      result = '/'.join(states_values)
      embeddings = generate_positional_one_hot_vector(result)
      print("Received from NDN/C++", embeddings)
      agent = Agent()
      print("Choosing Action !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
      print(embeddings)
      action = agent.choose_action(embeddings)
      
      os.close(read_pipe)
      
      write_pipe = os.open(fifo_object_details, os.O_WRONLY)
    
      response = "{}".format(counter)
      counter = counter + 1
      os.write(write_pipe, response.encode())

      os.close(write_pipe)

    except Exception as e:
      print ("exception: ", e)
  # create fifo for communication. Make sure that RL module is started before C++
  # C++ will write to an open fifo 
  # try:
  #   os.mkfifo(fifo_object_details, mode=0o777)
  #   os.mkfifo(fifo_suppression_value, mode=0o777)

  # except OSError as oe:
  #   if oe.errno != errno.EEXIST:
  #     print("some shit happened")
  #     raise

  # counter = 0 # this is only for testing, modify accordingly
  # while True:
  #   try:
  #     read_pipe = os.open(fifo_object_details, os.O_RDONLY)
  #     bytes = os.read(read_pipe, 1024)
      
  #     if len(bytes) == 0:
  #       break
  #     states_string = bytes.decode()
  #     states_dict = json.loads(states_string)
  #     states_values = [str(value) for value in states_dict.values()]
  #     result = '/'.join(states_values)
  #     embeddings = generate_positional_one_hot_vector(result)
  #     print(embeddings)

  #     agent = Agent() 

  #     action = agent.choose_action(embeddings)
      
  #     os.close(read_pipe)
      
  #     print("Action", action)

  #     write_pipe = os.open(fifo_suppression_value, os.O_WRONLY)
    
  #     response = "{}".format(action)
  #     os.write(write_pipe, response.encode())

  #     os.close(write_pipe)
  #     # read_pipe_second = os.open(fifo_suppression_value, os.O_RDONLY)
  #     # bytes_second = os.read(read_pipe_second, 1024)
  #     # states_string_second = bytes_second.decode()
  #     # states_dict_second = json.loads(states_string_second)
  #     # states_values_second = [str(value) for value in states_dict_second.values()]
  #     # result_second = '/'.join(states_values_second)
  #     # embeddings_second = generate_positional_one_hot_vector(result_second)

  #   except Exception as e:
  #     print ("exception: ", e)