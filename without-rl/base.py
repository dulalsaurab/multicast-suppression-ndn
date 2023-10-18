
import os
import time
from sys import argv, exit
import string
import json
import errno

import numpy as np

import tensorflow as tf
from pathlib import Path as p


fifo_suppression_value = 'fifo_suppression_value' #we don't care if this file is chaged because we write to this file
fifo_object_details = 'fifo_object_details'
num = 0




# INCOMPLETE
if __name__ == "__main__":
  # mcast_sup = MulticastSuppression(fifo_object_details, fifo_suppression_value)
  print ("The reinforcement is starting........")
  print("*********", p.cwd(), "*************") # for debugging
  

  try:
    os.mkfifo(fifo_object_details, mode=0o777)
  except OSError as oe:
    if oe.errno != errno.EEXIST:
      print("File does not exist!!!")
      raise

  counter = 0 # this is only for testing, modify accordingly
  while True:
    try:
      read_pipe = os.open(fifo_object_details, os.O_RDONLY)
      bytes = os.read(read_pipe, 1024)
      if len(bytes) == 0:
        print("length of byte is zero")
        break
      states_string = bytes.decode()
      states_dict = json.loads(states_string)
      
      states_values = [str(value) for value in states_dict.values()]
      
      result = '/'.join(states_values)
      print("Fifo content : ", result)

      # # Pad the tensor
      #padded_tensor = new_embeddings

      # Now, padded_tensor will have a shape of (1, 120)

      action = float(counter)
      print("Final action  = ", action)
      os.close(read_pipe)

      write_pipe = os.open(fifo_object_details, os.O_WRONLY)
    
      response = "{}".format(action)
      os.write(write_pipe, response.encode())
      os.close(write_pipe)


      counter = counter + 1
      print("Counter in RL: ", counter)



    except Exception as e:
      print ("exception: ", e)
  