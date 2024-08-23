This is the submission for Assignment_1 of Router Architecture (CS6040) by Aditya K (CH21B005)

The following files have been used in the assignment:

1. `routing.c`: Main file
2. `routing.h`: Header file
3. `routing`: Executable that takes in the parameters mentioned
4. `topologyfile`: Sample input file for the graph network
5. `connectionsfile`: Sample input file for requests
6. `routingtablefile`: Output file from sample inputs for information on each processed request
7. `forwardingfile`: Output file from sample inputs for VCID forwarding table for each node
8. `pathsfile`: Output file from sample inputs for information on admitted connections

#### How to compile the program

```
make
```

OR

```
gcc routing.c -o routing
```

#### How to run the program

```
./routing -top topologyfile -conn connectionsfile -rt routingtablefile \
 -ft forwardingfile -path pathsfile -flag hop -p 0

```
