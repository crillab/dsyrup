.PHONY: clean

normal:
	cd Test && $(MAKE) normal -j
	cd Manager && $(MAKE) normal -j
	cd Solvers/SAT/GlucoseSyrup/ && $(MAKE) normal -j

cluster:
	cd Test && $(MAKE) normal -j
	cd Manager && $(MAKE) cluster -j
	cd Solvers/SAT/GlucoseSyrup/ && $(MAKE) cluster -j

debugCluster:
	cd Test && $(MAKE) normal -j
	cd Manager && $(MAKE) debugCluster -j
	cd Solvers/SAT/GlucoseSyrup/ && $(MAKE) debugCluster -j

debugNormal:
	cd Test && $(MAKE) normal -j
	cd Manager && $(MAKE) debugNormal -j
	cd Solvers/SAT/GlucoseSyrup/ && $(MAKE) debugNormal -j

clean:
	find . -name '*.*~'
	find . -name '*.*~' -delete
	find . -name '*.*#'
	find . -name '*.*#' -delete
	find . -name '.#*'	
	find . -name '.#*' -delete
	rm -rf *~
	rm -rf CMakeLists.txt
	cd Manager && $(MAKE) clean -j
	cd Solvers/SAT/GlucoseSyrup/ && $(MAKE) clean -j
	cd Test && $(MAKE) clean -j
