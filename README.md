<h1 align="center"> D-Syrup </h1>

D-Syrup is an original distributed SAT solver based on the portfolio paradigm. This solver, dedicated to work on plenty of cores, implements a new fully hybrid distributed programming model from Syrup.

See http://www.cril.univ-artois.fr/dsyrup/ for more details.

# Install

To install the necessary components :
- Install g++ with c++2011 minimum
- Install an MPI implementation (MPICH2 preferred) 
  with the mode MPI_THREADS_MULTIPLE (Option to enable during the installation of the MPI implementation)
- Install paquets libxml2-dev and pthread 2011

To compile the solver :
```console
make clean
make -j
```

To configure :
- You can change options in the file Config/concurrentSAT.xml. 
  - The number of computers  
  - the number of threads
  - The programming model  
  - ...
- All possibilities for options are described in the file OptionCoding.h. 

# Run

To launch dSyrup in concurent mode on a computer :
```console
mpirun -np 1 Manager/bin/manager <CNF> Config/concurrentSAT.xml 
```

To launch dSyrup with the dynamic Divide and Conquer mode of Ampharos: 
```console
mpirun -np 1 Manager/bin/manager <CNF> Config/ampharosSAT.xml  
```

To launch with a specific hostfile (launch.sh create the good XML configuration file) :
./launch.sh <CNF>  

If you detect a bug don't hesitate to contact me : szczepanski.nicolas@gmail.com
