import os
import tensorflow.keras as keras
from tensorflow.keras.layers import Dense  #fully connected dens layer

class ActorCriticNetwork(keras.Model):
    def __init__(self, n_actions, fc1_dims=1024, fc2_dims=512,
            name='actor_critic', chkpt_dir='tmp/actor_critic'):
        super(ActorCriticNetwork, self).__init__()
        self.fc1_dims = fc1_dims
        self.fc2_dims = fc2_dims
        self.n_actions = n_actions
        self.model_name = name
        self.checkpoint_dir = chkpt_dir
        self.checkpoint_file = os.path.join(self.checkpoint_dir, name+'_ac')
        
        self.fc1 = Dense(self.fc1_dims, activation='relu')
        self.fc2 = Dense(self.fc2_dims, activation='relu')
        
        self.v = Dense(1, activation=None)                  #value function, single value with no activation
        self.action = Dense(n_actions, activation=None)         #policy pi with no activation

    def call(self, state):
        # forward pass
        value = self.fc1(state)
        value = self.fc2(value)

        value_function = self.v(value)
        action_probs = self.pi(value)
        return value_function, action_probs
