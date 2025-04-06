import hashlib
import random
import time

class Transaction:
    """Reprezentuje transakcję w sieci."""
    def __init__(self, sender, receiver, amount):
        self.sender = sender
        self.receiver = receiver
        self.amount = amount

    def __repr__(self):
        return f"{self.sender}->{self.receiver}:{self.amount}"

class Block:
    """Reprezentuje blok w łańcuchu."""
    def __init__(self, index, transactions, previous_hash, nonce=0):
        self.index = index
        self.transactions = transactions
        self.previous_hash = previous_hash
        self.nonce = nonce
        self.hash = self.compute_hash()

    def compute_hash(self):
        block_data = (
            str(self.index) + str(self.transactions) + self.previous_hash + str(self.nonce)
        )
        return hashlib.sha256(block_data.encode()).hexdigest()

    def __repr__(self):
        return f"Block(index={self.index}, hash={self.hash[:10]}...)"

class Node:
    """Reprezentuje węzeł w sieci."""
    def __init__(self, node_id):
        self.node_id = node_id
        self.chain = []
        self.transactions_pool = []

    def create_genesis_block(self):
        genesis_block = Block(0, [], "0")
        self.chain.append(genesis_block)

    def add_transaction(self, transaction):
        self.transactions_pool.append(transaction)

    def mine_block(self, difficulty):
        if not self.transactions_pool:
            return None

        new_block = Block(
            index=len(self.chain),
            transactions=self.transactions_pool,
            previous_hash=self.chain[-1].hash,
        )
        
        while not new_block.hash.startswith("0" * difficulty):
            new_block.nonce = random.randint(0, 2**32 - 1)  # Losowanie nowego `nonce`
            new_block.hash = new_block.compute_hash()

        self.transactions_pool = []  # Clear transaction pool
        return new_block

    def add_block(self, block):
        if block.previous_hash == self.chain[-1].hash:  # Węzeł sprawdza czy poprzedni hash NOWEGO BLOKU jest taki sam jak hash OSTANIEGO BLOKU W JEGO ŁAŃCUCHU
            self.chain.append(block)

    def resolve_conflicts(self, other_chain):   # Konflikt pojawia się 
        if len(other_chain) > len(self.chain):
            self.chain = other_chain

class BlockchainNetwork:

    """Symuluje sieć blockchain."""
    def __init__(self, num_nodes, difficulty):
        self.nodes = [Node(node_id=i) for i in range(num_nodes)]    # Tworzenie węzłów
        self.difficulty = difficulty # Poziom trudności = liczba zer na początku hasha

        # Inicjalizacja każdego węzła z blokiem startowym
        for node in self.nodes:
            node.create_genesis_block() # Każdy węzeł zaczyna od tego samego bloku genesis = Block(0, [], "0") = sha256( "0" || "0" || "0" || "0" )

    # Rozgłaszanie bloku do wszystkich węzłów w sieci
    def broadcast_block(self, block, sender_node):
        for node in self.nodes:
            if node.node_id != sender_node.node_id:
                node.add_block(block)   # Dodanie bloku do łańcucha każdego węzła


    def simulate(self, num_transactions):
        # Generowanie transakcji
        for _ in range(num_transactions):
            sender = random.randint(0, len(self.nodes) - 1)
            receiver = random.randint(0, len(self.nodes) - 1)
            amount = random.randint(1, 100)

            transaction = Transaction(sender, receiver, amount)
            # Każdy węzeł dodaje wszytskie transakcję do swojego poola
            for node in self.nodes:
                node.add_transaction(transaction)

        # Symulacja miningu
        while any(node.transactions_pool for node in self.nodes):
            for node in self.nodes:
                mined_block = node.mine_block(self.difficulty)
                if mined_block:
                    print(f"Node {node.node_id} mined a block: {mined_block}")
                    self.broadcast_block(mined_block, node)

        # Przechodzimy przez wszystkie węzły i rozwiązujemy ewentualne konflikty
        longest_chain = max(self.nodes, key=lambda x: len(x.chain)).chain
        for node in self.nodes:
            node.resolve_conflicts(longest_chain)

        # Wyświetlenie finalnych łańcuchów
        for node in self.nodes:
            print(f"Node {node.node_id}'s chain: {[block.hash[:10] for block in node.chain]}\n")


if __name__ == "__main__":
    num_nodes = 5
    num_transactions = 20
    difficulty = 3

    network = BlockchainNetwork(num_nodes, difficulty)
    network.simulate(num_transactions)

    trans = Transaction(1, 2, 100)
    print(trans)
