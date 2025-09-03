from rpcf.rpcbase import * 

class NamedProcessLock:
    def __init__(self, lock_path:str):
        self.sem = cpp.PyNamedProcessLock( lock_path )
        self.locked = False

    def Acquire(self):
        self.sem.Acquire()
        self.locked = True

    def Release(self):
        if self.locked:
            self.sem.Release()
            self.locked = False

    def __enter__(self):
        self.Acquire()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.Release()

    def __del__(self):
        try:
            self.Release()
        except Exception:
            pass