//////////////////////////////////////////////////
// Procesos auxiliares para la creacion del CPT //
/////////////////////////////////////////////////
#include "common.h"
// a/8 y a%8
#define DIV(a) ((a)>> 3)
#define MOD(a) ((a) & 7)

// a/32 y a%32
#define DIVb(a) ((a)>> 5)
#define MODb(a) ((a) & 31)


extern byte RankTable[255];
extern char MinExcessTable[255];
extern char *MinExcessBitmap;
extern byte NumOfLeaves[255];
extern ulong b; //marca el nro de bit tomados a la ves 32 en este caso



//--------------------------------------------------------------------------
// Encuentra el 0 =")" que hace match con el 1="(" en pos. Last indica cual
// es el ultimo bit con info util dentro de A
// nroNodo y nroHoja solo son utilizados por el proceso RightChild
//--------------------------------------------------------------------------


char *error_index (int e)
{ static char error_mostrar[50];
  if (e) {
	  strcpy(error_mostrar,"Error en el indice");
  }
  else
	  strcpy(error_mostrar,"\0");

  return error_mostrar;
}


int free_index (struc_index *index)
{

	free((*index).ptr_arr_parent);
	free((*index).rotulo_text);
	free((*index).saltos);
	free((*index).cant_hijos);
	free((*index).SA);
	(*index).ptr_arr_parent=NULL;
	(*index).rotulo_text=NULL;
	(*index).saltos=NULL;
	(*index).cant_hijos=NULL;
	(*index).SA=NULL;
	return 0;
}

/*
//-----------------------------------------------------------------
//Escribir en un archivo el resultado dado
//-----------------------------------------------------------------

//writeResult *resutl utilizado para escribir el resultado
void writeResult(elemResult resutl, FILE *f)
{ //int error;
	if(fprintf(f, "%ld \t", resutl)<0)
		error("en writeResult");    
	 
}
*/


ulong  FindClose_byte (byte *A, ulong pos, ulong last, ulong *nroNodo, ulong *nroHoja)
{
 int E, is_one=0;;
        // procesamos los primeros bits hasta llegar al inicio de un byte
         E=-1; pos++;
         while ( MOD(pos) && E!=0)
         {     if (A[DIV(pos)] & (128>> MOD(pos++)) )
               { E--; is_one=1; (*nroNodo)++;}
               else
               { E++;
                 if(is_one){(*nroHoja)++;is_one=0;}
	           }
         }
	if (!(MOD(pos)))
		if ( ((A[DIV(pos)-1]) & (128>> 7)) && (A[DIV(pos)] < 128) ) (*nroHoja)++;//Realizamos esta consulta para saber si el nro anterior termina en 1 y el siguiente empieza en 0, si este es el caso estamos en presencia de una hoja.
        is_one=0;
        // se procesa de a bytes
        while (pos+8 < last && E!=0)
        {     if ( MinExcessTable[ A[DIV(pos)] ] <= E)
                  break;
              else  { E -= 2 * RankTable[ A[DIV(pos)] ] - 8 ; //exceso total
                      (*nroNodo)+= RankTable [ A[DIV(pos)]];
                      (*nroHoja)+= NumOfLeaves[ A[DIV(pos)]];
                    }
              pos+=8;
	      if ( ((A[DIV(pos)-1]) & (128>> 7)) && (A[DIV(pos)] < 128) ) (*nroHoja)++;
        }
	//if ( ((A[DIV(pos)-1]) & (128>> 7)) && (A[DIV(pos)] < 128) ) (*nroHoja)++;
        while (E!=0 && pos <last)
        {      if (A[DIV(pos)] & (128>> MOD(pos++)) )
               { E--; is_one=1; (*nroNodo)++;}
               else
               { E++;
		 if(is_one){(*nroHoja)++;is_one=0;}
               }
        }

        if (E!=0)
              error ("en FindClose, no lo encontro");
        return (pos-1);
}

