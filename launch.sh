#!/bin/bash

XML=$PWD/Config/auto.xml
rm -rf $XML
touch $XML

(( CURRENT=0 ))
(( NODE_FREE=0 ))
(( CPT_OK=0 ))
(( NB_NODE=0 ))
MY_NODE=`cat hostfile`
echo $MY_NODE >$PWD/Config/last.txt
for node in $MY_NODE; do
    (( NB_NODE++ ))
done

if [ $NB_NODE -eq '0' ]; then 
    echo "Error : hostfile is empty !"
    exit 0
fi 

echo "<config>                                                                                                                        
  <problem>                                                                                                                           
    <type> SAT </type>                                                                                                                
    <sizeAssignment> 1 </sizeAssignment>                                                                                              
  </problem>                                                                                                                          
  <method type=\"concurrent\"/>" > $XML
echo "<solvers nb=\"$NB_NODE\">" >> $XML
#<method type=\"concurrent\"/>                                                                                                        
#<method type=\"ampharos\"/>"                                                                                                         
HOST="-l nodes="
for node in $MY_NODE; do
    (( CURRENT++ ))
    echo "                                                                                                                            
    <solver>                                                                                                                          
       <executable> $PWD/Solvers/SAT/GlucoseSyrup/bin/glucose-syrup </executable>                                                     
       <options>
        <option> -nthreads=0 </option>                                                                                                
        <option> -verb=0 </option>                                                                                                    
        <option> -no-com-display </option>
	<option> -com-how-clause-sharing=2 </option>                                                                                  
        <option> -com-alea=0 </option>                                                                                                
        <option> -com-sleeping-time-base=100 </option>                                                                                
        <option> -com-sleeping-time-sender=1000 </option>                                                                             
        <option> -com-sleeping-time-listener=100 </option>                                                                            
        <option> -com-sleeping-time-collectif=1000 </option>                                                                          
        <option> -fifosize=600000 </option>                                                                                           
        <option> BENCHNAME </option>                                                                                                  
      </options>                                                                                                                      
      <machine>                                                                                                                       
        <npernode> 1 </npernode>                                                                                                      
        <host> $node </host>                                                                                                          
      </machine>                                                                                                                      
    </solver>" >> $XML
done
 echo "                                                                                                                               
 </solvers>                                                                                                                           
</config>" >> $XML
MPIRUN="mpirun -disable-hostname-propagation -f hostfile"
echo "Launch : $MPIRUN -np 1 $PWD/Manager/bin/manager $1 $XML"

$MPIRUN -np 1 $PWD/Manager/bin/manager $1 $XML



