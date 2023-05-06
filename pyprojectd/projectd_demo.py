import os
import sys
import site
import time

sim_hz = 333.0
sim_rate = 1.0 / sim_hz
smooth_controls = False
enable_second_sim = False
async_playground = False

base_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..')
bin_dir = os.path.join(base_dir, 'bin')
site.addsitedir(bin_dir)
os.add_dll_directory(bin_dir)

import PyProjectD as pd
pd.setLogFile(os.path.join(base_dir, 'projectd.log'), True);

if async_playground:
    pd.launchPlaygroundInOwnThread(base_dir)
else:
    pd.initPlayground(base_dir)

sim = pd.createSimulator(base_dir)
pd.loadTrack(sim, 'driftplayground')
car = pd.addCar(sim, 'gravygarage_street_ae86_readie')

pd.teleportCarToSpline(sim, car, 0.0)
pd.setCarAutoTeleport(sim, car, True, True, 1) # 0:Start, 1:Nearest, 2:Random

if enable_second_sim:
    sim2 = pd.createSimulator(base_dir)
    pd.loadTrack(sim2, 'driftplayground')
    car2 = pd.addCar(sim2, 'gravygarage_street_ae86_readie')

    pd.teleportCarToSpline(sim2, car2, 0.5)
    pd.setCarAutoTeleport(sim2, car2, True, True, 1)

while not pd.isPlaygroundInitialized():
    time.sleep(0.1)

pd.setRenderHz(int(sim_hz), not async_playground)
pd.setActiveSimulator(sim, not async_playground)
pd.setActiveCar(car, True, True)

state = pd.CarState()
controls = pd.CarControls()
controls.gas = 0.0
controls.clutch = 1
#controls.requestedGearIndex = 2 # 0=R, 1=N, 2=H1, 3=H2, 4=H3, 5=H4, 6=H5, 7=H6

pd.writeLog('MAIN LOOP')
while not pd.isPlaygroundExited():
    active_sim = pd.getActiveSimulator()
    active_car = pd.getActiveCar()
    
    pd.setCarControls(active_sim, active_car, smooth_controls, controls)
    
    if async_playground:
        pd.stepSimulator(active_sim, sim_rate)
        time.sleep(sim_rate)
        
    pd.tickPlayground()
    pd.getCarState(active_sim, active_car, state)
    rpm = state.engineRPM

pd.shutAll()
