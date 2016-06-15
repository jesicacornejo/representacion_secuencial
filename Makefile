all: Run_queries.o 
	gcc -g -o EXE  run_queries.o create_index.o auxiliary.o count_locate.o bmestatico.o -lm	
	rm *.o

Run_queries.o: create_index.o auxiliary.o count_locate.o bmestatico.o interface.o
	gcc -g -c run_queries.c -o run_queries.o

create_index.o:    
	gcc -g -c create_index.c -o create_index.o
auxiliar.o:
	gcc -g -c auxiliary.c -o auxiliary.o

count_locate.o:
	gcc -g -c count_locate.c -o count_locate.o

bmestatico.o:
	gcc -g -c BMestatico.c -o bmestatico.o
	
interface.o:
	gcc -g -c interface.h -o interface.o	
	
	
	
	