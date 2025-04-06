import hashlib
import colorama
from colorama import Fore, Style
import random
import time
from multiprocessing import Process, Manager, Lock
import multiprocessing
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class Transaction:
    """Reprezentuje transakcję w sieci."""
    def __init__(self, sender, receiver, amount):
        self.sender = sender
        self.receiver = receiver
        self.amount = amount

    def __repr__(self):
        return f"{self.sender}->{self.receiver}:{self.amount}"
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class Block:
    """Reprezentuje blok w łańcuchu."""
    def __init__(self, index, transactions, previous_hash, nonce=0):
        self.index = index
        self.transactions = transactions
        self.previous_hash = previous_hash
        self.nonce = nonce  # A Bitcoin nonce is a 32-bit (or 4-byte) number that miners use to attempt to generate a valid hash to produce a new block in the Bitcoin blockchain. The nonce is the number that blockchain miners are solving for.
        self.hash = self.compute_hash()

    # Hash obliczany na podstawie ID bloku, transakcji, hash poprzedniego bloku i losowej wartości nonce.
    def compute_hash(self):
        block_data = (
            str(self.index) + str(self.transactions) + self.previous_hash + str(self.nonce)
        )
        return hashlib.sha256(block_data.encode()).hexdigest()

    def __repr__(self):
        return f"Block(index={self.index}, hash={self.hash[:10]}...)"
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class Node(Process):
    """Reprezentuje węzeł w sieci."""
    def __init__(self, node_id, network, lock, difficulty):
        super().__init__()
        self.node_id = node_id
        self.chain = []                 # Lokalny łańcuch next_transbloków !! Inne węzły nie zmieniają tego bloku !!
        self.transactions_pool = []     # Lista transakcji oczekujących na zatwierdzenie
        self.network = network
        self.lock = lock
        self.difficulty = difficulty    # Poziom trudności = liczba zer na początku hasha (parametr `k`)
        self.popped_num = 0

    # Tworzenie bloku startowego --> każdy węzeł zaczyna od tego bloku
    def create_genesis_block(self):
        genesis_block = Block(0, [], "0")
        self.chain.append(genesis_block)
        with self.lock:
            self.network[self.node_id].append(genesis_block)

    def add_transaction(self, transaction):
        self.transactions_pool.append(transaction)

    def mine_block(self):
        if not self.transactions_pool:
            return None

        next_trans = self.transactions_pool.pop(0)  # Wybieramy pierwszą transakcję z puli transakcji
        self.popped_num += 1
        
        new_block = Block(
            index=len(self.chain),
            transactions=next_trans,
            previous_hash=self.chain[-1].hash,
        )

        # Cała trudność polega na znalezieniu takiego `nonce`, aby hash bloku zaczynał się od `k` zer
        while not new_block.hash.startswith("0" * self.difficulty):
            # new_block.nonce = random.randint(0, 2**32 - 1)  # Losowanie nowego `nonce`
            new_block.nonce += 1
            new_block.hash = new_block.compute_hash()

        # Kluczowy moment
        # self.transactions_pool.remove()  # Clear transaction pool
        # self.transactions_pool.pop(0) # Usuwamy pojedynczą transakcję z puli transakcji, którą zakodowaliśmy w bloku

        return new_block

    def add_block(self, block):
        if block.previous_hash == self.chain[-1].hash:
            self.chain.append(block)

    def resolve_conflicts(self):
        with self.lock:
            longest_chain = max(self.network.values(), key=len)
            # print(f"Node {self.node_id}: longest_chain in network (Len: {len(longest_chain)})\n{longest_chain} ")
            # print("longest: ", longest_chain)
            # print("self: ", self.chain)
            if len(longest_chain) > len(self.chain):
                print(f"Node {self.node_id}: Someone was faster :(\nMy chain: {self.chain[-3:]}\nLongest chain: {longest_chain[-3:]}")
                self.chain = [b for b in longest_chain]
                self.network[self.node_id] = [b for b in longest_chain]
                # Nie chcemy kodować transakcji które już są w łańcuchu (ktoś wcześniej zakodował je w blok)
                self.transactions_pool = self.transactions_pool[len(longest_chain) - len(self.chain):]
                print(f"Node {self.node_id}: My chain is now len {len(self.chain)}: {self.chain[-3:]}")
                print(f"{Fore.GREEN}Node {self.node_id}: My transactions pool is now len {len(self.transactions_pool)}: {self.transactions_pool}{Style.RESET_ALL}")
            else:
                print(f"Node {self.node_id}: I'm still in the race")
            
    def resolve_conflicts_2(self):
        # with network.lock:
        with self.lock:
            # print(f"Network: {self.network}")
            longest_chain = max(self.network.values(), key=len)
            # print(f"Node {self.node_id}: longest_chain in network (Len: {len(longest_chain)})\n{longest_chain} ")
            # print("longest: ", longest_chain)
            # print("self: ", self.chain)
            if len(longest_chain) > len(self.network[self.node_id]):
                print(f"Node {self.node_id}: Someone was faster :(\nMy chain: {self.network[self.node_id][-3:]}\nLongest chain: {longest_chain[-3:]}")
                # self.chain = [b for b in longest_chain]
                self.network[self.node_id] = [b for b in longest_chain]
                # Nie chcemy kodować transakcji które już są w łańcuchu (ktoś wcześniej zakodował je w blok)
                self.transactions_pool = self.transactions_pool[len(longest_chain) - self.popped_num:]
                print(f"Node {self.node_id}: My chain is now len {len(self.network[self.node_id])}: {self.network[self.node_id][-3:]}")
                print(f"{Fore.GREEN}Node {self.node_id}: My transactions pool is now len {len(self.transactions_pool)}: {self.transactions_pool}{Style.RESET_ALL}")
            else:
                print(f"Node {self.node_id}: I'm still in the race")


    # def get_best_broadcasted_block(self) -> Block:
    #     with self.lock:
    #         if self.network[self.node_id]:
    #             best_block = max(self.network[self.node_id], key=lambda block: block.index)
    #             self.network[self.node_id] = []
    #             return best_block
    #         return None
                
    def broadcast_block(self, block):
        with self.lock:
            self.network[self.node_id].append(block)
            # print("I theoreticcly added block. My chain in network =", self.network[self.node_id])

    def broadcast_block_2(self, block):
        with self.lock:
            if self.network[self.node_id][-1].hash == block.previous_hash:
                self.network[self.node_id].append(block)


    def run(self):
        self.create_genesis_block() # Tworzymy blok startowy

        # Zaczyna się walka o nowe bloki
        while self.transactions_pool:   # kończymy działanie węzła, gdy nie ma już transakcji w puli
    
            mined_block = self.mine_block() # Węzeł próbuje wykopać nowy blok
    
            if mined_block:
                print(f"Node {self.node_id} mined a block: {Fore.CYAN}{mined_block}{Style.RESET_ALL}")
                self.add_block(mined_block)
                self.broadcast_block(mined_block)

            self.resolve_conflicts()
            # time.sleep(1)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class BlockchainNetwork:
    """Symuluje sieć blockchain."""
    def __init__(self, num_nodes, difficulty):
        self.manager = Manager()
        self.network = self.manager.dict({i: self.manager.list() for i in range(num_nodes)})
        self.lock = Lock()
        self.nodes = [Node(i, self.network, self.lock, difficulty) for i in range(num_nodes)]

    def start_network(self):
        for node in self.nodes:
            node.start()

    def stop_network(self):
        for node in self.nodes:
            node.join()

    def add_transaction(self, transaction):
        for node in self.nodes:
            node.add_transaction(transaction)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


if __name__ == "__main__":

    num_nodes = 5
    num_transactions = 20
    difficulty = 5

    blockchain_network = BlockchainNetwork(num_nodes, difficulty)
    # Generowanie losowych transakcji
    for _ in range(num_transactions):
        sender = random.randint(0, num_nodes - 1)
        receiver = random.randint(0, num_nodes - 1)
        amount = random.randint(1, 100)

        transaction = Transaction(sender, receiver, amount)
        blockchain_network.add_transaction(transaction)

    blockchain_network.start_network()
    print ("Network started")
    blockchain_network.stop_network()
    print ("Network stopped")
