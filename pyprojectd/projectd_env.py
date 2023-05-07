import os
import sys
import site
import time
import math
import random
import numpy as np

base_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..')
bin_dir = os.path.join(base_dir, 'bin')
site.addsitedir(bin_dir)
os.add_dll_directory(bin_dir)

import PyProjectD as pd
import utils_d as u

class ProjectDEnv():

    sim_dt = 1.0 / 333.0
    render_hz = 60
    viewer_enabled = True
    smooth_controls = True
    
    terminate_reward = -1000.0
    terminate_on_hit = True
    terminate_off_track = True
    
    teleport_mode = 0 # 0:Start, 1:Nearest, 2:Random
    teleport_on_reset = True
    teleport_on_hit = False
    teleport_off_track = False
    
    track_name = 'driftplayground'
    car_model = 'gravygarage_street_ae86_readie'

    #==============================================================================================

    def __init__(self):

        global base_dir
        self.base_dir = base_dir
        self.sim_initialized = False
        self.viewer_initialized = False
        self.step_id = 0

        self.init_sim()

    def close(self):
        self.del_viewer()
        self.del_sim()

    #==============================================================================================
    
    def init_sim(self):
        self.sim = pd.createSimulator(self.base_dir)
        pd.loadTrack(self.sim, self.track_name)
        self.car = pd.addCar(self.sim, self.car_model)
        
        pd.teleportCarByMode(self.sim, self.car, self.teleport_mode)
        pd.setCarAutoTeleport(self.sim, self.car, self.teleport_on_hit, self.teleport_off_track, self.teleport_mode)
        
        self.dstate = pd.CarState()
        self.dcontrols = pd.CarControls()
        self.sim_initialized = True

    def del_sim(self):
        if self.sim_initialized:
            pd.destroySimulator(self.sim)
            self.sim_initialized = False

    def init_viewer(self):
        pd.initPlayground(self.base_dir)
        pd.setRenderHz(int(self.render_hz), False)
        pd.setActiveSimulator(self.sim, False)
        pd.tickPlayground()
        self.viewer_initialized = True
        
    def del_viewer(self):
        if self.viewer_initialized:
            pd.shutPlayground()
            self.viewer_initialized = False

    #==============================================================================================

    def step(self, action):
        
        self.dcontrols.steer = action[0] # steer [-1, 1]
        
        self.dcontrols.gas = u.linscalef(action[1], -1.0, 1.0, 0.0, 1.0) # gas [0, 1]
        #self.dcontrols.brake = u.linscalef(action[2], -1.0, 1.0, 0.0, 1.0) # brake [0, 1]
        
        #throtl = action[1]
        #if throtl >= 0.0:
            #self.dcontrols.gas = throtl
        #else:
            #self.dcontrols.brake = -throtl
        
        self.dcontrols.brake = 0.0
        self.dcontrols.handBrake = 0.0
        self.dcontrols.clutch = 1.0
        self.dcontrols.requestedGearIndex = 2 # 0=R, 1=N, 2=G1, 3=G2, 4=G3, 5=G4, 6=G5, 7=G6
        
        pd.setCarControls(self.sim, self.car, self.smooth_controls, self.dcontrols)
        pd.stepSimulator(self.sim, self.sim_dt)
        pd.getCarState(self.sim, self.car, self.dstate)
        self.step_id += 1

        if self.viewer_enabled:
            if not self.viewer_initialized and self.step_id > 10:
                self.init_viewer()
            if self.viewer_initialized:
                pd.tickPlayground()

        state = self._get_obs_state()
        reward = self.dstate.agentDriftReward
        terminate = ((self.terminate_on_hit and self.dstate.collisionFlag != 0) or (self.terminate_off_track and self.dstate.outOfTrackFlag != 0))
        truncate = False

        if terminate:
            if self.dstate.collisionFlag:
                pd.writeLog('[ENV] collision')
            if self.dstate.outOfTrackFlag:
                pd.writeLog('[ENV] out of track')
            reward += self.terminate_reward

        return state, reward, terminate, truncate, {}

    #==============================================================================================

    def reset(self):
        pd.writeLog('[ENV] reset')

        self.step_id = 0

        if self.teleport_on_reset:
            pd.teleportCarByMode(self.sim, self.car, self.teleport_mode)
            
        state, reward, terminate, truncate, info = self.step(np.array([0, 0, 0]))
        
        return state

    #==============================================================================================
    
    def _get_obs_state(self):
        
        state = np.array([

            u.linscalef(self.dstate.engineRPM, 0.0, 20000.0, 0.0, 1.0),
            
            self.dstate.bodyVsTrack, # body_direction dot track_direction
            self.dstate.velocityVsTrack, # velocity dot track_direction

            self.dstate.velocity.x, # m/s
            self.dstate.velocity.y,
            self.dstate.velocity.z,
            
            self.dstate.angularVelocity.x, # rad/s
            self.dstate.angularVelocity.y,
            self.dstate.angularVelocity.z,
            
            self.dstate.tyreNdSlip[0],
            self.dstate.tyreNdSlip[1],
            self.dstate.tyreNdSlip[2],
            self.dstate.tyreNdSlip[3],
            
            self.dstate.probes[0], # distance to obstacle, m
            self.dstate.probes[1],
            self.dstate.probes[2],
            self.dstate.probes[3],
            self.dstate.probes[4],
            self.dstate.probes[5],
            self.dstate.probes[6],
            
            self.dstate.lookAhead[0], # track direction change [-PI, PI] normalized to [-1, 1]
            self.dstate.lookAhead[1],
            self.dstate.lookAhead[2],
            self.dstate.lookAhead[3],
            self.dstate.lookAhead[4],
            
        ], dtype=np.float32)
        
        return state

    #==============================================================================================

    def _get_obs_space(self):
    
        range_velocity = 100
        range_angularVelocity = 100
        range_tyreNdSlip = 10
        range_probe = 50
        
        obs_low = np.array([

            0.0, # engineRpmNorm
            
            -1.0, # bodyVsTrack
            -1.0, # velocityVsTrack

            -range_velocity, # velocity.x
            -range_velocity, # velocity.y
            -range_velocity, # velocity.z
            
            -range_angularVelocity, # angularVelocity.x
            -range_angularVelocity, # angularVelocity.y
            -range_angularVelocity, # angularVelocity.z
            
            0.0, # tyreNdSlip0
            0.0, # tyreNdSlip1
            0.0, # tyreNdSlip2
            0.0, # tyreNdSlip3
            
            0.0, # probe0
            0.0, # probe1
            0.0, # probe2
            0.0, # probe3
            0.0, # probe4
            0.0, # probe5
            0.0, # probe6
            
            -1.0, # lookAhead0
            -1.0, # lookAhead1
            -1.0, # lookAhead2
            -1.0, # lookAhead3
            -1.0, # lookAhead4
            
        ], dtype=np.float32)
        
        obs_high = np.array([

            1.0, # engineRpmNorm
            
            1.0, # bodyVsTrack
            1.0, # velocityVsTrack

            range_velocity, # velocity.x
            range_velocity, # velocity.y
            range_velocity, # velocity.z
            
            range_angularVelocity, # angularVelocity.x
            range_angularVelocity, # angularVelocity.y
            range_angularVelocity, # angularVelocity.z
            
            range_tyreNdSlip, # tyreNdSlip0
            range_tyreNdSlip, # tyreNdSlip1
            range_tyreNdSlip, # tyreNdSlip2
            range_tyreNdSlip, # tyreNdSlip3
            
            range_probe, # probe0
            range_probe, # probe1
            range_probe, # probe2
            range_probe, # probe3
            range_probe, # probe4
            range_probe, # probe5
            range_probe, # probe6
            
            1.0, # lookAhead0
            1.0, # lookAhead1
            1.0, # lookAhead2
            1.0, # lookAhead3
            1.0, # lookAhead4
            
        ], dtype=np.float32)
        
        return obs_low, obs_high

    #==============================================================================================

    def _get_action_space(self):

        # steer, gas
        a_low  = np.array([-1, -1], dtype=np.float32)
        a_high = np.array([+1, +1], dtype=np.float32)

        return a_low, a_high
