
def clampf(x, a, b):
    if x < a: return a
    if x > b: return b
    return x

def linscalef(x, x0, x1, r0, r1):
    x = clampf(x, x0, x1)
    return ((r1 - r0) * (x - x0)) / (x1 - x0) + r0

def init_cvar(obj, name):
    obj.__dict__[name] = obj.__class__.__dict__[name]
