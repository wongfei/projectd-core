from gymnasium.envs.registration import register as register
from projectd_gymnasium.projectd_gymnasium import ProjectDEnvGymnasium

register(
    id="ProjectD-v0",
    entry_point="projectd_gymnasium.projectd_gymnasium:ProjectDEnvGymnasium",
    max_episode_steps=80000,
)
