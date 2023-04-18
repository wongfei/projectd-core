import os
import sys
import site
import time

baseDir = os.path.dirname(os.path.realpath(sys.argv[0]))
site.addsitedir(os.path.join(baseDir, 'bin'))
#print(sys.path)

import PyProjectD as pd

pd.initLogFile(os.path.join(baseDir, 'pyprojectd.log'))

print('createSimulator')
sim = pd.createSimulator(baseDir)

print('loadTrack')
pd.loadTrack(sim, 'driftplayground')

print('addCar')
car = pd.addCar(sim, 'ks_toyota_ae86_drift')
pd.teleportCarToPits(sim, car, 0)

controls = pd.CarControls()
state = pd.CarState()

controls.gas = 1
controls.clutch = 1
controls.requestedGearIndex = 1 # 0=R, 1=N, 2=H1, 3=H2, 4=H3, 5=H4, 6=H5, 7=H6

for i in range(0, 1000):
    pd.setCarControls(sim, car, controls)
    pd.stepSimulator(sim)
    pd.getCarState(sim, car, state)

    b = state.bodyMatrix
    print("{rpm} # {pointid} # {px} {py} {pz}".format(rpm=state.engineRPM, pointid=state.nearestTrackPointId, px=b.M41, py=b.M42, pz=b.M43))

print('destroySimulator')
pd.destroySimulator(sim)

pd.closeLogFile()
