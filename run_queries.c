/*
 * Run Queries
 */
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* only for getTime() */
#include <sys/time.h>
#include <sys/resource.h>

#include <time.h>



#include "interface.h"

#define COUNT 		('C')
#define LOCATE 		('L')
#define EXTRACT 	('E')
#define DISPLAY 	('D')
#define VERBOSE 	('V')

/* macro to detect and to notify errors */
#define IFERROR(error) {{if (error) { fprintf(stderr, "%s\n", error_index(error)); exit(1); }}}
#define error(msg) {printf("\n error, ");printf(msg); printf("\n");exit(1);}


/* local headers */
void do_locate (char *filename_dat,  char *file_name_result_query);
//void do_count ();
void do_count (char *filename_dat, char *file_name_result_query);

//void do_extract (void);
void do_display(ulong length);
void pfile_info (ulong *length, ulong *numpatt, FILE **file_text);
//void pfile_info (ulong *length, ulong *numpatt);
void output_char(uchar c, FILE * where);
double getTime (void);
void usage(char * progname);


static void *Index;	 /* opauque data type */  //modifique tipo de estructura index de void a struc_index
static int Verbose = 0; 
static ulong Index_size, Text_length;
static double Load_time;


/*
 * Temporary usage: run_queries <index file> <type> [length] [V]
 * Argumentos: <File_Name_Dat> <QueryType (C)Count, (L)Locate)> <File_Name_LCP> <File_Name_SA> <Print_Result [V]> <File_Name_Const><File_Name_Result_Query><file-Lote-Consul>
 */
int main (int argc, char *argv[])
{

	int error = 0;
	char *filename_dat, *filename_lcp, *filename_sa, *file_name_const, *file_name_result_query, *size_text, *file_name_lote_consult;
	FILE *f_result_size;
	char querytype;
	
	if (argc < 3)	{
		usage(argv[0]);	
		exit (1);
	}
	
	//Parámetros: <File_Name_Dat> <QueryType (C)Count, (L)Locate)> <File_Name_LCP> <File_Name_SA> <Print_Result (V)Print> <File_Name_Const><File_Name_Result_Query><size_text>
	//Si Print_Result = V - Es obligación pasar los parámetro "File_Name_Const" y "File_Name_Result_Query". En caso contrario no son necesarios.

	filename_dat = argv[1];//Nombre del archivo .DAT
	querytype = *argv[2];// QueryType -- (C)Cout, (L)Locate
	filename_lcp = argv[3]; //Nombre del archivo LCP
	filename_sa = argv[4];//Nombre del archivo SA

	if (argc > 5)
		if (*argv[5] == VERBOSE) {
			Verbose = 1;
			file_name_const = argv[6]; // Nombre del archivo en el cual se imprimen datos sobre la construcción del índice.
			file_name_result_query = argv[7]; // Nombre del archivo en el cual se imprimen los resultados obtenidos.
			size_text = argv[8];//Tamaño del archivo de texto Sólo lo utilizamos para imprimir la salida. Si no hay  que imprimir resultados podemos eliminarlo
			//file_name_lote_consult = argv[9];// Ver mas adelante si conviene pasar el archivo con los patrones a buscar.
		}

	Load_time = getTime ();
	if (Verbose){
		f_result_size = fopen(file_name_const, "a+" );
		if(!f_result_size)
			error("No se abrio el archivo result");
	}
	//error = load_index (filename,filename_lcp,filename_sa, &Index, size_text);
	error = load_index (filename_dat,filename_lcp,filename_sa, &Index, Verbose, f_result_size, size_text);
	IFERROR (error);
	Load_time = getTime () - Load_time;

	if(Verbose)//imprimir resultados de construcción
	{
		fprintf(f_result_size,"%.2f \n", Load_time);
		fclose(f_result_size);
	}



	int salir=1;
	switch (querytype){
		case COUNT:
			if (argc > 3) 
				if (*argv[5] == VERBOSE) {
					//Verbose = 1;
					fprintf(stdout,"%c", COUNT);
				}

             //fprintf (stdout, "LongPat\tCantString\tNroOcc\tNroMediOccf\tTiempoCadaPatron\tiempo_total\n");
			//Agregamo un parametro al do_count que originalmente no tiene para poder pasar el nombre del archivo que contiene el texto
			while(salir==1)
			{
				if (Verbose)
					do_count(filename_dat, file_name_result_query); //filename_dat es el nombre del archivo que contiene el texto
																	//file_name_result_query es el nombre del archivo donde se escriben los resultados
				//do_count();
				//####	Sólo para Pruebas - Realizar varios Count
					fprintf(stderr,"Ingrese 1 para Realizar otro count, Ingrese cualquier otro nro para terminar \n");
					scanf("%d",&salir);
				//####	FIN  --- Sólo para Pruebas - Realizar varios Count
			}
			break;
		case LOCATE:
				if (argc > 3)
					if (*argv[5] == VERBOSE) {
						//Verbose = 1;
						fprintf(stdout,"%c", LOCATE);
					}
					while(salir==1)
					{
						do_locate(filename_dat, file_name_result_query); //filename_dat es el nombre del archivo que contiene el texto
																		 //file_name_result_query es el nombre del archivo donde se escriben los resultados
						//####	Sólo para Pruebas - Realizar varios Locate
						fprintf(stderr,"Ingrese 1 para Realizar otro count, Ingrese cualquier otro nro para terminar \n");
						scanf("%d",&salir);
						//####	FIN  --- Sólo para Pruebas - Realizar varios Locate
					}

				break;
		case EXTRACT:
			if (argc > 3) 
				if (*argv[3] == VERBOSE) {
						Verbose = 1;
						fprintf(stdout,"%c", EXTRACT);
				}

			//do_extract(); //Comentamos del original
			break;
		case DISPLAY:
			if (argc < 4) {
				usage(argv[0]);	
				exit (1);
			}
			if (argc > 4) 
				if (*argv[4] == VERBOSE){
						Verbose = 1;
						fprintf(stdout,"%c", DISPLAY);

				}
			//do_display((ulong) atol(argv[3]));
			break;
		default:
			fprintf (stderr, "Unknow option: main ru\n");
			exit (1);
	}

	error = free_index(Index);
	IFERROR(error);

	return 0;
}


