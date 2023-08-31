input := $(if $(input),$(input),default_input)
name := $(word 1,$(subst ., ,$(input)))
all:
	@sudo ./pythoncompiler $(input)
	@sudo g++ -m64 -no-pie -Wl,-rpath,. $(name).s $(name).cc -o $(name).out -L. -lruntime -Wl,--wrap=getchar

clean:
	@rm -f *.out *.s *.tig *.cc 