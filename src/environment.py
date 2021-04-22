
from gym import Env
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
  action_dim = 3
  state_dim = 14
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


# need to compute the reward for an interval
  def _get_reward(dup_count, prev_act_timestamp):
    time_in_sec = abs (int ((time.time() - prev_act_timestamp) * 1000))
    reward = - dup_count/(time_in_sec*time_in_sec + 1)
    return reward

  def sample_action(self):
    return np.random.uniform(*self.action_bound, size = self.action_dim)

def generate_prefixes(prefix, alphabets):
    name_prefixes = [prefix+''.join(random.choices(alphabets, k=5)) for i in range(0,20)]
    for name in name_prefixes:
        pad_emb = [0]*MAX_LENGTH
        for element in name:
          try:
            map_val = word_to_int_map[name]
          except:
            word_to_int_map[name] = map_counter+1
        pad_emb = [0]*MAX_LENGTH
        emb = [ord(i) for i in list(name)]
        pad_emb[:len(emb)] = emb 
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

objects_A = generate_prefixes('/uofm/',  'abcdefgh')
objects_B = generate_prefixes('/mit/',  'ijklmno')
objects_B = generate_prefixes('/ucla/',  'pqrstuvw')

class _Embedding():
    def __init__(self, vocab_size=300, emd_dim=50):
        self.vocab_size = vocab_size
        self.emd = emd_dim
        self.input = Input(shape=(len(doc_dict.values()), MAX_LENGTH), dtype='float64') #len(doc_dict.values()) = number of document
        self.word_input = Input(shape=(MAX_LENGTH,), dtype='float64') 
        self.word_embedding = Embedding(input_dim=vocab_size, output_dim=2, input_length=MAX_LENGTH)(self.word_input)
        self.word_vec=Flatten()(self.word_embedding)
        self.embed_model = Model([self.word_input],self.word_vec)
        self.embed_model.compile(optimizer=tf.keras.optimizers.Adam(lr=1e-3), loss='binary_crossentropy', metrics=['acc']) 

    def get_embedding(self, words_id):
        return self.embed_model.predict(words_id) 

# process the above names
if __name__ == "__main__":
    # env = MulticastEnvironment()
    # print(env.sample_action())
    pass



    '''
    comments: we can decrease the input size, 12 seem very big, for obj -- make 3, and delay 1 --- and dub -- 1  in total 5
    '''