void do_count (char *filename_dat, char *file_name_result_query)//
//void do_count ()
{

	double fc_total_time;

	int error = 0;
	ulong numocc, length, tot_numocc = 0, numpatt, res_patt, pagefaults;
	unsigned long pagefaults_full=0; // --------pagefaults, pagefaults_full utilizada para guardar la cantidad de paginas de disco consultadas
	double time, tot_time = 0;

// >>>>>>>>>>>>>>> vbles para medir tiempo de ejecucion de findclose y count.
	clock_t t_ini_count, t_fin_count;
	double tot_time_count=0, parametro_time = 0;
// <<<<<<<<<<<<<<< vbles para medir tiempo de ejecucion de findclose y count.

	uchar *pattern;

	struct rusage usage;  //--------------Modificaciones para saber si va a disco cuando realiza el count-------//



//######## Parte sólo para obtener resultados##############//
//####Abre el archivo que contiene los patrones a buscar.

	int control=1;//Sólo para obtener mediciones borrar al final
	char tipo[10];
	char tamanio_mb[10];
	char long_string[10];
	char nro_string_buscar[10];
	FILE *f_text_patron_buscar;

	//########################################################################################
	//Abrimos el archivo que contiene los patrones que vamos a buscar en el índice para obtener los resultados
	//#########################################################################################
		while (control==1)
		{
			fprintf(stderr, "Ingrese tipo -- <dna>, <pro>, <source> \n");
			fscanf(stdin, "%s",tipo);
			fprintf(stderr,"\n");
			fprintf(stderr, "Ingrese el tamaño Archivo en MB \n");
			fscanf(stdin, "%s",tamanio_mb);
			fprintf(stderr,"\n");
			fprintf(stderr,"Ingrese Longitud de strig a buscar \n");
			fscanf(stdin, "%s",long_string);
			fprintf(stderr,"\n");
			fprintf(stderr,"Ingrese Numero de string a buscar \n");
			fscanf(stdin, "%s",nro_string_buscar);

			//------------Texto---------------------//
				char lote_texto_buscar[512];

				//strcpy(lote_texto_buscar,"lotes/Patt_L");
				strcpy(lote_texto_buscar,"/home/jesica/Documentos/entrada/");
				strcat(lote_texto_buscar,tipo);
				strcat(lote_texto_buscar,tamanio_mb);
				strcat(lote_texto_buscar,"mb/Patt_L");

				strcat( lote_texto_buscar, tamanio_mb ); // El valor 10 corresponde a la cantidad de MB que contiene el archivo de texto ----- Cambiar para archivo de otro tamaño
				strcat( lote_texto_buscar, "MG_" );
				strcat( lote_texto_buscar, long_string );
				strcat( lote_texto_buscar, "_" );
				strcat( lote_texto_buscar, nro_string_buscar);
				f_text_patron_buscar = fopen( lote_texto_buscar, "r" );

				if(!f_text_patron_buscar)
					control=1;
				else
				{
					//fseek( f_text_patron_buscar, 0, SEEK_END );
					control=0;
				}
		}

		//########################################################################################
		//FIN  Abrir el archivo que contiene los patrones que vamos a buscar en el índice para obtener los resultados
		//#########################################################################################


	//pfile_info (&length, &numpatt);
    pfile_info (&length, &numpatt, f_text_patron_buscar);
	res_patt = numpatt;

	pattern = (uchar *) malloc (sizeof (uchar) * (length+1));//Agregamos un lugar cuando pedimos memoria para colocar \0 al final del string para poder utiliza la función de comparación
	if (pattern == NULL)
	{
		fprintf (stderr, "Error: cannot allocate\n");
		exit (1);
	}


	pattern[length]='\0'; //Le damos a la última posición del arreglo el caracter de fin de string (Agregado)

//------------Apertura del archivo que contiene el Texto---------------------//
	FILE *f_text;
	f_text = fopen( filename_dat, "r" );

	if(!f_text)
		//printf ("No se abrio el archivo de texto para count");
		error("No se abrio el archivo de texto para count");
	//fseek( f_text, 0, SEEK_END );
//------------FIN Apertura del archivo que contiene el Texto---------------------*/




	while (res_patt)
	{

		//if (fread (pattern, sizeof (*pattern), length, stdin) != length) //Esta parte debe ir cuando no se saquen mas resultados
		if (fread (pattern, sizeof (*pattern), length, f_text_patron_buscar) != length)
		{
			fprintf (stderr, "Error: cannot read patterns file\n");
			perror ("run_queries");
			exit (1);
		}


		// Count //
		time = getTime ();

		//--------------Modificaciones para saber si va a disco cuando realiza el count-------//
			pagefaults=0;
			getrusage (RUSAGE_SELF, &usage);
			pagefaults=usage.ru_majflt;
		//--------------Fin Modificaciones para saber si va a disco cuando realiza el count-------//


		//error = count (Index, pattern, length, &numocc);

	//#################################################################
	//##############COUNT##############################################

		t_ini_count = clock();
		error = count(Index, &parametro_time, pattern, length, &numocc, f_text);
		t_fin_count = clock();
		tot_time_count += (double)(t_fin_count - t_ini_count) / CLOCKS_PER_SEC;

		IFERROR (error);
		//cout << "total find_close = %.4f" << (fc_total_time*1000);

	
		//if (Verbose) {
			//fwrite(&length, sizeof(length), 1, stdout);
			//fwrite(pattern, sizeof(*pattern), length, stdout);
			//fwrite(&numocc, sizeof(numocc), 1, stdout);
			//}
//>>>>>>>>>>>>>>>>>>>>> para imprimir tiempos de ejecucion
//		printf("find_close total time: %.4f\n", (parametro_time*1000.0));
//		printf("count total time: %.4f\n", (tot_time_count*1000.0));
		printf("find_close total time: %.16g milisegundos\n", (parametro_time * 1000.0));
		printf("count total time: %.16g milisegundos\n", tot_time_count * 1000.0);
		tot_time += (getTime () - time);
		printf("Old count total time: %.16g\n", (tot_time*1000.0));
		printf("-------------------------------------------------------\n");
//<<<<<<<<<<<<<<<<<<<<<< para imprimir tiempos de ejecucion

		tot_numocc += numocc;
		res_patt--;

		//--------------Modificaciones para saber si va a disco cuando realiza el count-------//
			getrusage (RUSAGE_SELF, &usage);
			pagefaults_full=pagefaults_full+ (usage.ru_majflt - pagefaults);
		//--------------Fin Modificaciones para saber si va a disco cuando realiza el count-------//

	}
	//--------------Modificaciones para saber si va a disco cuando realiza el count-------//
		fprintf (stderr, "Total Num fallo de página = %lu\n", pagefaults_full);
	//--------------Fin Modificaciones para saber si va a disco cuando realiza el count-------//

		int long_patron=length, cant_string=numpatt;//Valores para imprimir en la salida
		float nro_medio_ocurrencia;
		nro_medio_ocurrencia = (tot_numocc * 1.0) / (cant_string * 1.0);


	 //------------##########################################---------------------//
	//------------Apertura del archivo que contiene el Texto---------------------//

		FILE *f_count_result;
		f_count_result = fopen(file_name_result_query, "a+" );

		if(!f_count_result)
			error("No se abrio el archivo de resultados para COUNT");

	//------------FIN Apertura del archivo que contiene el Texto---------------------//
   //------------##############################################---------------------//


		printf("count total time: %.4f\n", (tot_time*100));
		fprintf(f_count_result,"%d\t%d\t%lu\t%f\t%.4f\t%.4f\n",long_patron, cant_string,tot_numocc,nro_medio_ocurrencia,(tot_time * 1000) / (numpatt),tot_time*1000);
		//fprintf (stderr, "%d\t%d\t%lu\t%f\t%.4f\t%.4f\n", long_patron, cant_string,tot_numocc,nro_medio_ocurrencia,(tot_time * 1000) / (numpatt),tot_time*1000);


	/*fprintf (stderr, "Total Num occs found = %lu\n", tot_numocc);
	fprintf (stderr, "Count time = %.4f msecs\n", tot_time*1000);
	fprintf (stderr, "Count_time/Pattern_chars = %.4f msecs/chars\n",
		 (tot_time * 1000) / (length * numpatt));
	fprintf (stderr, "Count_time/Num_patterns = %.4f msecs/patterns\n\n",
		 (tot_time * 1000) / numpatt);
	fprintf (stderrcount, "(Load_time+Count_time)/Pattern_chars = %.4f msecs/chars\n",
		 ((Load_time+tot_time) * 1000) / (length * numpatt));
	fprintf (stderr, "(Load_time+Count_time)/Num_patterns = %.4f msecs/patterns\n\n",
		 ((Load_time+tot_time) * 1000) / numpatt);	(((((Va comentado*/

	free (pattern);
	fclose(f_text);
	fclose(f_count_result);

}


