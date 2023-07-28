import os
import tensorflow.keras as keras
from tensorflow.keras.layers import Dense  #fully connected dens layer


'''
The actor is represented by the action layer, which is defined in the __init__() method as a 
fully connected dense layer with n_actions nodes and no activation function. The action layer 
outputs a probability distribution over the possible actions given the current state. During 
training, the actor learns to maximize the expected reward by adjusting these action probabilities 
based on the observed rewards and states.

The critic is represented by the v layer, which is also defined in the __init__() method as a 
fully connected dense layer with a single output node and no activation function. The 'v' layer outputs 
the estimated value of the current state. The value function is used to guide the actor by providing an 
estimate of how good a particular state is in terms of expected future rewards. During training, the critic 
learns to better estimate the value function by minimizing the temporal difference error (refer to agent.py).

In the 'call()' method, the forward pass through the network is defined. The state input is first passed 
through the fully_connected_1 and fully_connected_2 layers, which apply ReLU activation functions to the outputs. 
The resulting value is then passed through the v layer to obtain the estimated value of the state, and through 
the action layer to obtain the action probabilities. The estimated value and action probabilities are then returned 
as outputs of the call() method.

'''
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
    
    self.fully_connected_1 = Dense(self.fc1_dims, activation='relu')
    self.fully_connected_2 = Dense(self.fc2_dims, activation='relu')
    
    self.value_function = Dense(1, activation=None)                  # value function, single value with no activation
    self.action_probabilities = Dense(n_actions, activation=None)     # policy pi with no activation

  def call(self, state):
    # forward pass
    value = self.fully_connected_1(state)
    value = self.fully_connected_2(value)

    value_function = self.value_function(value)
    action_probs = self.action_probabilities(value)
    return value_function, action_probs