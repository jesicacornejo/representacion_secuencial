#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
/* Definición de Tipos */

//typedef unsigned char byte;

//typedef unsigned long ulong;
typedef unsigned char byte;
#ifndef uchar
#define uchar unsigned char
#endif
//#ifndef ulong
//#define ulong unsigned long
//#endif
#ifndef ulong  //Este debería llamarse uint pero le pongo ulong porque tengo todo el código de tipo ulong
#define ulong unsigned int
#endif
typedef ulong elemLCP;
typedef ulong elemSA;
typedef ulong elemSaltos;
typedef ulong elemCant_Hijos;
typedef char elemRotulo;
typedef ulong elemResult;

typedef struct tipoPila {
	ulong elem;
	ulong cant_bit;
	ulong nro_nodos; //Este campo se utiliza para saber el número de nodos (no hojas) que hay en este nivel
	ulong end_index_ptr_arr_parent; //Marca el limite superior de la estructuras ptr_arr_parent de acuerdo a la cantidad de elementos pedidos con ralloc
	byte *ptr_arr_parent;
} tipoPila;


//Esta estructura es utilizada para devolver el index creado
typedef struct struc_index {
	byte *ptr_arr_parent; //Puntero arreglo de paréntesis representado en binario
	ulong cant_bit;//Cantidad de bit que contiene el arreglo binario que representa los parentesis
	elemSaltos *saltos;
	elemCant_Hijos *cant_hijos;
	elemRotulo *rotulo_text;
	elemSaltos *SA;
} struc_index;


//--------------------------------------------------------------------------
// Encuentra el 0 =")" que hace match con el 1="(" en pos. Last indica cual
// es el ultimo bit con info util dentro de A
// nroNodo y nroHoja solo son utilizados por el proceso RightChild
//--------------------------------------------------------------------------
//ulong  FindClose (byte *A, ulong pos, ulong last, ulong *nroNodo, ulong *nroHoja, ulong *bitmap_ulong);

ulong  FindClose (ulong *bitmap_ulong, ulong pos, ulong last, ulong *nroNodo, ulong *nroHoja);


ulong  FindClose_byte (byte *A, ulong pos, ulong last, ulong *nroNodo, ulong *nroHoja); //Es usado sólo para la creación

void verif(void *p);

ulong popLeaves (register int x, int *bandera);

ulong popcount (register int x);

ulong b;


