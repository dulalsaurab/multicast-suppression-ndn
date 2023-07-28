# from gym import Env
# from gym.spaces import Descrete, Box
import re
import string
import itertools
import copy 
import random
import numpy as np
import time

#keras
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
    self._take_action(action) #once this function is called, the interest or data will be sent out
    # self.status = self.env.step()
    reward = self._get_reward()
    # ob = self.env.getState()
    episode_over = self.status != hfo_py.IN_GAME
    return ob, reward, episode_over, {}

  def _set_state(self, state):
    self.state = state

  def get_state():
    return self.state

  def _render(self):
    pass

  def _reset(self):
    pass

  def _get_reward(self, dup_count, prev_act_timestamp):
    # time_in_sec = abs (int ((time.time() - prev_act_timestamp) * 1000))
    # reward = - dup_count/(time_in_sec*time_in_sec + 1)
    if dup_count <= 0: 
      reward = 5; 
    else:
      reward = -dup_count
    # reward = - (1/dup_count+2)*10
    return reward

  def sample_action(self):
    return np.random.uniform(0, 1) #return some number between 0 and 1
    # return np.random.uniform(*self.action_bound, size = self.action_dim)

def generate_prefixes(prefix, alphabets, number_of = 20):
    global map_counter
    name_prefixes = None
    if prefix == "/uofm/":
        p = ['ahdhg', 'afhag', 'babgh', 'adgfe', 'bhada', 'deceh', 'gcbhb', 'fbebh', 'hdbgh', 'gcddf', 'ehhfh', 'cffff', 'eegec', 'baafd', 'hfecf', 'hhbdf', 'hfgaa', 'hgcha', 'aehac', 'acgce']
        name_prefixes = ["/uofm/"+i for i in p]
    if prefix == "/mit/":
        p = ['nijmm', 'jmknk', 'niiij', 'xnkoi', 'ljloj', 'jokim', 'konji', 'iojom', 'oxloi', 'xjjkl', 'jnmij', 'xkimm', 'njoix', 'lxikk', 'mojim', 'ixilj', 'xikjm', 'xmnnj', 'mnxkj', 'ikomn']
        name_prefixes =["/mit/"+i for i in p] 
    if prefix == "/ucla/":
        p = ['wrqqt', 'vuutq', 'rqusw', 'tsrqr', 'qqspq', 'pqvrq', 'vwwqt', 'prppr', 'pppup', 'vqsuw', 'rrrur', 'vwqss', 'quvqp', 'pswrr', 'rqvvp', 'wruuv', 'ruuqu', 'twuvv', 'qpvur', 'vqtrv']
        name_prefixes =["/mit/"+i for i in p]  
    
    for name in name_prefixes:
        pad_emb = list()
        for element in name.split('/'):
          if element == "":
            continue
          if element in word_to_int_map:
            pad_emb.append(word_to_int_map[element])
          else:
            map_counter = map_counter + 1 
            word_to_int_map[element] = map_counter
            pad_emb.append(map_counter)
        doc_dict[name] = pad_emb
    return name_prefixes

def object_embedding_char(objects):
    embedding_dict = {}
    for obj in objects:
        vec_dict = copy.copy(root_vec_dict)
        try:
            for _char in list(obj):
                vec_dict[_char] = vec_dict[_char] + 1  
        except Exception as e:
            print(e)
        embedding_dict[obj] = list(vec_dict.values())
    return embedding_dict

class _Embedding():
    def __init__(self, number_of_document, vocab_size=63, emd_dim=40):
        self.vocab_size = vocab_size
        self.emd = emd_dim
        self.input = Input(shape=(number_of_document, MAX_LENGTH-1), dtype='float32') #len(doc_dict.values()) = number of document
        self.word_input = Input(shape=(MAX_LENGTH-2), dtype='float32') 
        self.word_embedding = Embedding(input_dim=vocab_size, output_dim=12,  input_length=MAX_LENGTH-1)(self.word_input)
        self.word_vec=Flatten()(self.word_embedding)
        self.embed_model = Model([self.word_input], self.word_vec)
        self.embed_model.compile(optimizer=tf.keras.optimizers.Adam(lr=1e-3), loss='binary_crossentropy', metrics=['acc']) 

    def get_embedding(self, words_id):
        return self.embed_model.predict(words_id) 
