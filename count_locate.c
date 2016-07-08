#include "common.h"
#include <stdio.h>
#include <time.h>

/* only for getTime() */
#include <sys/time.h>
#include <sys/resource.h>


extern byte RankTable[255];
extern char MinExcessTable[255];
extern char *MinExcessBitmap;
extern byte NumOfLeaves[255];
extern char *MinExcessBitmap_RS;

extern ulong *bitmap_ulong;
extern ulong *pos_term_raiz; //Estructura que mantiene las posiciones de finalización de cada hijo del nodo raiz
extern ulong b; //marca el nro de bit tomados a la ves 32 en este caso
extern ulong s; //marca el nro de ulong que se toman de la estructura Rb (En nuestro caso MinExcessBitmap) en este caso es 4

// a/8 y a%8
#define DIV(a) ((a)>> 3)
#define MOD(a) ((a) & 7)

// a/32 y a%32
#define DIVb(a) ((a)>> 5)
#define MODb(a) ((a) & 31)

// a/s y a%s
#define DIVs(a) ((a)>> 7)//Aquí asumimos que s=4 y b=32 por lo que s*b=128 --> 2^7=128.
#define MODs(a) ((a) & ((s*b)-1))

ulong cursor_pos_term_raiz=0;

double getTime_count (void)
{

    double cpu_time, sistime;
    struct rusage usage_count;

    getrusage (RUSAGE_SELF, &usage_count);

    cpu_time = (double) usage_count.ru_utime.tv_sec +
        (double) usage_count.ru_utime.tv_usec / 1000000.0;

    sistime = (double) usage_count.ru_stime.tv_sec +
        (double) usage_count.ru_stime.tv_usec / 1000000.0;

    return (cpu_time + sistime);

}

