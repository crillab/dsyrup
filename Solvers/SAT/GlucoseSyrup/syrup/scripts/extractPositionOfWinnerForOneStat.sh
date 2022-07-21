#!/bin/csh
# 
# take all STDOUT answered SAT/UNSAT
# /extractPositionOfWinnerForOneStat.sh 'Conflicts' 'none'  : count how many times the winner is the best, the second one....
#output position allBenchs SAT UNSAT

rm -rf /tmp/data.txt
rm /tmp/sat.txt /tmp/unsat.txt /tmp/partial.txt

foreach file (`grep -l -e "^s" STD*`)
set w=`grep 'finish!' $file|cut -d' ' -f3`
awk -F'|' -v stat=$1 -v winner=$w -v div=$2 -f extractPositionOfWinnerForOneStat.awk $file >> /tmp/data.txt
end;

sort /tmp/data.txt |awk 'BEGIN{cur=-1;nb=0;}{if(cur==-1) cur=$1;if(cur!=$1) {printf("%d %d\n",cur,nb);nb=1;cur=$1;}else nb++;}END{printf("%d %d\n",cur,nb);}' > /tmp/all.txt

rm -rf /tmp/data.txt
foreach file (`grep -l -e "^s UN" STD*`)
set w=`grep 'finish!' $file|cut -d' ' -f3`
awk -F'|' -v stat=$1 -v winner=$w -v div=$2 -f extractPositionOfWinnerForOneStat.awk $file >> /tmp/data.txt
end;

sort /tmp/data.txt |awk 'BEGIN{cur=-1;nb=0;}{if(cur==-1) cur=$1;if(cur!=$1) {printf("%d %d\n",cur,nb);nb=1;cur=$1;}else nb++;}END{printf("%d %d\n",cur,nb);}' > /tmp/unsat.txt

rm -rf /tmp/data.txt
foreach file (`grep -l -e "^s SA" STD*`)
set w=`grep 'finish!' $file|cut -d' ' -f3`
awk -F'|' -v stat=$1 -v winner=$w -v div=$2 -f extractPositionOfWinnerForOneStat.awk $file >> /tmp/data.txt
end;

sort /tmp/data.txt |awk 'BEGIN{cur=-1;nb=0;}{if(cur==-1) cur=$1;if(cur!=$1) {printf("%d %d\n",cur,nb);nb=1;cur=$1;}else nb++;}END{printf("%d %d\n",cur,nb);}' > /tmp/sat.txt

join -1 1 -2 1 /tmp/all.txt /tmp/sat.txt > /tmp/partial.txt
join -1 1 -2 1 /tmp/partial.txt /tmp/unsat.txt
