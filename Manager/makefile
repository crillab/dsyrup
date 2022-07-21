# compilateur :
COMPILATEUR = /usr/local/bin/mpich-3.2/bin/mpic++

# COPTIONS = -O3 -Wall -Wextra
COPTIONS = -O3 -Wall -Wextra -lpthread -std=c++11 -I/usr/include/libxml2 `xml2-config --cflags`
LOPTIONS = -lz -I/usr/include/libxml2 `xml2-config --libs` -Wno-literal-suffix -Wno-write-strings -lpthread -std=c++11 -Wno-misleading-indentation -Wno-unused-parameter

UNAME_S := $(shell uname -s)

EXECUTABLE = manager
# -------------------------------------------------------------------
#  Emplacement des sources et des objets, des includes et de l'executable
# -------------------------------------------------------------------

# path:
CODE = $(shell pwd)

# folder for the executable 
EXEDIR = $(CODE)/bin

# adress of the source code, objects and include files:
SRCDIR = $(CODE)/Main
OBJDIR = $(CODE)/obj

# create the list of file sources:
SRC = $(wildcard $(CODE)/*/*.cc)

# list the files:
NOM = $(basename $(notdir $(SRC)))
OBJ = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(NOM)))



normal:
	$(MAKE) install COMPILATEUR=mpic++
cluster: 
	$(MAKE) install COMPILATEUR=/usr/local/bin/mpich-3.2/bin/mpic++

debugNormal:
	$(MAKE) install COMPILATEUR=mpic++ COPTIONS+=-g

debugCluster:
	$(MAKE) install COMPILATEUR=/usr/local/bin/mpich-3.2/bin/mpic++ COPTIONS+=-g

# link edition
install: faire_dossier $(OBJ)
	$(COMPILATEUR) -o $(EXEDIR)/$(EXECUTABLE) $(OBJ) $(LOPTIONS)



# rules to do the folders
faire_dossier: makedir
	@echo "folders created"
	@echo COPTIONS

# compilation rules
$(OBJDIR)/%.o: $(CODE)/*/%.cc $(CODE)/*/%.h
	$(COMPILATEUR) -c $(COPTIONS) -o $@ $< $(LOPTIONS)


# -------------------------------------------------------------------
#  to create the objects folder
# -------------------------------------------------------------------

# creation of the folder $(OBJDIR) if it is necessary:
makedir: 
	mkdir -p $(OBJDIR)
	mkdir -p $(EXEDIR)

# -------------------------------------------------------------------
#  cleaning rules
# -------------------------------------------------------------------

.PHONY: clean purge

clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(EXEDIR)
	@rm -rf *~
help:
	@echo "--------------------------------------------------------------------------"
	@echo " Disponible rules"
	@echo
	@echo " install : compilation and executable creation"
	@echo " clean   : remove the object files"
	@echo " purge   : remove the object files and the executable"
	@echo " help    : print this comments (defautl)"
	@echo "--------------------------------------------------------------------------"
	@echo
