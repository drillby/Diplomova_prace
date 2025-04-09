import random
import socket
import threading
import time

# --- Konstanty ---
TCP_PORT = 8888
REFRESH_RATE_HZ = 1  # 1Hz pro snadné testování
REFRESH_INTERVAL = 1.0 / REFRESH_RATE_HZ
MAX_SENSORS = 4


# --- Simulace EMG senzoru ---
class EMGSensorMock:
    def read_voltage(self, reference_voltage=5.0, resolution=1023):
        # Náhodně generovaná hodnota mezi 0 a 5V
        return random.uniform(0, reference_voltage)


# --- TCP Server pro EMG ---
class EMGSystem:
    def __init__(self, port):
        self.port = port
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket = None
        self.client_address = None
        self.sensors = []
        self.last_sent_value = -1
        self.initialized = False

    def start_server(self):
        self.server_socket.bind(("", self.port))
        self.server_socket.listen(1)
        print(f"Server spuštěn na portu {self.port}")
        threading.Thread(target=self.accept_clients, daemon=True).start()

    def accept_clients(self):
        while True:
            client_socket, addr = self.server_socket.accept()
            print(f"Klient připojen: {addr}")
            self.client_socket = client_socket
            self.client_address = addr
            self.handle_client()

    def handle_client(self):
        try:
            self.client_socket.settimeout(3.0)
            input_data = self.client_socket.recv(10).decode().strip()
            print(f"Přijatý vstup: {input_data}")
            count = int(input_data)

            if 1 <= count <= MAX_SENSORS:
                self.init_sensors(count)
                self.initialized = True
                self.client_socket.settimeout(None)
                self.send_loop()
            else:
                print("Neplatný počet senzorů.")
                self.client_socket.close()

        except (ValueError, socket.timeout) as e:
            print("Chyba během čtení od klienta:", e)
            self.client_socket.close()

    def init_sensors(self, count):
        self.sensors = [EMGSensorMock() for _ in range(count)]
        print(f"Inicializováno {count} EMG senzor(ů).")

    def send_loop(self):
        try:
            while self.initialized and self.client_socket:
                # kontrola příkazu DISCONNECT
                self.client_socket.setblocking(False)
                try:
                    cmd = self.client_socket.recv(64).decode()
                    if "DISCONNECT" in cmd:
                        print("Klient odeslal DISCONNECT.")
                        break
                except BlockingIOError:
                    pass
                self.client_socket.setblocking(True)

                sensor_data = [1 if s.read_voltage() > 1.6 else 0 for s in self.sensors]
                decimal_value = sum(
                    val << (len(sensor_data) - 1 - i)
                    for i, val in enumerate(sensor_data)
                )

                if decimal_value != self.last_sent_value:
                    self.last_sent_value = decimal_value
                    msg = f"{decimal_value}\n"
                    self.client_socket.sendall(msg.encode())
                    print(f"Odesláno: {msg.strip()}")

                time.sleep(REFRESH_INTERVAL)

        except Exception as e:
            print("Chyba během komunikace:", e)
        finally:
            self.cleanup_client()

    def cleanup_client(self):
        if self.client_socket:
            self.client_socket.close()
        self.client_socket = None
        self.sensors = []
        self.initialized = False
        self.last_sent_value = -1
        print("Klient odpojen a systém resetován.")


# --- Spuštění serveru ---
if __name__ == "__main__":
    emg = EMGSystem(TCP_PORT)
    emg.start_server()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Server ukončen.")