ulong  FindClose( ulong *bitmap_ulong, ulong pos, ulong last, ulong *nroNodo, ulong *nroHoja)
{
		ulong *A;

		A=bitmap_ulong;
		int E, is_one=0,bandera, break_byte=1, break_ulong=1, cont_s;
 	    ulong *aux_ulong, *aux_ulong2,pos_aux, exce_maximo=0;
 	    byte *aux_byte, *aux_byte2;
 	    int pos_bits, of_bits=0;
 	 	aux_ulong=(ulong *) malloc(sizeof(ulong)*1);
 	 	aux_ulong2=(ulong *) malloc(sizeof(ulong)*1);

 	 	*aux_ulong = A[DIVb(pos)];
 	 	aux_byte=(byte *)aux_ulong;

 	 	pos_bits=MODb(pos);

 	 	E=-1; pos++; pos_bits++;

		 if ((MODs(pos))) { //Si esta en posición modulo s*b = 0  directamente procesamos de a s*b bit

			if ((MODb(pos))) { //Si esta en posición modulo 32 = 0  directamente procesamos de a ulong


				 //********************   Procesa de a bits *******************//
				// procesamos los primeros bits hasta llegar al inicio de un byte

				 if (MOD(pos_bits))
				 {of_bits=1;
					 while ( MOD(pos_bits) && E!=0)
					 {     if (aux_byte[3-DIV(pos_bits)] & (128>> MOD(pos_bits++)) )
						   { E--; is_one=1; (*nroNodo)++;}
						   else
						   { E++;
							 if(is_one){(*nroHoja)++;is_one=0;}
						   }
						 pos++;
					 }
				 }
				 else{ //else MOD --Verificamos si el cero correspondiente al uno se encuentra el el inicio del byte. Estariamos en presencia de un hoja que tiene su 1 en un byte y su cero el inicio del proximo byte.
					 if (!(aux_byte[3-DIV(pos_bits)] & (128>> MOD(pos_bits)) ))
					 { E++; pos++; pos_bits++;}
				 }
				/*if ((!(MOD(pos))) && pos < last)
					if ( ((A[DIV(pos)-1]) & (128>> 7)) && (A[DIV(pos)] < 128) ) (*nroHoja)++;//Realizamos esta consulta para saber si el nro anterior termina en 1 y el siguiente empieza en 0, si este es el caso estamos en presencia de una hoja.
				*/

			}

			else{//else MODb -- Sólo ingresa cuando búscamos el cero para el uno que esta al final de un ulong y verificamos si el siguiente ulong comienza con 0, podemos ver que es una hoja cuyo uno esta al final de un ulong y su cero esta al inicio del siguiente ulong.
				*aux_ulong = A[DIVb(pos)];
				aux_byte=(byte *)aux_ulong;
				pos_bits=MODb(pos);
				if (!(aux_byte[3-DIV(pos_bits)] & (128>> MOD(pos_bits)) ))
				{ E++; pos++;pos_bits++;}

			}
		 }
		 else //else MODs  --
		 {  if (!(A[DIVb(pos)]&(1<<b-1)))
				{ E++; pos++;pos_bits++;}
		 }



        //********************   Procesa de a bytes *******************//

	 if ((MODs(pos))){  //Si esta en posición modulo s*b = 0  directamente procesamos de a s*b bit


        if ((MODb(pos))) {
			// se procesa de a bytes
        	// bandera=0;  Descomenar en caso de realizar el proceso de hojas llamando a pop_leaves
			while (pos+8 < last && E!=0 && MODb(pos))  //RECORDAR QUE PUSIMOS <= last DEBEMOS PROBAR SI FUNCIONA
			{
				  if (of_bits){//Si pos_bit dividido 8 da cero (0). Estamos en el situación donde pos_bit queda sobre el bit cero de nuestra estructura y por lo tanto no se ha hecho un procesamiento de a bits.
					  if ( ((aux_byte[3-DIV(pos_bits)+1]) & (128>> 7)) && (aux_byte[3-DIV(pos_bits)] < 128) ) (*nroHoja)++;//Realizamos esta consulta para saber si el nro anterior termina en 1 y el siguiente empieza en 0, si este es el caso estamos en presencia de una hoja.
				  }
				  else
					  of_bits=1;
				  if ( MinExcessTable[ aux_byte[3-DIV(pos_bits)] ] <= E){
					  break_byte=0;
					  break;
				  }
				  else  { E -= 2 * RankTable[ aux_byte[3-DIV(pos_bits)] ] - 8 ; //exceso total
				  //      E -= 2 * popcount8(aux_byte[3-DIV(pos_bits)]) - 8 ; //exceso total
						  (*nroNodo)+= RankTable [ aux_byte[3-DIV(pos_bits)]];
						//(*nroNodo)+=popcount8(aux_byte[3-DIV(pos_bits)]);
						  (*nroHoja)+= NumOfLeaves[ aux_byte[3-DIV(pos_bits)]];
						//(*nroHoja)+=popLeaves8(aux_byte[3-DIV(pos_bits)], &bandera);
						}
				  pos+=8;
				  pos_bits+=8;
			  //if ( ((A[DIV(pos)-1]) & (128>> 7)) && (A[DIV(pos)] < 128) ) (*nroHoja)++;
			}
        }
	 }


		if (break_byte){
			if((aux_byte[0] & 128>> 7) && of_bits)
				bandera=1;
			else
				bandera=0;


			//********************   Procesa de a ulong *******************//
			while (pos+b < last && E!=0 && MODs(pos))
				// se procesa de a ulong
			{     if ( MinExcessBitmap[ DIVb(pos) ] <= E){
						break_ulong=0;
						break;
					}
				  else  { E -= 2 * popcount( A[DIVb(pos)] ) - b ; //exceso total b=32 en este caso
						  (*nroNodo)+= popcount ( A[DIVb(pos)]);
						  (*nroHoja)+= popLeaves( A[DIVb(pos)], &bandera);
						}
				  pos+=b;

			}


			if (break_ulong){
			//********************   Procesa de a (s*b) bit *******************//
				while (pos+(b*s) < last && E!=0)
				{
					//if(pos>=14011554 && pos<=14011810)
						//cont_s=0;

					  cont_s=0;
					  exce_maximo=0;
					  if ( MinExcessBitmap_RS[ DIVs(pos) ] <= E)
							break;

					  else
					  {
						  pos_aux=DIVb(pos);
						  while(cont_s<s){
							  exce_maximo+= popcount( A[pos_aux] );
							  (*nroNodo)+= popcount ( A[pos_aux]);
							  (*nroHoja)+= popLeaves( A[pos_aux], &bandera);
							  pos_aux+=1;
							  cont_s+=1;
						  }

						  E -= 2 * exce_maximo - b*s ; //exceso total s*b

					   }
					  pos+=b*s;

				}

			}


			//********************   Procesa de a ulong *******************//
			while (pos+b < last && E!=0)
				// se procesa de a ulong
			{     if ( MinExcessBitmap[ DIVb(pos) ] <= E)
						break;

				  else  { E -= 2 * popcount( A[DIVb(pos)] ) - b ; //exceso total b=32 en este caso
						  (*nroNodo)+= popcount ( A[DIVb(pos)]);
						  (*nroHoja)+= popLeaves( A[DIVb(pos)], &bandera);
						}
				  pos+=b;

			}


			if (E!=0)
				if (!(A[DIVb(pos)]&(1<<b-1)) && bandera) (*nroHoja)++;
				/*
				*aux_ulong = A[DIVb(pos)];
				*aux_ulong2 = A[DIVb(pos)-1];
				aux_byte=(byte *)aux_ulong;
				aux_byte2=(byte *)aux_ulong2;
				pos_bits=MODb(pos);
				if ( ((aux_byte2[0]) & (128>> 7)) && (aux_byte[3-DIV(pos_bits)] < 128) ) (*nroHoja)++; //Realizamos esta consulta para saber si el nro anterior termina en 1 y el siguiente empieza en 0, si este es el caso estamos en presencia de una hoja.
				*/

		}


		//********************   Procesa de a bytes *******************//

		//Esta pensado para trabajar con b=32 por lo tanto siempre que frene la iteración anterior es porque llegamos al final o no existen b bit pas procesar. En este último caso siempre queda sobre el comienzo de un byte
		// se procesa de a bytes
        *aux_ulong2=0; //Es utilizado para
        *aux_ulong = A[DIVb(pos)];
        aux_byte=(byte *)aux_ulong;
        pos_bits=MODb(pos);
		while (pos+8 < last && E!=0)
		{
			if (*aux_ulong2){ //Aquí se debe realizar esta pregunta luego de haber avanzado al menos una ves en el byte en cuestión
				if ( ((aux_byte[3-DIV(pos_bits)+1]) & (128>> 7)) && (aux_byte[3-DIV(pos_bits)] < 128) ) (*nroHoja)++; //Realizamos esta consulta para saber si el nro anterior termina en 1 y el siguiente empieza en 0, si este es el caso estamos en presencia de una hoja.

			}
			else
				*aux_ulong2=1;

			if ( MinExcessTable[ aux_byte[3-DIV(pos_bits)] ] <= E)
				  break;
			  else  { E -= 2 * RankTable[ aux_byte[3-DIV(pos_bits)] ] - 8 ; //exceso total
			        //E -= 2 * popcount8(aux_byte[3-DIV(pos_bits)]) - 8 ; //exceso total
					  (*nroNodo)+= RankTable [ aux_byte[3-DIV(pos_bits)]];
					//(*nroNodo)+=popcount8(aux_byte[3-DIV(pos_bits)]);
					  (*nroHoja)+= NumOfLeaves[ aux_byte[3-DIV(pos_bits)]];
					//(*nroHoja)+=popLeaves8(aux_byte[3-DIV(pos_bits)], &bandera);
					}
			  pos+=8;
			  pos_bits+=8;
		  //if ( ((A[DIV(pos)-1]) & (128>> 7)) && (A[DIV(pos)] < 128) ) (*nroHoja)++;
		}


		//********************   Procesa de a bits *******************//
		if (pos+8 >= last && E!=0){
				if ((pos<last) && ((aux_byte[3-DIV(pos_bits)+1]) & (128>> 7)) && (aux_byte[3-DIV(pos_bits)] < 128) ) (*nroHoja)++;}
		is_one=0;
        while (E!=0 && pos <last)
        {      if (aux_byte[3-DIV(pos_bits)] & (128>> MOD(pos_bits++)) )
               { E--; is_one=1; (*nroNodo)++;}
               else
               { E++;
               	 if(is_one){(*nroHoja)++;is_one=0;}
               }
        	pos++;
        }

        free(aux_ulong);
        free(aux_ulong2);
        //free(aux_byte);
        //free(aux_byte2);
        if (E!=0)
              error ("en FindClose, no lo encontro");
        return (pos-1);
}



