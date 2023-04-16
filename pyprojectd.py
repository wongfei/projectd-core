import os
import sys
import site
import time

scriptDir = os.path.dirname(os.path.realpath(sys.argv[0]))
site.addsitedir(scriptDir + '/bin')
os.chdir(scriptDir)
#print(sys.path)

import PyProjectD as pd

print('createSimulator')
sim = pd.createSimulator(scriptDir)

print('loadTrack')
pd.loadTrack(sim, 'driftplayground')

print('addCar')
car = pd.addCar(sim, 'ks_toyota_ae86_drift')
pd.teleportCarToPits(sim, car, 0)

controls = pd.CarControls()
state = pd.CarState()

controls.gas = 1
controls.clutch = 1
controls.requestedGearIndex = 1

dt = 1.0 / 333.0
physT = 0.0
gameT = 0.0

for i in range(0, 1000):
    pd.setCarControls(sim, car, controls)
    pd.stepSimulator(sim, dt, physT, gameT)
    pd.getCarState(sim, car, state)
    physT = physT + dt
    gameT = gameT + dt

    b = state.bodyMatrix
    print("{rpm} # {pointid} # {px} {py} {pz}".format(rpm=state.engineRPM, pointid=state.nearestTrackPointId, px=b.M41, py=b.M42, pz=b.M43))

print('destroySimulator')
pd.destroySimulator(sim)
