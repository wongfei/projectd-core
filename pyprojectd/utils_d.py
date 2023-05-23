import os
import re

def clampf(x, a, b):
    if x < a: return a
    if x > b: return b
    return x

def linscalef(x, x0, x1, r0, r1):
    x = clampf(x, x0, x1)
    return ((r1 - r0) * (x - x0)) / (x1 - x0) + r0

def init_cvar(obj, name):
    obj.__dict__[name] = obj.__class__.__dict__[name]

def find_latest_file(path, filter):

    pairs = []

    for currentpath, folders, files in os.walk(path):
        for file in files:
            filepath = os.path.join(currentpath, file)
            if re.search(filter, filepath) != None:
                mtime = os.path.getmtime(filepath)
                pair = {'filepath' : filepath, 'mtime' : mtime}
                pairs.append(pair)

    def sort_by_mtime(a, b):
        if a.mtime < b.mtime:
            return -1
        if a.mtime > b.mtime:
            return 1
        return 0

    sorted(pairs, key=lambda x: x['mtime'])

    if len(pairs) > 0:
        return pairs[len(pairs) - 1]['filepath']
        
    return None
