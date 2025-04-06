from process import Process
from snapshot import SnapshotManager

def receive_dolars_and_update_account(process, from_pid):
    message = process.receive_message(from_pid)
    if message.isdigit():
        dolars = int(message)
        process.update_state(dolars)


def update_acount_and_send_dolars(process, to_pid, dolars):
    process.update_state(-dolars)
    process.send_message(to_pid, str(dolars))


# Tworzenie procesów
p1 = Process(1)
p2 = Process(2)
p3 = Process(3)
# ...

# Tworzenie połączeń
p1.connect_to(p2)
p2.connect_to(p3)
p3.connect_to(p1)
# ...

# Symulacja
p1.update_state(10)
p2.update_state(20)
p3.update_state(30)
# ...

# Akcje przed rozpoczęciem snapshota (wysłaniem pierwszego markera)
update_acount_and_send_dolars(p1, 2, 1)
update_acount_and_send_dolars(p1, 3, 1)
update_acount_and_send_dolars(p2, 3, 1)
update_acount_and_send_dolars(p2, 1, 1)
update_acount_and_send_dolars(p3, 1, 1)
update_acount_and_send_dolars(p3, 2, 1)
update_acount_and_send_dolars(p3, 2, 1)

# Uruchomienie snapshotu
snapshot_manager = SnapshotManager({1: p1, 2: p2, 3: p3})
snapshot_manager.initiate_snapshot(1)

# Akcje po rozpoczęciu snapshota
receive_dolars_and_update_account(p1, 2) # C_12 = {1}
receive_dolars_and_update_account(p2, 1)
receive_dolars_and_update_account(p2, 3)
receive_dolars_and_update_account(p3, 1)

p2.receive_marker(1)
p3.receive_marker(1)

receive_dolars_and_update_account(p1, 3)
receive_dolars_and_update_account(p2, 3)
receive_dolars_and_update_account(p3, 2)

p1.receive_marker(2)
p1.receive_marker(3)
p2.receive_marker(3)
p3.receive_marker(2)

snapshot_manager.collect_snapshots()

print('Snapshot collected:', snapshot_manager.collect_snapshots())
