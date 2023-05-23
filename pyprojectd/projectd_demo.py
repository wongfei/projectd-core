import os
import sys
import site
import time

sim_hz = 333.0
sim_rate = 1.0 / sim_hz
smooth_controls = False
enable_second_sim = False
async_playground = False

teleport_on_hit = False
teleport_off_track = False

auto_clutch = True
auto_shift = True
auto_blip = True

track_name = 'driftplayground'
car_model = 'ks_toyota_supra_mkiv_drift'

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
pd.loadTrack(sim, track_name)
car = pd.addCar(sim, car_model)

pd.teleportCarToSpline(sim, car, 0.01)
pd.setCarAutoTeleport(sim, car, teleport_on_hit, teleport_off_track, 1) # 0:Start, 1:Nearest, 2:Random
pd.setCarAssists(sim, car, auto_clutch, auto_shift, auto_blip)

tune = car_tunes[car_model]
if tune != None:
    for name, value in tune.items():
        pd.setCarTune(sim, car, name, value)

if enable_second_sim:
    sim2 = pd.createSimulator(base_dir)
    pd.loadTrack(sim2, track_name)
    car2 = pd.addCar(sim2, car_model)

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
