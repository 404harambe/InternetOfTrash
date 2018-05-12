import threading


def synchronized_with_attr(lock_name):
    def decorator(method):
        def synced_method(self, *args, **kws):
            lock = getattr(self, lock_name)
            with lock:
                return method(self, *args, **kws)

        return synced_method

    return decorator


class SyncOrderedList:

    def __init__(self):
        self.syncList = []
        self.lock = threading.RLock()


    @synchronized_with_attr("lock")
    def is_in(self, arduino_id):
        for i in range(len(self.syncList)):
            if arduino_id == self.syncList[i][0]:
                return i

        return -1


    @synchronized_with_attr("lock")
    def put(self, arduino_id, time=0):
        print('put request')
        idx = self.is_in(arduino_id)
        if idx > -1:
            self.remove_element(idx)
            print("Removing arduino %s" % (arduino_id))
        for i in range(len(self.syncList)):
                if time <= self.syncList[i][1]:
                        print("Updating time for arduino %s from %f to %f" % (arduino_id, self.syncList[i][1], time))
                        self.syncList.insert(i, (arduino_id, time))
                        return
        self.syncList.append((arduino_id, time))
        print("Now listening for arduino %s" % (arduino_id))


    @synchronized_with_attr("lock")
    def remove_element(self, index):
        del(self.syncList[index])


    @synchronized_with_attr("lock")
    def get_first(self):
        if len(self.syncList) == 0:
            raise ValueError("Empty list")
        return self.syncList[0]

    @synchronized_with_attr("lock")
    def wait_for_task(self):
        if self.empty():
            return 3600
        else:
            return int(self.get_first()[1])

    @synchronized_with_attr("lock")
    def pop(self):
        if len(self.syncList) == 0:
            raise ValueError("Empty list")

        first = self.syncList[0]
        self.syncList = self.syncList[1:]
        return first


    @synchronized_with_attr("lock")
    def empty(self):
        return len(self.syncList) == 0

