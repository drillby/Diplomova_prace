import sys
import time
from collections import deque

import pyqtgraph as pg
import serial
from pyqtgraph.Qt import QtCore, QtWidgets

# --- Parametry sériové komunikace ---
PORT = "/dev/ttyACM0"
BAUDRATE = 9600

# --- Inicializace sériového portu ---
ser = serial.Serial(PORT, BAUDRATE)
time.sleep(2)

# --- Parametry grafu ---
WINDOW_WIDTH_SEC = 5
SAMPLING_INTERVAL = 0.01
MAX_SAMPLES = int(WINDOW_WIDTH_SEC / SAMPLING_INTERVAL)

# --- Inicializace dat ---
data_queues = []
curves = []
timestamps = [i * SAMPLING_INTERVAL for i in range(MAX_SAMPLES)]

# --- Aplikace ---
app = QtWidgets.QApplication(sys.argv)
win = pg.GraphicsLayoutWidget(title="EMG signály v reálném čase")
win.show()
win.setWindowTitle("EMG Realtime Plot")


def update():
    global data_queues, curves

    while ser.in_waiting:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        print(f"[DEBUG] Přijatá zpráva: {line}")

        if line.startswith("IP adresa:") or not line:
            continue

        try:
            values = [float(v) for v in line.split(",")]
        except ValueError:
            print(f"[WARNING] Neplatný řádek (nelze převést): {line}")
            continue

        if len(data_queues) != len(values):
            print(f"[INFO] Nový počet kanálů: {len(values)}")
            data_queues.clear()
            curves.clear()
            win.clear()

            num_cols = int(len(values) ** 0.5 + 0.9999)  # zaokrouhlení nahoru
            for i in range(len(values)):
                queue = deque([0] * MAX_SAMPLES, maxlen=MAX_SAMPLES)
                data_queues.append(queue)

                row = i // num_cols
                col = i % num_cols

                plot = win.addPlot(row=row, col=col, title=f"Kanál {i + 1}")
                plot.setLabel("left", "Hodnota")
                plot.setLabel("bottom", "Čas", units="s")
                plot.setXRange(0, WINDOW_WIDTH_SEC)
                plot.setYRange(-1, 1)
                plot.showGrid(x=True, y=True)

                curve = plot.plot(timestamps, list(queue))
                curves.append(curve)

        for i, value in enumerate(values):
            data_queues[i].append(value)
            curves[i].setData(timestamps, list(data_queues[i]))


# --- Timer ---
timer = QtCore.QTimer()
timer.timeout.connect(update)
timer.start(int(SAMPLING_INTERVAL * 1000))

# --- Spuštění aplikace ---
if __name__ == "__main__":
    QtWidgets.QApplication.instance().exec_()
