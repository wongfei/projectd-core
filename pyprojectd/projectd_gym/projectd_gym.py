import gym as gym
from gym import spaces as gym_spaces
from gym.utils import seeding as gym_seeding

import numpy as np
from typing import Optional

from projectd_env import ProjectDEnv

class ProjectDEnvGym(gym.Env):

    def __init__(self):
        self.seed()
        self.impl = ProjectDEnv()
        
        obs_low, obs_high = self.impl._get_obs_space()
        a_low, a_high = self.impl._get_action_space()
        
        self.observation_space = gym_spaces.Box(low=obs_low, high=obs_high, dtype=np.float32)
        self.action_space = gym_spaces.Box(low=a_low, high=a_high, dtype=np.float32)
        
        self.impl.init_sim()

    def close(self):
        self.impl.close()

    def step(self, action):
        state, reward, terminate, truncate, info = self.impl.step(action)
        return state, reward, (terminate or truncate), {}

    def reset(self):
        state = self.impl.reset()
        return state

    def seed(self, seed=None):
        self.np_random, seed = gym_seeding.np_random(seed)
        return [seed]

    def render(self, mode):
        pass