int es_hoja(byte *tree,ulong pos)
{
	pos++;
	if(tree[DIV(pos)] & (128>> MOD(pos++)))
		return 0; //No es hoja
	else
		return 1; //es hoja

}

//-----------------------------------------------------------------
//carga en memoria principal la cadena a la cual apunta la primera hoja de la busqueda
//-----------------------------------------------------------------
//"*cadena" cadena leida desde el archivo f_text
//"sizeCadena" longitud de la cadena a recuperar
//"f_text" archivo del cual recuperar la cadena
//"primer_hoja" Posición desde la cual se debe empezar a leer desde el archivo
void loadCadena (char *cadena,int sizeCadena,FILE *f_text, ulong primer_hoja)
{
    if (fseek(f_text, primer_hoja, SEEK_SET)) //Se mueve dentro del archivo desde el inicio (SEEK_SET) hasta la posición indicada (primer_hoja)
	error("en fseek");
    if (fread(cadena, sizeof(char),sizeCadena,f_text) != sizeCadena)
	if(!feof(f_text))
        	error("en lectura cadena a comparar");
}



/*
"index" Es una estructura que contiene los siguientes campos
	"tree" Estructura del árbol con representación de parentésis
	"cant_bit" Representa cantidad de bit útiles en tree
	"rotulos" Estructura que almacena los rotulos correspodientes a las aristas del árbol
	"saltos" Estructura que almacena los saltos correspondientes a los nodos del árbol
	"cant_hijos" Estructura que almacena cantidad de hijos que posee cada sub-árbol.
	"SA" Estructura que almacena las hojas del árbol

"length" Longitud de la cadena de entrada

"pattern" cadena de entrada para la busqueda.
"*f_text" archivo en el que se encuentra el texto
"numocc" Cantidad de ocurrencias de la palabra buscada

count devuelve 0 (cero) en caso de haber encontrado alguna ocurrencia de la palabra "pattern" y 1 (uno) en caso contrario.
*/

