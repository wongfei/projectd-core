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

    track_name = 'driftplayground'
    car_model = 'ks_toyota_supra_mkiv_drift'

    smooth_controls = True
    auto_clutch = True
    auto_shift = True
    auto_blip = True

    range_velocity = 100
    range_angularVelocity = 100
    range_tyreNdSlip = 10
    range_lookAhead = math.pi
    range_probe = 50

    terminate_on_hit = True
    terminate_off_track = True
    terminate_when_stuck = True
    
    terminate_hit_penalty = 100.0
    terminate_off_track_penalty = 100.0
    terminate_stuck_penalty = 0.0

    terminate_low_reward = -10000.0
    stuck_timeout = 5.0

    teleport_mode = 2 # 0:Start, 1:Nearest, 2:Random
    teleport_on_reset = True
    teleport_on_hit = False
    teleport_off_track = False

    min_gas = 0.1
    max_gas = 1.0

    scoring_vars = {
        "SmoothSteerSpeed" : 10.0, # car.steer += (target - current) * SmoothSteerSpeed * dt
        "MinBonusSpeed" : 5.0, # kmh
        "MaxBonusSpeed" : 200.0, # kmh
        "StallRpm" : 300.0,
        "DirectionThreshold" : 0.75,
        "OutOfTrackThreshold" : 0.51,
        "ApproachDistance" : 3.5,
        "CriticalDistance" : 2.0,

        "TravelBonus" : 1.0, # once per track point
        "TravelSplineBonus" : 0.1, # once per spline point
        "DriftBonus" : 0.0,
        "SpeedBonus" : 0.0,
        "ThrottleBonus" : 0.0,
        "EngineRpmBonus" : 0.0,
        "DirectionBonus" : 0.0,

        "DirectionPenalty" : 1.0,
        "ObstApproachPenalty" : 1.0,
        "CollisionPenalty" : 1.0,
        "OffTrackPenalty" : 1.0,
        "GearGrindPenalty" : 0.0,
        "StallPenalty" : 0.0,
    }

    car_tunes = {
        "ks_toyota_supra_mkiv_drift" : {
            "FRONT_BIAS" : 53.0,
            "DIFF_POWER" : 90.0,
            "DIFF_COAST" : 90.0,
            "FINAL_RATIO" : 5.0,
            "TURBO_0" : 100.0,
            "TURBO_1" : 100.0,
        }
    }

    #==============================================================================================

    def __init__(self):

        global base_dir
        self.base_dir = base_dir
        self.sim_initialized = False
        self.viewer_initialized = False
        self.step_id = 0
        self.total_reward = 0.0

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
        pd.setCarAssists(self.sim, self.car, self.auto_clutch, self.auto_shift, self.auto_blip)
        
        tune = self.car_tunes[self.car_model]
        if tune != None:
            for name, value in tune.items():
                pd.setCarTune(self.sim, self.car, name, value)

        for name, value in self.scoring_vars.items():
            pd.setScoringVar(self.sim, self.car, name, value)
        
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
        
        if True:
            self.dcontrols.gas = u.linscalef(action[1], -1.0, 1.0, self.min_gas, self.max_gas)
            self.dcontrols.brake = u.linscalef(action[2], -1.0, 1.0, 0.0, 1.0)
        else:
            throtl = action[1]
            if (throtl > 0.0):
                self.dcontrols.gas = u.clampf(throtl, min_gas, max_gas)
            else:
                self.dcontrols.brake = throtl * -1.0

        self.dcontrols.handBrake = 0.0
        
        if not self.auto_clutch:
            self.dcontrols.clutch = 1.0
            
        if not self.auto_shift:
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
        reward = self.dstate.stepReward
        terminate = False
        truncate = False

        if self.terminate_on_hit and self.dstate.collisionFlag != 0:
            pd.writeLog('[ENV] collision')
            reward -= self.terminate_hit_penalty
            terminate = True

        if self.terminate_off_track and self.dstate.outOfTrackFlag != 0:
            pd.writeLog('[ENV] offtrack')
            reward -= self.terminate_off_track_penalty
            terminate = True

        if self.terminate_when_stuck and self.dstate.lastTrackPointTimestamp + self.stuck_timeout < self.dstate.timestamp:
            pd.writeLog('[ENV] stuck')
            reward -= self.terminate_stuck_penalty
            terminate = True

        self.total_reward += reward

        #if self.dstate.totalReward < self.terminate_low_reward:
        if self.total_reward < self.terminate_low_reward:
            pd.writeLog('[ENV] low reward')
            terminate = True

        if terminate:
            #pd.writeLog('[ENV] total reward: ' + str(self.dstate.totalReward))
            pd.writeLog('[ENV] total reward: ' + str(self.total_reward))

        return state, reward, terminate, truncate, {}

    #==============================================================================================

    def reset(self):
        pd.writeLog('[ENV] reset')

        if self.teleport_on_reset:
            pd.teleportCarByMode(self.sim, self.car, self.teleport_mode)
            
        state, reward, terminate, truncate, info = self.step(np.array([0, 0, 0]))
        
        self.step_id = 0
        self.total_reward = 0

        return state

    #==============================================================================================
    
    def _get_obs_state(self):
        
        state = np.array([

            #u.linscalef(self.dstate.engineRPM, 0.0, 20000.0, 0.0, 1.0),
            
            #self.dstate.localVelocity.x, # m/s
            #self.dstate.localVelocity.y,
            #self.dstate.localVelocity.z,

            #self.dstate.localAngularVelocity.x, # rad/s
            #self.dstate.localAngularVelocity.y,
            #self.dstate.localAngularVelocity.z,
            
            #self.dstate.tyreNdSlip[0],
            #self.dstate.tyreNdSlip[1],
            #self.dstate.tyreNdSlip[2],
            #self.dstate.tyreNdSlip[3],
            
            self.dstate.bodyVsTrack, # body_direction dot track_direction
            self.dstate.velocityVsTrack, # velocity dot track_direction

            self.dstate.lookAhead[0], # track direction change [-PI, PI]
            self.dstate.lookAhead[1],
            self.dstate.lookAhead[2],
            self.dstate.lookAhead[3],
            self.dstate.lookAhead[4],

            self.dstate.probes[0], # distance to obstacle, m
            self.dstate.probes[1],
            self.dstate.probes[2],
            self.dstate.probes[3],
            self.dstate.probes[4],
            self.dstate.probes[5],
            self.dstate.probes[6],
            
        ], dtype=np.float32)
        
        return state

    #==============================================================================================

    def _get_obs_space(self):
       
        obs_low = np.array([

            #0.0, # engineRpmNorm
            
            #-self.range_velocity, # localVelocity.x
            #-self.range_velocity, # localVelocity.y
            #-self.range_velocity, # localVelocity.z
            
            #-self.range_angularVelocity, # localAngularVelocity.x
            #-self.range_angularVelocity, # localAngularVelocity.y
            #-self.range_angularVelocity, # localAngularVelocity.z
            
            #0.0, # tyreNdSlip0
            #0.0, # tyreNdSlip1
            #0.0, # tyreNdSlip2
            #0.0, # tyreNdSlip3
            
            -1.0, # bodyVsTrack
            -1.0, # velocityVsTrack
            
            -self.range_lookAhead, # lookAhead0
            -self.range_lookAhead, # lookAhead1
            -self.range_lookAhead, # lookAhead2
            -self.range_lookAhead, # lookAhead3
            -self.range_lookAhead, # lookAhead4

            0.0, # probe0
            0.0, # probe1
            0.0, # probe2
            0.0, # probe3
            0.0, # probe4
            0.0, # probe5
            0.0, # probe6

        ], dtype=np.float32)
        
        obs_high = np.array([

            #1.0, # engineRpmNorm
            
            #self.range_velocity, # localVelocity.x
            #self.range_velocity, # localVelocity.y
            #self.range_velocity, # localVelocity.z
            
            #self.range_angularVelocity, # localAngularVelocity.x
            #self.range_angularVelocity, # localAngularVelocity.y
            #self.range_angularVelocity, # localAngularVelocity.z
            
            #self.range_tyreNdSlip, # tyreNdSlip0
            #self.range_tyreNdSlip, # tyreNdSlip1
            #self.range_tyreNdSlip, # tyreNdSlip2
            #self.range_tyreNdSlip, # tyreNdSlip3
            
            1.0, # bodyVsTrack
            1.0, # velocityVsTrack

            self.range_lookAhead, # lookAhead0
            self.range_lookAhead, # lookAhead1
            self.range_lookAhead, # lookAhead2
            self.range_lookAhead, # lookAhead3
            self.range_lookAhead, # lookAhead4
            
            self.range_probe, # probe0
            self.range_probe, # probe1
            self.range_probe, # probe2
            self.range_probe, # probe3
            self.range_probe, # probe4
            self.range_probe, # probe5
            self.range_probe, # probe6
            
        ], dtype=np.float32)
        
        return obs_low, obs_high

    #==============================================================================================

    def _get_action_space(self):

        # steer, gas, brake
        a_low  = np.array([-1, -1, -1], dtype=np.float32)
        a_high = np.array([+1, +1, +1], dtype=np.float32)

        return a_low, a_high
