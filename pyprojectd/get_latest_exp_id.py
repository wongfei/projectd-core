import os
import utils_d

filepath = utils_d.find_latest_file(".\\logs", ".*model.*zip$")
if filepath != None:
    dirname = os.path.basename(os.path.dirname(filepath))
    print(dirname.replace('ProjectD-v0_', ''))
else:
    print("NOT_FOUND")
