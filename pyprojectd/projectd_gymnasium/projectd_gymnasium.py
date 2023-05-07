import gymnasium as gym
from gymnasium import spaces as gym_spaces

import numpy as np
from typing import Optional

from projectd_env import ProjectDEnv

class ProjectDEnvGymnasium(gym.Env):

    def __init__(self):
        super().__init__()
        
        self.impl = ProjectDEnv()
        
        obs_low, obs_high = self.impl._get_obs_space()
        a_low, a_high = self.impl._get_action_space()
        
        self.observation_space = gym_spaces.Box(low=obs_low, high=obs_high, dtype=np.float32)
        self.action_space = gym_spaces.Box(low=a_low, high=a_high, dtype=np.float32)
        
        self.impl.init_sim()

    def close(self):
        self.impl.close()

    def step(self, action):
        return self.impl.step(action)

    def reset(self, *, seed: Optional[int] = None, options: Optional[dict] = None):
        super().reset(seed=seed)
        state = self.impl.reset()
        return state, {}

    def render(self):
        pass

    # compat with legacy gym
    def seed(self, seed=None):
        pass
        