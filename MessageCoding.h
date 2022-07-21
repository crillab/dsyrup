

#define NOT_FINISHED_STATE                          0
#define SOLUTION_FOUND_STATE                        1
#define SOLUTION_FOUND_BY_ANOTHER_STATE             2
#define END_INIT_STATE                              3
#define END_SUBPROBLEM_STATE                        4


#define TAG_MSG                     222
#define TAG_DISPLAY                 888
#define TAG_UL                      777
#define TAG_CLAUSE                  999
#define TAG_KILL                    666
#define TAG_PHARUS                  555
#define TAG_MANAGER                 111
#define TAG_SOLUTION                333

#define TAG_THREADS                1000   

#define STOP_AND_KILL                -1
#define STOP_SEARCH                1666

#define SOLUTION_FOUND                1
#define SOLUTION_SAT                  2
#define SOLUTION_UNS                  3
#define SOLUTION_IND                  4

#define SEND_RESULT                   5
#define SEARCH_MORE                   6
#define AMPHAROS_BARRIER              8
#define STOP_COM_ULS                  9
#define STOP_COM_CLAUSES             10

#define PHARUS_INITIALIZATION                      11
#define PHARUS_TRANSMISSION_FROM_ROOT              12
#define PHARUS_TRANSMISSION_FROM_SUBPROBLEM        13
#define PHARUS_TRANSMISSION_CHILDREN               14
#define PHARUS_TRANSMISSION_DELETE_FATHER          15
#define PHARUS_TRANSMISSION_GO_RIGHT               16
#define PHARUS_TRANSMISSION_GO_LEFT                17
#define PHARUS_TRANSMISSION_ALREADY_ASSIGNED       18
#define PHARUS_SUBPROBLEM_UNSAT                    19
#define PHARUS_SUBPROBLEM_INDETERMINATE            20

#define PHARUS_SUBPROBLEM_EXIST                    27
#define PHARUS_SUBPROBLEM_EXIST_LEAF               28
#define PHARUS_SUBPROBLEM_NOT_EXIST                29

#define PHARUS_EXTEND                              23
#define PHARUS_NO_EXTEND                           24
#define PHARUS_ULS_UNSAT                           25
#define PHARUS_NO_ULS_UNSAT                        26

#define END_SUBPROBLEM               -2
#define UNSAT_SUBPROBLEM             -3
#define SAME_SUBPROBLEM              -1



#define PRINT_SOLUTION             1000
#define JOB_FINISHED               1001

#define CONCU_MODE                10001
#define EPS_MODE                  10002
#define AMPHAROS_MODE             10003

#define MORE_GENERATE_CUBE        20001
#define STOP_GENERATE_CUBE        20002

#define ASK_CUBE                  30001

#define SIZE_CLAUSE               400

#define VAR_POSITIVE_ASSIGN           0
#define VAR_NEGATIVE_ASSIGN           1
#define VAR_NO_ASSIGN                 2

#define DECISION_PROPAGATES_CONFLICT -1
#define DECISION_PROPAGATES_SAT       0

#define NOT_SENT                     -1 //To kwon if a UL (under assumption is sent or not)


