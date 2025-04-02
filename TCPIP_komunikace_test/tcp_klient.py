import socket

arduino_ip = "192.168.132.225"
arduino_port = 8888

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    client_socket.connect((arduino_ip, arduino_port))
    print(f"Připojeno k Arduinu na {arduino_ip}:{arduino_port}")

    # Pošli první znak, aby se Arduino "aktivovalo"
    client_socket.sendall(b"\n")

    # Získej file-like objekt pro čtení po řádcích
    with client_socket.makefile("r") as sock_file:
        for line in sock_file:
            value = line.strip()  # odstraní \n a mezery
            if value.isdigit():
                print(f"Přijatá hodnota: {value}")
            else:
                print(f"Neplatná data: {value}")

except Exception as e:
    print("Chyba při komunikaci:", e)

finally:
    client_socket.close()
    print("Odpojeno.")
