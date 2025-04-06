import networkx as nx
import matplotlib.pyplot as plt

class SnapshotManager:
    def __init__(self, processes):
        self.processes = processes

    def initiate_snapshot(self, initiator_pid):
        if initiator_pid in self.processes:
            initiator = self.processes[initiator_pid]
            initiator.send_marker()

    def collect_snapshots(self):
        snapshots = {}
        for pid, process in self.processes.items():
            snapshots[pid] = {
                'state': process.recorded_state,
                'in_channels': {from_pid: process.in_channels[from_pid].get_recorded_messages() for from_pid in process.in_channels}
            }
        return snapshots
    
    @staticmethod
    def visualize_snapshot(snapshot, file_name="snapshot_visualization.png"):
        """
        Wizualizuje snapshot systemu jako graf.
        
        Args:
            snapshot (dict): Słownik snapshotów w formacie:
                            {
                                process_id: {
                                    'state': int,
                                    'in_channels': {
                                        source_process_id: [message1, message2, ...],
                                        ...
                                    }
                                },
                                ...
                            }
        """
        # Tworzenie grafu skierowanego
        graph = nx.DiGraph()

        # Dodawanie wierzchołków z etykietami stanów
        for pid, data in snapshot.items():
            graph.add_node(pid, label=f"P{pid}\nState: {data['state']}")

        # Dodawanie skierowanych krawędzi z wiadomościami jako etykietami
        for pid, data in snapshot.items():
            for from_pid, messages in data['in_channels'].items():
                print(f'({from_pid}, {pid}) = {messages}')
                edge_label = f"C_{from_pid}{pid} = {messages}"
                graph.add_edge(from_pid, pid, label=edge_label)

        # Pozycje wierzchołków
        pos = nx.spring_layout(graph)

        # Rysowanie wierzchołków
        nx.draw_networkx_nodes(graph, pos, node_size=2500, node_color='cyan', edgecolors='black')
        nx.draw_networkx_labels(
            graph, pos,
            labels={pid: data['label'] for pid, data in graph.nodes(data=True)},
            font_size=10,
        )

        # Rysowanie krawędzi
        nx.draw_networkx_edges(graph, pos, arrowstyle="-|>", arrowsize=15, node_size= 2500, connectionstyle="arc3,rad=0.1")
        

        # Dodawanie etykiet do krawędzi
        edge_labels = {(u, v): data['label'] for u, v, data in graph.edges(data=True)}
        print(edge_labels)
        nx.draw_networkx_edge_labels(
            graph, 
            pos, 
            edge_labels=edge_labels, 
            connectionstyle="arc3,rad=0.1"
        )

        # Wyświetlanie grafu
        plt.axis("off")
        plt.savefig(file_name)

