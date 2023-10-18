import gym

from gym import spaces
import numpy as np
from gym.spaces import Box, Discrete




class NDNEnv(gym.Env):
    def __init__(self):
        self.action_space = Box(low=np.array([0]), high=np.array([100]))
        # self.obsrevation_space = spaces.Box


    def step(self, action):
        # Execute one time step wihtin the environment
        pass

    def reset(self):
        # Reset the state of the environment to an initial state
        pass

    def _next_observation(self):
        pass

    def choose_action(self, action):
        pass


    