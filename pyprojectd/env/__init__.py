from env.projectd_env import ProjectDEnv
from gymnasium.envs.registration import register

register(
    id="ProjectD-v0",
    entry_point="env.projectd_env:ProjectDEnv",
    max_episode_steps=40000,
)
