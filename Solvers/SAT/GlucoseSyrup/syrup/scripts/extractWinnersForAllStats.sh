#!/bin/csh


./extractPositionOfWinnerForOneStat.sh 'Conflicts' 'none' > stats/conflicts.txt
./extractPositionOfWinnerForOneStat.sh 'Decisions' 'Conflicts' > stats/decisionswrtconflicts.txt
./extractPositionOfWinnerForOneStat.sh 'Propagatations' 'Décisions' > stats/prowrtdecisions.txt
./extractPositionOfWinnerForOneStat.sh 'Unaries' 'none' > stats/unaries.txt
./extractPositionOfWinnerForOneStat.sh 'Glues' 'none' > stats/glues.txt
tar -cf ~/stats.tgz stats