//void do_locate (char *filename)
void do_locate (char *filename_dat, char *file_name_result_query)
{
	int error = 0;
	ulong numocc, length, *occ=NULL, tot_numocc = 0, numpatt; //Realizamos la modificación de asignar NULL a *occ ya que en la función locate pedimos memoria con la función ralloc y si no tiene asignado NULL falla
	double time, tot_time = 0;
	uchar *pattern;

	///######## Parte sólo para obtener resultados##############//
		//####Abre el archivo que contiene los patrones a buscar.

			int control=1;//Sólo para obtener mediciones borrar al final
			char tipo[10];
			char tamanio_mb[10];
			char long_string[10];
			char nro_string_buscar[10];
			FILE *f_text_patron_buscar;

			//########################################################################################
			//Abrimos el archivo que contiene los patrones que vamos a buscar en el índice para obtener los resultados
			//#########################################################################################
			while (control==1)
			{
				fprintf(stderr, "Ingrese tipo -- <dna>, <pro>, <source> \n");
				fscanf(stdin, "%s",tipo);
				fprintf(stderr,"\n");
				fprintf(stderr, "Ingrese el tamaño Archivo en MB \n");
				fscanf(stdin, "%s",tamanio_mb);
				fprintf(stderr,"\n");
				fprintf(stderr,"Ingrese Longitud de strig a buscar \n");
				fscanf(stdin, "%s",long_string);
				fprintf(stderr,"\n");
				fprintf(stderr,"Ingrese Numero de string a buscar \n");
				fscanf(stdin, "%s",nro_string_buscar);

				//------------Texto---------------------//
					char lote_texto_buscar[512];

					//strcpy(lote_texto_buscar,"lotes/Patt_L");
					strcpy(lote_texto_buscar,"/home/dario/tesis_area_trabajo/datos-de-entrada/");
					strcat(lote_texto_buscar,tipo);
					strcat(lote_texto_buscar,tamanio_mb);
					strcat(lote_texto_buscar,"mb/Patt_L");

					strcat( lote_texto_buscar, tamanio_mb ); // El valor 10 corresponde a la cantidad de MB que contiene el archivo de texto ----- Cambiar para archivo de otro tamaño
					strcat( lote_texto_buscar, "MG_" );
					strcat( lote_texto_buscar, long_string );
					strcat( lote_texto_buscar, "_" );
					strcat( lote_texto_buscar, nro_string_buscar);
					f_text_patron_buscar = fopen( lote_texto_buscar, "r" );

					if(!f_text_patron_buscar)
						control=1;
					else
					{
						//fseek( f_text_patron_buscar, 0, SEEK_END );
						control=0;
					}
			}

			//########################################################################################
			//FIN  Abrir el archivo que contiene los patrones que vamos a buscar en el índice para obtener los resultados
			//#########################################################################################

//pfile_info (&length, &numpatt);
	pfile_info (&length, &numpatt, f_text_patron_buscar);
	pattern = (uchar *) malloc (sizeof (uchar) * (length+1));
	if (pattern == NULL)
	{
		fprintf (stderr, "Error: cannot allocate\n");
		exit (1);
	}
	pattern[length]='\0'; //Le damos a la última posición del arreglo el caracter de fin de string (Agregado)

//------------Apertura del archivo que contiene el Texto---------------------//
	FILE *f_text;
	f_text = fopen( filename_dat, "r" );

	if(!f_text)
		//printf ("No se abrio el archivo de texto para count");
		error("No se abrio el archivo de texto para count");
	//fseek( f_text, 0, SEEK_END );
//------------FIN Apertura del archivo que contiene el Texto---------------------*/


//------------##########################################---------------------//
//------------Apertura del archivo Resultado para LOCATE---------------------//

	FILE *f_salida_locate;
	f_salida_locate = fopen(file_name_result_query, "a+" );

	if(!f_salida_locate)
		error("No se abrio el archivo de resultados para LOCATE");
	fseek( f_salida_locate, 0, SEEK_END );
//------------FIN Apertura del archivo que contiene el Texto---------------------//
   //------------##############################################---------------------//




	while (numpatt)
	{
		//if (fread (pattern, sizeof (*pattern), length, stdin) != length)
		if (fread (pattern, sizeof (*pattern), length, f_text_patron_buscar) != length)
		{
			fprintf (stderr, "Error: cannot read patterns file\n");
			perror ("run_queries");
			exit (1);
		}

		/* Locate */
		time = getTime ();

		//error =	locate (Index, pattern, length, &occ, &numocc);
		error =	locate (Index, pattern, length, &occ, &numocc, f_text);
		IFERROR (error);
		tot_time += (getTime () - time);
		
		tot_numocc += numocc;
		numpatt--;
		
		/*if (Verbose) {
			fwrite(&length, sizeof(length), 1, stdout);
			fwrite(pattern, sizeof(*pattern), length, stdout);
			fwrite(&numocc, sizeof(numocc), 1, stdout);
			fwrite(occ, sizeof(*occ), numocc, stdout);
			}*/
			
		//if (numocc) free (occ); Modifico esta liberación ya que en mi caso tengo el arreglo SA El cual tiene todas las posiciones del archivo. occ va apuntar a la posición en el arreglo SA que contenga la posición de la primer palabra a devolver y con numocc sabes cuanto imprimir de este arreglo
	}
		if (Verbose)
			fprintf (f_salida_locate, "%s\t%u\t%.5f\t%.5f\t%.5f\n", long_string,tot_numocc, tot_time, (tot_time * 1000) / tot_numocc, ((tot_time+Load_time) * 1000) / tot_numocc);

		//fprintf (stderr, "Total Num occs found = %lu\n", tot_numocc);
		//fprintf (stderr, "Locate time = %.2f secs\n", tot_time);
		//fprintf (stderr, "Locate_time/Num_occs = %.4f msec/occss\n\n", (tot_time * 1000) / tot_numocc);
		//fprintf (stderr, "(Load_time+Locate_time)/Num_occs = %.4f msecs/occs\n\n", ((tot_time+Load_time) * 1000) / tot_numocc);

		free (pattern);
		fclose(f_text);
		fclose(f_text_patron_buscar);
		fclose(f_salida_locate);
	}




