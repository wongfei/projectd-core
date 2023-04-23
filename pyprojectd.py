import os
import sys
import site
import time

baseDir = os.path.dirname(os.path.realpath(sys.argv[0]))
site.addsitedir(os.path.join(baseDir, 'bin'))
os.add_dll_directory(os.path.join(baseDir, 'bin'))
#print(sys.path)

import PyProjectD as pd
pd.initLogFile(os.path.join(baseDir, 'projectd.log'))

sim = pd.createSimulator(baseDir)
pd.loadTrack(sim, 'driftplayground')
car = pd.addCar(sim, 'gravygarage_street_ae86_readie')

#pd.teleportCarToLocation(sim, car, 70.57, 66.34, 14.29)
#pd.teleportCarToPits(sim, car, 0) # simId, carId, pitId
pd.teleportCarToTrackLocation(sim, car, 1, 0.1) # simId, carId, distanceNorm, offsetY

pd.initPlayground(baseDir)
pd.setActiveSimulator(sim, True) # simId, simEnabled
pd.setActiveCar(car, True, True) # carId, takeControls, enableSound

state = pd.CarState()
controls = pd.CarControls()
controls.gas = 1
controls.clutch = 1
controls.requestedGearIndex = 1 # 0=R, 1=N, 2=H1, 3=H2, 4=H3, 5=H4, 6=H5, 7=H6

pd.writeLog('MAIN LOOP')
#for i in range(0, 1000):
while not pd.isPlaygroundExited():
    #pd.setCarControls(sim, car, controls)
    #pd.stepSimulator(sim)
    pd.tickPlayground()
    pd.getCarState(sim, car, state)
    #b = state.bodyMatrix
    #print("{rpm} # {pointid} # {px} {py} {pz}".format(rpm=state.engineRPM, pointid=state.nearestTrackPointId, px=b.M41, py=b.M42, pz=b.M43))

pd.shutPlayground()
pd.destroySimulator(sim)
pd.closeLogFile()
