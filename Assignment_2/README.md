## How to run

```bash
make
```

```bash
./program2 -in inputfile -sw switchingMethod
```

where `switchingMethod` is `Omega/Delta/Benes`.

The output will be printing to `stdout`.

The program works as expected. There are no known bugs or issues. The output will be printed in this format (just an example)):

```
T T C
T C C
C C T
C C T
Dropped packets
6
7
```

### Sample tests

`inputfile (N = 8, A = 6)`

```plaintext
8
6
1
2
4
5
6
7
```

1.  `omega.txt`

```plaintext
T T C
T C C
C C T
C C T
```

2.  `delta.txt`

```plaintext
T T C
T C C
C C T
C C T
```

3.  `benes.txt`

```plaintext
T T T T C
T T C C C
T T T C T
T T C C T
```
