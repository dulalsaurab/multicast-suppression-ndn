import re
import string
import itertools
import copy 
import random
import numpy as np
import time


import tensorflow as tf
import tensorflow.keras
from tensorflow.keras.preprocessing.text import one_hot,Tokenizer
from tensorflow.keras.preprocessing.sequence import pad_sequences
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense , Flatten ,Embedding,Input
from tensorflow.keras.models import Model

root_vec = [chr(x) for x in range(47, 57)]+[chr(y) for y in range (97,123)]
root_vec_dict = dict((keys, 0) for keys in root_vec)

# This dictionary contains all the possible name prefixes we will have
#  and is constructed by generate_prefixes. 
doc_dict = {}
word_to_int_map = {}
map_counter = -1
# maximum length of each string, i.e. object (12) + dup_counter (1)+ delay_timer (1) 
MAX_LENGTH = 4


class MulticastEnvironment():
  action_bound = [0, 10]
  action_dim = 2
  state_dim = 4
  reward_range = [-100, 100]

  def __init__(self, name, obj_dict = {}, curr_state = None):
    self.name = name
    self.obj_dict = obj_dict 
    self.curr_state = curr_state

  def _step(self, action):
    self._take_action(action)
    reward = self._get_reward()
    episode_over = False
    return self.curr_state, reward, episode_over, {}

  def _set_state(self, state):
    self.state = state

  def get_reward(ewma_duplicate_count, rtt, srtt, alpha=0.5, beta=0.4, gamma=0.3):
    if rtt is not None:
        reward = -(alpha * ewma_duplicate_count + beta * rtt + gamma * (rtt - srtt))
    else:
        reward = -(alpha * ewma_duplicate_count + gamma * srtt)
    return reward

  def get_state():
    return self.state


'''
 * @brief Class for generating word embeddings based on name prefixes, EWMA of duplicate count, and SRTT.
 * This class uses pre-trained embedding models from TensorFlow to generate embeddings based on the input data.
'''
class _Embedding():
  def __init__(self, number_of_document, prefix_dim=128, ewma_dim=1, srtt_dim=1, rtt_dim=1):
      self.prefix_dim = prefix_dim
      self.ewma_dim = ewma_dim
      self.srtt_dim = srtt_dim
      self.rtt_dim = rtt_dim
      self.input = Input(shape=(number_of_document, prefix_dim+ewma_dim+srtt_dim+rtt_dim), dtype='float32')
      
      
      # Load a pre-trained embedding model from TensorFlow Hub
      embedding_module = "https://tfhub.dev/google/tf2-preview/nnlm-en-dim128/1"
      self.embedding_layer = hub.KerasLayer(embedding_module, input_shape=[], dtype=tf.string, trainable=True)

      # if we want to use pre-train from local, Load a pre-trained embedding model from TensorFlow
      # self.embedding_model = tf.keras.Sequential([
      #     tf.keras.layers.Embedding(input_dim=10000, output_dim=128),
      #     tf.keras.layers.GlobalAveragePooling1D()
      # ])
      # self.embedding_model.load_weights("path/to/embedding/model")
        
  def get_embedding(self, prefix, ewma, srtt, rtt):
    # Concatenate the input features into a single input array
    input_data = np.concatenate([prefix, ewma, srtt, rtt], axis=-1)
    
    # Convert to string
    input_strings = []
    for i in range(input_data.shape[0]):
        input_strings.append(" ".join([str(x) for x in input_data[i]]))
    
    '''
      Note embedding_vectors = self.embedding_layer(input_strings) performs the same task 
      as embedding_vectors = self.embedding_model.predict(input_strings). 
      It applies the pre-trained embedding to the input tensor and returns the resulting embedding vectors.
      Note that the input_strings can have different size mostly based on the preifx size, but pre-trained 
      model can handle this. Additionally, the output vector size will be 128-dimensional vector
    '''
    embedding_vectors = self.embedding_layer(input_strings)
    
    return embedding_vectors




