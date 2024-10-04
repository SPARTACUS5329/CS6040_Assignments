## Assignment 3 - CS6040

**Submission by: Aditya K (CH21B005)**

Command to run the program for a particular input

```
./routing -N switchportcount -B buffersize -p packetgenprob \
 -q NOQ | INQ | CIOQ -K knockout -L inpq -o outputfile -T maxslots
```

To execute the entire simulation

```
chmod +x run
./run
```

OR you can manually run (assuming `numpy` and `matplotlib` are installed)

```
<your-python-version> simulate.py
```

The outputs for the simulation will be in the `outputs/` directory with sub-directories for each method.

The report for the simulations is present as `Report.pdf`.
