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

The program will take `topologyfile` and `connectionsfile` as input in the format specified in the problem statement.

All outputfiles use "|" as a delimited in each line. The output files are:

* `routingtablefile`: It contains a table with
  * source
  * destination
  * pathIndex - 0 or 1 for shortest or second shortest
  * path - Node IDs separated by "->" to indicate order
  * pathDelay
  * pathCost - Dependent on "hop" | "dist"
* `forwardingfile`: It contains a table with
  * routerID
  * incomingNode
  * incomingVCID
  * outgoingNode
  * outgoingVCID
* `pathsfile`: First line is two integers representing total number of requests and total admitted connections. It also contains a table
  * connectionID
  * sourceNode
  * destinationNode
  * Path - nodes separated by "->" for order
  * VC ID List - along the path
  * pathCost - Dependent on "hop" | "dist"

#### Observations on ARPANET and NSFNET

The connections are successful at a rate of 1-5% in the case of 100 requests and it goes lower to about 0.5 - 1.5% in the cases of 200 and 300 requests in both "hop" and "dist" with the *optimistic approach*. This low rate of success can be attributed to the low capacity of the links and that the requirements of requests in terms of capacity in a link can only be an integer. Since there is not much room available in the links after some connections are established, the subsequent ones fail, since all links should satisfy the minimum requirement condition.
