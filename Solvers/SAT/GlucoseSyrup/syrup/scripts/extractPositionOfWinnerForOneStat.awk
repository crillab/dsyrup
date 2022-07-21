# awk -f extractPositionOfWinnerForOneStat.awk -F'|' -v stat='Conflicts' -v div='none' -v winner=1
# Gives the position of the winner (thread 1) wrt conflict 
# order is < (best is the smallest one)

# awk -f extractPositionOfWinnerForOneStat.awk -F'|' -v stat='Decisions' -v div='Conflicts' -v winner=1
# Gives the position of the winner wrt Decisions/Conflicts 
# order is < (best is the smallest one)
# 
BEGIN{min=-1;max=-1;}
{

    if($2 ~ stat) {
	for(i=4;i<NF;i++)  {
	    arrayStats[i-4] = $i;
	}
	nbthreads = NF-4;
    }
    if($2  ~ div) {
	for(i=4;i<NF;i++) { 
	    arrayDiv[i-4] = $i;
	}
    }
}
END{
    if(div == "none") {
	for(i=0;i<nbthreads;i++) 
	    arrayDiv[i] = 1;
    }



    pos = 1;
    min =  arrayStats[0]/arrayDiv[0] ;
    max =  arrayStats[0]/arrayDiv[0] ;

    for(i = 0;i<nbthreads;i++) {
	if(i!=winner && arrayStats[winner]/arrayDiv[winner] > arrayStats[i]/arrayDiv[i]) 
	    pos++;
	if(min> arrayStats[i]/arrayDiv[i]) 
	    min =  arrayStats[i]/arrayDiv[i];
	if(max< arrayStats[i]/arrayDiv[i]) 
	    max =  arrayStats[i]/arrayDiv[i];

	
    }
    if( max==0 || 1-min/max < 0.04) 
	pos = nbthreads+1;
    printf("%d\n",pos);

}
