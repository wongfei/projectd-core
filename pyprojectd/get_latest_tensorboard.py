import os
import utils_d

filepath = utils_d.find_latest_file(".\\logs_tb", "events.out.tfevents.*")
if filepath != None:
    print(os.path.dirname(filepath))
else:
    print("NOT_FOUND")