int count(void *index, double find_close_time, char *pattern, ulong length, ulong *numocc, FILE *f_text)
{
	//FILE *f_text;
	ulong *saltos, *cant_hijos, *SA, cant_bit;
	//ulong pos_borrar=0;
	byte *tree;
	char *rotulos;
	struc_index *index_common;

	//------->>>>>> Para medir los tiempos
	clock_t t_ini_fclose;
	double time_acum_FindClose=0;
	//-------<<<<<< Para medir los tiempos

	index_common=index;
	SA=&(*index_common->SA);
	cant_bit=(*index_common).cant_bit;
	cant_hijos=&(*index_common->cant_hijos);
	tree=&(*index_common->ptr_arr_parent);
	rotulos=&(*index_common->rotulo_text);
	saltos=&(*index_common->saltos);

	char *cadena; //utilizada para leer una cadena del archivo f_text y compararla con "c"
	ulong pos=0, pos_cierre=0, pos_anterior=0, primer_hoja=0;
	int cant_h_vistos=0,primera_vez=1, testigo_pos_term; //long_c=0;
	//indice_C_H_actual representa el nro de hijos en el nodo que estoy parado
	//indice_C_H_virtual representa el nro de hijos que tiene el subarbol en el cual puede seguirse la busqueda
	ulong indice_tree=0, pos_tree=0, indice_R=0, indice_S=0, indice_C_H_actual=0, indice_C_H_virtual=0, indice_SA=0, indice_c=0;

	ulong nroNodo=0, nroHoja=0, nro_Rotulos_Saltar=0, nro_saltos_Saltar=0, nro_cant_hijos_Saltos=0, nro_SA_Saltos=0;


	//long_c=strlen(pattern);//Longitud de la cadena a buscar
	pos=1; indice_S=1; indice_C_H_virtual=1;
	cursor_pos_term_raiz=0;//Cursor estructura posiciones terminación hijos del nodo raiz
	testigo_pos_term=1;

	//ulong nroNodo_borrar=0,nroHoja_borrar=0;
	while((indice_c < length) && (cant_hijos[indice_C_H_actual] > cant_h_vistos) && (rotulos[indice_R] <= pattern[indice_c]))
	{
		if(pattern[indice_c]!=rotulos[indice_R])
		{
			pos_anterior=pos;

			if(testigo_pos_term)
			{
				pos=pos_term_raiz[cursor_pos_term_raiz++];
				nroNodo=pos_term_raiz[cursor_pos_term_raiz++]-1; //resto 1 porque en este estructura se cuenta el bit en el que estoy parado que es un 1
				nroHoja=pos_term_raiz[cursor_pos_term_raiz++];
			}
			else
			{
				/*** Propuesta Dario **
				t_ini_fclose = getTime_count();
				pos=FindClose(bitmap_ulong, pos, cant_bit, &nroNodo, &nroHoja);
				time_acum_FindClose += (getTime_count()- t_ini_fclose);
				 */
				/* Jesi con clock **/
				t_ini_fclose = clock();
				pos=FindClose(bitmap_ulong, pos, cant_bit, &nroNodo, &nroHoja);
				time_acum_FindClose += ((double)(clock() - t_ini_fclose) / CLOCKS_PER_SEC);
				/**/
			}

			//pos=FindClose(tree, pos, cant_bit, &nroNodo, &nroHoja);
			if (pos_anterior!=pos-1) //Falso si consulto parado en un 1 correspondiente a una hoja
			{	nro_Rotulos_Saltar= nroNodo+1;//mantiene el nro lugares que debo moverme en la estructura de rotulos
				nro_saltos_Saltar=nroNodo-nroHoja+1;//Mantine el nro de lugares que debo moverme en la estructura saltos
				nro_cant_hijos_Saltos=nroNodo-nroHoja+1;//Mantine el nro de lugares que debo moverme en la estructura Cantidad de hijos
				nro_SA_Saltos=nroHoja;//Mantine el nro de lugares que debo moverme en la estructura que contiene a SA

				nroNodo=0; nroHoja=0;
				indice_R+=nro_Rotulos_Saltar;
				indice_S+=nro_saltos_Saltar;
				indice_C_H_virtual+=nro_cant_hijos_Saltos;
				indice_SA+=nro_SA_Saltos;
			}
			else
			{
				indice_SA+=1;//si es una hoja entonces aumentamos el indice correspondiente a la estructura que mantiene las hojas
				indice_R+=1;
				nroNodo=0; nroHoja=0;
			}

			pos+=1;
			cant_h_vistos+=1;
		}
		else
		{
			testigo_pos_term=0;
			if(es_hoja(tree,pos))
			{
				indice_c=length;//Para que cuando salga de la iteración sea un caso en el cual debemos preguntar si es el elemento
				pos+=1;
				break;//Salimos de la iteración ya que llegamos a una hoja.
			}
			else
			{
				if (indice_c == length-1)//Caso en el cual y chequeamos el último caracter del string y lo encontramos, en este caso tenemos que parar la iteración ir a buscar el string con SA correspondiente y chequear si es el elemento buscado
				{
					indice_c=length;//Para que cuando salga de la iteración sea un caso en el cual debemos preguntar si es el elemento
					pos+=1;
					break;//Salimos de la iteración ya que llegamos a una hoja.
				}
				else
				{
					indice_c=saltos[indice_S];//Realizamos el salto correspondiente en la cadena de entrada
					indice_C_H_actual=indice_C_H_virtual;
					indice_C_H_virtual+=1;
					indice_R+=1;
					pos+=1;
					indice_S+=1;
					cant_h_vistos=0;
				}
			}
		}
	}
	*find_close_time += time_acum_FindClose;

	if(indice_c >= length)
	{
		primer_hoja= SA[indice_SA];

		cadena = (uchar *)malloc((length*sizeof(uchar))+sizeof(uchar));
		cadena[length]='\0';
		loadCadena (cadena,length,f_text,primer_hoja);

		if(!strcmp(pattern,cadena)) //Si son iguales llamamos a FindClose para saber el nro de hojas que debo devolver
		{
			/*** Propuesta Dario **
			t_ini_fclose=getTime_count();
			pos_cierre = FindClose (bitmap_ulong, pos-1, cant_bit, &nroNodo, &nroHoja);
			*parametro_time += (getTime_count()- t_ini_fclose);
			 */

			/* Jesi con clock **/
			t_ini_fclose = clock();
			pos_cierre = FindClose (bitmap_ulong, pos-1, cant_bit, &nroNodo, &nroHoja);
			(*find_close_time) += ((double)(clock() - t_ini_fclose) / CLOCKS_PER_SEC);
			/**/


			//pos_cierre=FindClose (tree, pos-1, cant_bit, &nroNodo, &nroHoja);
			if(!nroHoja)
				(*numocc)=1;
			else
				(*numocc)=nroHoja; //numocc optiene el nro de posiciones en las cuales comienza esta cadena

			free(cadena);
			return 0;
		}
		else{
			(*numocc)=0;
			free(cadena);
			return 0;
		}
		//free(cadena);

	}
	else{
		(*numocc)=0;
		//free(cadena);
		return 0;
	}

}