/* Open patterns file and read header */
void pfile_info (ulong * length, ulong * numpatt, FILE **f_text_buscar)//FILE *f_text_buscar no es un parámetro normal de pfile_info solo es para obtener resultados cuando termine hay que sacarlo.
//void pfile_info (ulong * length, ulong * numpatt)
{
	int error;
	uchar c;
	uchar origfilename[257];
	*numpatt=0; *length=0;

	//######## Parte sólo para obtener resultados##############//

	error = fscanf (f_text_buscar, "# number=%u length=%u file=%s forbidden=", numpatt, length, origfilename);
	//error = fscanf (stdin, "# number=%lu length=%lu file=%s forbidden=", numpatt, length, origfilename);
	//######## FIN Parte sólo para obtener resultados##############//


	if (error != 3)
	{
		fprintf (stderr, "Error: Patterns file header not correct\n");
		perror ("run_queries");
		exit (1);
	}

	fprintf (stderr, "# patterns = %u length = %u file = %s forbidden chars = ", *numpatt, *length, origfilename);

	//while ( (c = fgetc(stdin)) != 0) {
	while ( (c = fgetc(f_text_buscar)) != 0) {
		if (c == '\n') break;
		fprintf (stderr, "%d",c);
	}

	fprintf(stderr, "\n");

}

