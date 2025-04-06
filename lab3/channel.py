from queue import Queue

class Channel:
    def __init__(self):
        self.queue = Queue()
        self.recording = False
        self.recorded_messages = []

    def send(self, message):
        self.queue.put(message)

    def receive(self):
        message = self.queue.get()
        if self.recording:
            self.recorded_messages.append(message)
        return message
    
    def start_recording(self):
        self.recording = True

    def stop_recording(self):
        self.recording = False

    def get_recorded_messages(self):
        return self.recorded_messages
    
    def clear_recorded_messages(self):
        self.recorded_messages = []