/*
"index" Es una estructura que contiene los siguientes campos
	"tree" Estructura del árbol con representación de parentésis
	"cant_bit" Representa cantidad de bit útiles en tree
	"rotulos" Estructura que almacena los rotulos correspodientes a las aristas del árbol
	"saltos" Estructura que almacena los saltos correspondientes a los nodos del árbol
	"cant_hijos" Estructura que almacena cantidad de hijos que posee cada sub-árbol.
	"SA" Estructura que almacena las hojas del árbol

"length" Longitud de la cadena de entrada

"pattern" cadena de entrada para la busqueda.
"*f_text" archivo en el que se encuentra el texto
"numocc" Cantidad de ocurrencias de la palabra buscada

count devuelve 0 (cero) en caso de haber encontrado alguna ocurrencia de la palabra "pattern" y 1 (uno) en caso contrario.
*/



int locate(void *index, char *pattern, ulong length, ulong **locate, ulong *numocc, FILE *f_text)
{
	//FILE *f_text;
	ulong *saltos, *cant_hijos, *SA, cant_bit;
	byte *tree;
	char *rotulos;
	struc_index *index_common;



	index_common=index;
	SA=&(*index_common->SA);
	cant_bit=(*index_common).cant_bit;
	cant_hijos=&(*index_common->cant_hijos);
	tree=&(*index_common->ptr_arr_parent);
	rotulos=&(*index_common->rotulo_text);
	saltos=&(*index_common->saltos);



	char *cadena; //utilizada para leer una cadena del archivo f_text y compararla con "c"
	ulong pos=0, pos_cierre=0, pos_anterior=0, primer_hoja=0;
	int cant_h_vistos=0,primera_vez=1, testigo_pos_term; //long_c=0;
	//indice_C_H_actual representa el nro de hijos en el nodo que estoy parado
	//indice_C_H_virtual representa el nro de hijos que tiene el subarbol en el cual puede seguirse la busqueda
	ulong indice_tree=0, pos_tree=0, indice_R=0, indice_S=0, indice_C_H_actual=0, indice_C_H_virtual=0, indice_SA=0, indice_c=0;

	ulong nroNodo=0, nroHoja=0, nro_Rotulos_Saltar=0, nro_saltos_Saltar=0, nro_cant_hijos_Saltos=0, nro_SA_Saltos=0;


	//long_c=strlen(pattern);//Longitud de la cadena a buscar
	pos=1; indice_S=1; indice_C_H_virtual=1;
	cursor_pos_term_raiz=0;//Cursor estructura posiciones terminación hijos del nodo raiz
	testigo_pos_term=1;
	while((indice_c < length) && (cant_hijos[indice_C_H_actual] > cant_h_vistos) && (rotulos[indice_R] <= pattern[indice_c]))
	{
		if(pattern[indice_c]!=rotulos[indice_R])
		{

			pos_anterior=pos;
			//pos=FindClose (bitmap_ulong, pos, cant_bit, &nroNodo, &nroHoja);
			//pos=FindClose (tree, pos, cant_bit, &nroNodo, &nroHoja);

			if(testigo_pos_term)
			{
				pos=pos_term_raiz[cursor_pos_term_raiz++];
				nroNodo=pos_term_raiz[cursor_pos_term_raiz++]-1; //resto 1 porque en este estructura se cuenta el bit en el que estoy parado que es un 1
				nroHoja=pos_term_raiz[cursor_pos_term_raiz++];
			}
			else
				pos=FindClose(bitmap_ulong, pos, cant_bit, &nroNodo, &nroHoja);



			if (pos_anterior!=pos-1) //Falso si consulto parado en un 1 correspondiente a una hoja
			{	nro_Rotulos_Saltar= nroNodo+1;//mantiene el nro lugares que debo moverme en la estructura de rotulos
				nro_saltos_Saltar=nroNodo-nroHoja+1;//Mantine el nro de lugares que debo moverme en la estructura saltos
				nro_cant_hijos_Saltos=nroNodo-nroHoja+1;//Mantine el nro de lugares que debo moverme en la estructura Cantidad de hijos
				nro_SA_Saltos=nroHoja;//Mantine el nro de lugares que debo moverme en la estructura que contiene a SA

				nroNodo=0; nroHoja=0;
				indice_R+=nro_Rotulos_Saltar;
				indice_S+=nro_saltos_Saltar;
				indice_C_H_virtual+=nro_cant_hijos_Saltos;
				indice_SA+=nro_SA_Saltos;
			}
			else
			{
				indice_SA+=1;//si es una hoja entonces aumentamos el indice correspondiente a la estructura que mantiene las hojas
				indice_R+=1;
				nroNodo=0; nroHoja=0;
			}

			pos+=1;
			cant_h_vistos+=1;
		}
		else
		{
			testigo_pos_term=0;
			if(es_hoja(tree,pos))
			{
				indice_c=length;//Para que cuando salga de la iteración sea un caso en el cual debemos preguntar si es el elemento
				pos+=1;
				break;//Salimos de la iteración ya que llegamos a una hoja.
			}
			else
			{
				if (indice_c == length-1)//Caso en el cual y chequeamos el último caracter del string y lo encontramos, en este caso tenemos que parar la iteración ir a buscar el string con SA correspondiente y chequear si es el elemento buscado
				{
					indice_c=length;//Para que cuando salga de la iteración sea un caso en el cual debemos preguntar si es el elemento
					pos+=1;
					break;//Salimos de la iteración ya que llegamos a una hoja.
				}
				else
				{
					indice_c=saltos[indice_S];//Realizamos el salto correspondiente en la cadena de entrada
					indice_C_H_actual=indice_C_H_virtual;
					indice_C_H_virtual+=1;
					indice_R+=1;
					pos+=1;
					indice_S+=1;
					cant_h_vistos=0;
				}
			}
		}
	}

	if(indice_c >= length)
	{
		primer_hoja= SA[indice_SA];
		//cadena = (uchar *) malloc (sizeof (uchar) * (length));
		//cadena = (char *) malloc (sizeof (char) * (length));
		cadena = (uchar *)malloc((length*sizeof(uchar))+sizeof(uchar));
		verif(cadena);
		cadena[length]='\0';
		loadCadena (cadena,length,f_text,primer_hoja);

		//pattern=(uchar *) realloc(pattern, length+1);
		//verif(pattern);
		//pattern[length]='\0';
		if(!strcmp(pattern,cadena)) //Si son iguales llamamos a FindClose para saber el nro de hojas que debo devolver
		{
			pos_cierre=FindClose (bitmap_ulong, pos-1, cant_bit, &nroNodo, &nroHoja);
			//pos_cierre=FindClose (tree, pos-1, cant_bit, &nroNodo, &nroHoja);
			if(!nroHoja)
				(*numocc)=1;
			else
				(*numocc)=nroHoja; //numocc optiene el nro de posiciones en las cuales comienza esta cadena

			(*locate)=&SA[indice_SA];//locate obtiene la posición de la primer hoja que se de debe devolver

			/*
			*locate=(ulong *)realloc(*locate,*numocc*sizeof(ulong)); //Pedimos memoria para el arreglo donde devolvemos el locate
			verif(*locate);


			//Copiamos las posiciones donde encontramos la cadena búscada
			int i=0;
			while(i<*numocc){
				//*locate[i] = SA[indice_SA];
				*(*locate+(i*sizeof(ulong)))=SA[indice_SA];
				i+=1;
				indice_SA+=1;
			}*/
			free(cadena);
			return 0;
		}
		else{
			(*numocc)=0;
			free(cadena);
			return 0;
		}
		//free(cadena);

	}
	else{
		(*numocc)=0;
		//free(cadena);
		return 0;
	}


}



