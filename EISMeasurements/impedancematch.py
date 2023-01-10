# python3

import numpy as np
import matplotlib.pyplot as plt
import sys

# the latest pip version is limited to numpy < 1.20, and I have more recent
sys.path.insert(0, './EISMeasurements/impedance.py/')

from impedance.visualization import plot_nyquist
from impedance.models.circuits import CustomCircuit
from impedance import preprocessing


# Load data from the example EIS data
#frequencies, Z = preprocessing.readCSV('EISMeasurements/batt_0.2-10k.csv')
frequencies, Z = preprocessing.readZPlot('EISMeasurements/batt_0.2-10k.z')

# keep only the impedance data in the first quandrant
#frequencies, Z = preprocessing.ignoreBelowX(frequencies, Z)
frequencies, Z = preprocessing.cropFrequencies(frequencies, Z, freqmax=5000)


circuitstr = 'L0-R0-p(R1,CPE1)-p(R2-Wo1,CPE2)'

#initial_guess = [0, 0.05, 0.003, 7,None, 0.01, 0.05, 100, 0.4,None]
#constants = {"CPE1_1":1,"CPE2_1":1}
initial_guess = [0, 0.05, 0.003, 7,1, 0.01, 0.05, 100, 0.4,1]
constants = {}

circuit = CustomCircuit(circuitstr, initial_guess=initial_guess, constants=constants)

circuit.fit(frequencies, Z, global_opt = False)

print(circuit)
circuit.save("./EISMeasurements/impedancefit.json")

frequencies = np.append(frequencies, [0.1,0.05,0.02,0.01])
frequencies = np.sort(frequencies)
Z_fit = circuit.predict(frequencies)

fig, ax = plt.subplots()

plot_nyquist(Z, ax=ax, fmt='o')
plot_nyquist(Z_fit, ax=ax, fmt='-')

plt.legend(['Data', 'Fit'])

plt.show()
