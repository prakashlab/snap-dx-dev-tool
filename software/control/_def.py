class TriggerMode:
    SOFTWARE = 'Software Trigger'
    HARDWARE = 'Hardware Trigger'
    CONTINUOUS = 'Continuous Acqusition'
    def __init__(self):
        pass

class MicroscopeMode:
    BFDF = 'BF/DF'
    FLUORESCENCE = 'Fluorescence'
    FLUORESCENCE_PREVIEW = 'Fluorescence Preview'
    def __init__(self):
        pass

class WaitTime:
    BASE = 0.0
    X = 0.0     # per mm
    Y = 0.0	 # per mm
    Z = 0.0     # per mm
    def __init__(self):
        pass

class AF:
    STOP_THRESHOLD = 0.85
    CROP_WIDTH = 500
    CROP_HEIGHT = 500
    def __init__(self):
        pass

class Motion:
    STEPS_PER_MM_XY = 1 # change the unit from per mm to per step
    STEPS_PER_MM_Z = 1  # change the unit from per mm to per step
    def __init__(self):
        pass
'''
# for octopi-malaria
class Motion:
    STEPS_PER_MM_XY = 40
    STEPS_PER_MM_Z = 5333
    def __init__(self):
        pass
'''

class Acquisition:
    CROP_WIDTH = 2200
    CROP_HEIGHT = 2200
    NUMBER_OF_FOVS_PER_AF = 3
    IMAGE_FORMAT = 'bmp'
    IMAGE_DISPLAY_SCALING_FACTOR = 0.25
    DX = 1
    DY = 1
    DZ = 3

    def __init__(self):
        pass

class PosUpdate:
    INTERVAL_MS = 25

class MicrocontrollerDef:
    # MSG_LENGTH = 9
    CMD_LENGTH = 9
    N_BYTES_POS = 3

class MCU:
    TIMER_PERIOD_ms = 5
    TIMEPOINT_PER_UPDATE = 50
    DATA_INTERVAL_ms = TIMER_PERIOD_ms*TIMEPOINT_PER_UPDATE
    RECORD_LENGTH_BYTE = 10
    MSG_LENGTH = TIMEPOINT_PER_UPDATE*RECORD_LENGTH_BYTE

VELOCITY_MAX = 100
ACCELERATION_MAX = 500
FULLSTEPS_PER_MM = 40

SIMULATION = False    