double getTime (void)
{

	double usertime, systime;
	struct rusage usage;

	getrusage (RUSAGE_SELF, &usage);

	usertime = (double) usage.ru_utime.tv_sec +
		(double) usage.ru_utime.tv_usec / 1000000.0;

	systime = (double) usage.ru_stime.tv_sec +
		(double) usage.ru_stime.tv_usec / 1000000.0;

	return (usertime + systime);

}

void usage(char * progname) {	
	fprintf(stderr, "\nThe program loads <index> and then executes over it the\n");
	fprintf(stderr, "queries it receives from the standard input. The standard\n");
	fprintf(stderr, "input comes in the format of the files written by \n");
	fprintf(stderr, "genpatterns or genintervals.\n");
	fprintf(stderr, "%s reports on the standard error time statistics\n", progname);
	fprintf(stderr, "regarding to running the queries.\n\n");
	fprintf(stderr, "Usage:  %s <index> <type> [length] [V]\n", progname);
	fprintf(stderr, "\n\t<type>   denotes the type of queries:\n");
	fprintf(stderr, "\t         %c counting queries;\n", COUNT);
	fprintf(stderr, "\t         %c locating queries;\n", LOCATE);
	fprintf(stderr, "\t         %c displaying queries;\n", DISPLAY);
	fprintf(stderr, "\t         %c extracting queries.\n\n", EXTRACT);
	fprintf(stderr, "\n\t[length] must be provided in case of displaying queries (D)\n");
	fprintf(stderr, "\t         and denotes the number of characters to display\n");
	fprintf(stderr, "\t         before and after each pattern occurrence.\n");
	fprintf(stderr, "\n\t[V]      with this options it reports on the standard output\n");
	fprintf(stderr, "\t         the results of the queries. The results file should be\n");
	fprintf(stderr, "\t         compared with trusted one by compare program.\n\n");
}



