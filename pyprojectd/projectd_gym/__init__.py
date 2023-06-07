from gym.envs.registration import register as register
from projectd_gym.projectd_gym import ProjectDEnvGym

register(
    id="ProjectD-v0",
    entry_point="projectd_gym.projectd_gym:ProjectDEnvGym",
    max_episode_steps=80000,
)
