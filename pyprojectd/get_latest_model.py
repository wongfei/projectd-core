import os
import utils_d

filepath = utils_d.find_latest_file(".\\logs", ".*model.*zip$")
if filepath != None:
    print(filepath)
else:
    print("NOT_FOUND")
