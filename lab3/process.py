import threading
from channel import Channel

class Process:
    def __init__(self, pid):
        self.pid = pid
        self.state = 0
        self.in_channels = {}
        self.out_channels = {}
        # Zmienne do obsługi algorytmu Chandy-Lamporta
        self.snapshot_marker_received = set()
        self.recorded_state = None
        
        self.lock = threading.Lock()

    def connect_to(self, other_process):
        channel_self_other = Channel()
        self.out_channels[other_process.pid] = channel_self_other
        other_process.in_channels[self.pid] = channel_self_other
        # Interpretacja polecenia:
        # Kanały komunikacyjne - jednokierunkowe (FIFO) istnieje ścieżka komunikacyjna między każdą parą procesów w systemie. 
        chanel_other_self = Channel()
        other_process.out_channels[self.pid] = chanel_other_self
        self.in_channels[other_process.pid] = chanel_other_self

    def send_message(self, to_pid, message):
        if to_pid in self.out_channels:
            self.out_channels[to_pid].send(message)

    def receive_message(self, from_pid):
        if from_pid in self.in_channels:
            return self.in_channels[from_pid].receive()

    def update_state(self, amount):
        with self.lock:
            self.state += amount

    # Algorytm Chandy-Lamporta
    # def take_snapshot(self):
    def send_marker(self):
        # zapisujemy swój stan
        self.recorded_state = self.state
        # informujemy wszystkie kanały wejściowe, żeby zaczęły nagrywać
        for channel in self.in_channels.values():
            channel.start_recording()
        # wysyłamy markery do wszystkich procesów
        for pid in self.out_channels:
            self.send_message(pid, 'MARKER')

    def receive_marker(self, from_pid):
        if self.recorded_state is None:
            self.send_marker()

        # zapisujemy, że otrzymaliśmy marker od procesu from_pid
        if from_pid not in self.snapshot_marker_received:
            self.snapshot_marker_received.add(from_pid)

        # jeśli otrzymaliśmy markery na każdym kanale wejściowym, to zatrzymujemy nagrywanie
        if len(self.snapshot_marker_received) == len(self.in_channels):
            for channel in self.in_channels.values():
                channel.stop_recording()

