
import os
import time
from sys import argv, exit
import string
import json

import numpy as np
from agent import Agent
import environment as e
from environment import _Embedding, MulticastEnvironment
import tensorflow as tf
from pathlib import Path as p


fifo_suppression_value = 'fifo_suppression_value' #we don't care if this file is chaged because we write to this file
fifo_object_details = 'fifo_object_details'
num = 0

'''
The Memory class is used to store the observations, actions, and rewards of the agent at each step during training. 
It has a store() method which takes as input the current state, the action taken by the agent, and the reward obtained, 
and stores these values in separate lists self.states, self.actions, and self.rewards, respectively.

The clear() method is used to clear the Memory object, which is useful when we want to start a new episode of training
'''


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
      agent = Agent()
      action = agent.choose_action(embeddings)
      print("Final action  = ", action)
      os.close(read_pipe)
      
      write_pipe = os.open(fifo_object_details, os.O_WRONLY)
    
      response = "{}".format(action)
      counter = counter + 1
      os.write(write_pipe, response.encode())

      os.close(write_pipe)

    except Exception as e:
      print ("exception: ", e)
  