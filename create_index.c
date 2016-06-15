#include "common.h"
//#include "BMestatico.c"

#define sizeofbits 8


#include <sys/time.h>

//---------Constantes que especifica cantidad de lugares que se piden para cada estructura en cada asignación de memoria---------------//
#define blk_arr_parent 10  //Especifica el tamaño de bloque de asignación para la estructura que contiene los bit del árbol en cada nivel
#define block_pila 10
#define block_saltos 10
#define block_can_hijos 10
#define block_rotulos 10
#define block_SA 10
#define block_pos_raiz 9

extern ulong b; //marca el nro de bit tomados a la ves 32 en este caso
extern ulong s; //marca el nro de ulong que se toman de la estructura Rb (En nuestro caso MinExcessBitmap) en este caso es 4
//ulong b=32;
//############--------------Definición de Variable--------------------------########

//-----------------------------------------------------------------
//-------------Pila
//-----------------------------------------------------------------
ulong stope;
//tipoPila pila[cant];//Declaramos la pila
tipoPila *pila;
ulong end_index_pila=0; //Marca el limite superior de la estructuras Pila de acuerdo a la cantidad de elementos pedidos con ralloc


///-----------------------------------------------------------------
//-------------Saltos
//-----------------------------------------------------------------
//elemSaltos saltos[cant];
elemSaltos *saltos;
ulong cursor_saltos; //mantiene el cursor en el último lugar disponible de la estructura de saltos
ulong aux_saltos_correr; //Utilizada para realizar los corrimientos necesarios en la estructura de saltos para agregar un nuevo salto
ulong lim_inf_salto; //Marca el límite inferior de la estructura de saltos cuando se realiza un corrimiento.
ulong end_index_saltos=0; //Marca el limite superior de la estructuras saltos de acuerdo a la cantidad de elementos pedidos con ralloc


///-----------------------------------------------------------------
//-------------Cant Hijos
//-----------------------------------------------------------------
//elemCant_Hijos cant_hijos[cant];
elemCant_Hijos *cant_hijos;
ulong cursor_h_inicio, cursor_h_ultimo, cursor_h, aux_h_correr;
ulong end_index_hijos=0; //Marca el limite superior de la estructuras hijos de acuerdo a la cantidad de elementos pedidos con ralloc


///-----------------------------------------------------------------
//-------------Rotulos
//-----------------------------------------------------------------
//elemRotulo rotulo_text[cant];
elemRotulo *rotulo_text;
ulong cursor_rotulo, cursor_rotulo_ultimo, aux_rotulo_correr;
ulong end_index_rotulos=0; //Marca el limite superior de la estructuras rotulos de acuerdo a la cantidad de elementos pedidos con ralloc


///-----------------------------------------------------------------
//-------------SA
//-----------------------------------------------------------------
//elemSaltos SA[cant];
elemSaltos *SA;
ulong cursor_SA;
ulong end_index_SA=0; //Marca el limite superior de la estructuras SA de acuerdo a la cantidad de elementos pedidos con ralloc


//-----------------------------------------------------------------
//-------------Estructura que mantiene las posiciones en las que terminan los nodos que pertenecen a la raiz del tree
//-----------------------------------------------------------------
ulong *pos_term_raiz, cursor_pos_raiz=0, nro_nodos_raiz=0, nro_hojas_raiz=0;
ulong end_pos_raiz=0; //Marca el limite superior de la estructuras pos_raiz de acuerdo a la cantidad de elementos pedidos con ralloc



byte RankTable[255];
char MinExcessTable[255];
char *MinExcessBitmap;  //estructura que mantiene los excesos minimos del bitmap que representa la estructura del árbol
char *MinExcessBitmap_RS;  //estructura que mantiene los excesos minimos del bitmap que representa la estructura del árbol para un total de (RS*b) bit
byte NumOfLeaves[255];


ulong *bitmap_ulong; //Variable utilizada para mantener el arreglo de parentesis luego de convertirlo a ulong.




//#define error(msg) {printf("\n error, ");printf(msg); printf("\n");}

#define ASIGNAR(ptr, size){\
        ptr= (byte *)malloc(size);\
	verif(ptr);\
        memset(ptr, 0 ,size); \
}

#define MIN(a,b) (a<b) ? a : b


#define LIBERAR(p) {\
         if ((p)!=NULL)\
             {free(p); p=NULL; }\
} 

#define error(msg) {printf("\n error, ");printf(msg); printf("\n");exit(1);}

// a/8 y a%8
#define DIV(a) ((a)>> 3)
#define MOD(a) ((a) & 7)

// a/32 y a%32
#define DIVb(a) ((a)>> 5)
#define MODb(a) ((a) & 31)

// a/s y a%s
#define DIVs(a) ((a)>> 7)//Aquí asumimos que s=4 y b=32 por lo que s*b=128 --> 2^7=128.
#define MODs(a) ((a) & ((s*b)-1))
/*-----------------Fin definiciones---------------------*/


//--------------------------------------------------------------------------
// Cantidad de 1 en el vector A hasta el bit nrobit (q esta en pos nrobit-1)
//--------------------------------------------------------------------------
ulong  Rank (byte *A, ulong nrobit)
{
ulong i, acum =0, aux;

      for (i=0; i< DIV(nrobit); acum+= RankTable[A[i++]]);

      for (i=0; i< MOD(nrobit); i++)
            if ( A[DIV(nrobit)] & (128 >> i))
                acum++;
      return(acum);
}


//-----------------------------------------------------------------
//Calcula ceil(a/8) sin usar operadores / y % --- Calcula cantidad de bloques necesarios para ""a"" elementos.
//-----------------------------------------------------------------
ulong CEIL( ulong a)
{      ulong c= (MOD(a)==0)? DIV(a): (DIV(a) +1);
       return(c);
}

//#define error(msg) {printf("\n error, ");printf(msg); printf("\n");exit(1);}
//------------------------------------------------------------------------------------------
// Crea tablas con cantidad de unos, exceso de ceros y cantidad de hijos de los nro 0 al 255
//------------------------------------------------------------------------------------------
void initRankExcLeavesTables()
{
ulong i, j, aux;
char actualExcess;
int is_one;

      memset(RankTable,0, 255);
      memset(MinExcessTable,8, 255);
      memset(NumOfLeaves,0,255);
      for (i=0; i<256; i++)
      {	    actualExcess=0; aux=i; is_one=0;
            for (j=0; j<8; j++)
            {      if (aux&128)
                   {   RankTable[i]++; actualExcess++;is_one=1;}

                   else
                   {   actualExcess--;
                   	   if(is_one)
                   	   {  NumOfLeaves[i]++; is_one=0;}
                   }

                   if (actualExcess < MinExcessTable[i])
                        MinExcessTable[i]=actualExcess;

                   aux =aux<<1;
            }
      }

}

// FIN --- Crea tablas con cantidad de unos, exceso de ceros y cantidad de hijos de los nro 0 al 255
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
// Crea tablas con exceso de ceros tomando de a 32 bit (4 Bytes) del Bipmap creado
// bitmap: es la estructura que mantiene los bit que conforman la estructura del árbol
// MinExcessBitmap: es un tabla global que mantiene los excesos minimos del bitmap
// last: Cantidad de bytes que tiene el bitmap
// b: variable global que nos da el tamaño del registro.
//------------------------------------------------------------------------------------------
void initExcBitmapTables(ulong *bitmap,ulong last)
{
ulong i, j, aux, ulong_size_bitmap, cant_bit_sobrantes, Rs_size_bitmap, is=0, cur_is=0;
char actualExcess;
char actualExcess_Rs; //Mantiene el exceso minimo para la estructura Rs

	  ulong_size_bitmap=DIVb(last);  //Ver el tema si modificamos b #define DIVS(a) ((a)>> 5)
	  Rs_size_bitmap=DIVs(last);

	  MinExcessBitmap=(char *)realloc(MinExcessBitmap,ulong_size_bitmap+1);
      memset(MinExcessBitmap,8, ulong_size_bitmap+1);

      MinExcessBitmap_RS=(char *)realloc(MinExcessBitmap_RS,Rs_size_bitmap+1);
      memset(MinExcessBitmap_RS,8, Rs_size_bitmap+1);

      actualExcess_Rs=0;
	  for (i=0; i<ulong_size_bitmap; i++)
	  {	    actualExcess=0; aux=bitmap[i];
			if (is==s) //La estructura MinExcessBitmap_RS toma de a "s" ulong
			{
				cur_is+=1;
				actualExcess_Rs=0;
				is=0;
			}

			for (j=0; j<32; j++)
			{      if (aux&(1<<b-1))
					{ //2147483648 en binario  1000 0000 0000 0000 0000 0000 0000 0000  (2^(b-1))
					  actualExcess++;
					  actualExcess_Rs++;
					}

				   else
				   {
					 actualExcess--;
					 actualExcess_Rs--;
				   }

				   if (actualExcess < MinExcessBitmap[i])
						MinExcessBitmap[i]=actualExcess;

				   if (actualExcess_Rs < MinExcessBitmap_RS[cur_is])
					    MinExcessBitmap_RS[cur_is]=actualExcess_Rs;

				   aux=aux<<1;
			}
			is+=1;

	  }

	  if(MODs(last)<=32){cur_is+=1;actualExcess_Rs=0;}

      if (cant_bit_sobrantes=MODb(last)){ //En esta sección sacamos el exceso del los últimos bit, es decir puede suceder que nuestro bitmap tenga T modulos de 32bit y el último sólo resten de 0 a 31 bit por lo tanto debemos realizar un trabajo diferenciados sobre ellos
    	  	  	  // MODS(a) ((a) & 31)
    	  actualExcess=0; aux=bitmap[i];
    	              for (j=0; j<cant_bit_sobrantes; j++)//cant_bit_sobrantes es la cantidad de bits que no completan un ulong de 32 bits
    	              {      if (aux&(2^(cant_bit_sobrantes-1)))
    	              	  	 {
    	                        actualExcess++;
    	                        actualExcess_Rs++;
    	              	  	 }

    	                     else
    	                     {
    	                       actualExcess--;
    	                       actualExcess_Rs--;
    	                     }


    	                     if (actualExcess < MinExcessBitmap[i])
    	                          MinExcessBitmap[i]=actualExcess;

    	                     if (actualExcess < MinExcessBitmap_RS[cur_is])
    	                    	 MinExcessBitmap_RS[cur_is]=actualExcess_Rs;

    	                     aux =aux<<1;
    	              }
      }

}


// Convierte un arreglo de uchar a uno de ulong
//bitmap_char arreglo de uchar a convertir tener en cuenta que nos devuelve un arreglo de ulong sin modificar el arreglo de uchar
//bitmap_long arreglo de ulong en el que será devuelto el arreglo convertido
//cant_bits_bitmap cantidad de bit que tiene el arreglo de uchar
void uchartoulong(byte *bitmap_char, ulong **bitmap_long, ulong cant_bits_bitmap){
	int i, aux, aux2;
	ulong tam_bitmap_byte, tam_bitmap_ulong;
	byte *bitmap_char_aux;

	if MODb(cant_bits_bitmap){
		tam_bitmap_byte= DIV(cant_bits_bitmap)+4;
		tam_bitmap_ulong= DIVb(cant_bits_bitmap)+1;
	}
	else{
		tam_bitmap_byte= DIV(cant_bits_bitmap);
		tam_bitmap_ulong= DIVb(cant_bits_bitmap)+1;
	}



	bitmap_char_aux=(byte *)malloc(sizeof(byte)*tam_bitmap_byte);
	*bitmap_long=(ulong *)malloc(sizeof(ulong)*tam_bitmap_ulong);
	//*bitmap_long=(ulong *)realloc(*bitmap_long,tam_bitmap_ulong);

	memset(bitmap_char_aux,0, tam_bitmap_byte);
	memset(*bitmap_long,0, tam_bitmap_ulong);

	i=4;
	aux=0;
	aux2=i-1;
	while (i<= tam_bitmap_byte)
	{

		while (aux<i){
			bitmap_char_aux[aux2]=bitmap_char[aux];
			aux2-=1;
			aux+=1;
		}
		i=i+4;
		aux2=i-1;
	}

	*bitmap_long=(ulong *)bitmap_char_aux;
}

// FIN --- Crea tablas con cantidad de unos, exceso de ceros y cantidad de hijos de los nro 0 al 255
//------------------------------------------------------------------------------------------

//-----------------------------------------------------------------
//verifica memoria pedida
//-----------------------------------------------------------------

void verif(void *p)
{ if (p == NULL) error ("sin memoria\n");   }




//-----------------------------------------------------------------
//Asignar espacio si hace falta para estructura de bit
//-----------------------------------------------------------------
/*
ptr - arreglo que mantiene la estructura del árbol
n - Cantidad de bit que hay en la estructura actualmente
m - Cantidad de bit a insertar
end_index -  Cantidad de bloques que se han pedidos hasta el momento en cada bloque que existe en la pila
blk es la cantidad e bloques que se agregan al arreglo cada vez que se pide memoria.
sizeofbits nos da cuantos bits hay en el bloque
*/

void IFSPACE(byte **ptr,ulong n,ulong m, ulong *end_index, int blk) {
 	ulong cant_bloques_actual=0; //Esta variable indica cuantos bloques hay actualmente en la estructura
	ulong cant_bloques_necesarios=0; //Indica cantidad de bloques necesarios que se requieren para almacenar los elementos en cuestión
	//byte *borrar;
	cant_bloques_actual = CEIL(n);
	cant_bloques_necesarios = CEIL(m+n);

	if (*end_index < cant_bloques_necesarios)
	{	
        *end_index=((cant_bloques_necesarios/blk)+1)*blk;//Obtenemos la cantidad de bloques necesario, siempre respetando que asignamos de blk bloques o multiplos de blk si la necesidad lo requiere
		*ptr=(byte *)realloc(*ptr,(*end_index));
		verif(*ptr);
		//borrar=(*ptr)+(cant_bloques_actual);
		memset(((*ptr)+(cant_bloques_actual)),0,((*end_index)-cant_bloques_actual));
	}
}



//-----------------------------------------------------------------
//Asignar espacio si hace falta para estructura Pila
//-----------------------------------------------------------------
//ptr apuntador a estructura para asignar memoria
//n cantidad de elementos actuales en la estructura
//end_index cantidad de lugares ya asignados para la estructura ptr
//blk_pila bloques de asignación de memoria
void IFSPACE_pila(tipoPila ***ptr,ulong n,ulong *end_index, int blk_pila) {

	if ( *end_index == n )//
	{
		//tipoPila *aux, *borrar;
		int i=0;
        *end_index=*end_index+blk_pila;//
		**ptr=(tipoPila *)realloc(**ptr,((*end_index)*sizeof(tipoPila)));
		verif(**ptr);
	    while (i<blk_pila) //Inicializamos los campos de la pila recién asignados
		   {
	    	(**ptr)[n].elem=0;
	    	(**ptr)[n].cant_bit=0;
	    	(**ptr)[n].nro_nodos=0;
	    	(**ptr)[n].end_index_ptr_arr_parent=0;
	    	(**ptr)[n].ptr_arr_parent=NULL;
	    	i++;
	    	n++;
		   }
	}
}



//-----------------------------------------------------------------
//Asignar espacio si hace falta para estructura de tipo ulong (Saltos, CantHijos, SA)
//-----------------------------------------------------------------
//ptr apuntador a estructura para asignar memoria
//n cantidad de elementos actuales en la estructura
//end_index cantidad de lugares ya asignados para la estructura ptr
//blk_ulong bloques de asignación de memoria
void IFSPACE_struc_ulong(ulong **ptr,ulong n,ulong *end_index, int blk_ulong) {

	if ( *end_index == n )//
	{
		int i=0;
        *end_index=*end_index+blk_ulong;
		*ptr=(ulong *)realloc(*ptr,((*end_index)*sizeof(ulong)));
		verif(*ptr);

		while (i<blk_ulong)
		{
			(*ptr)[n]=0;
			i++;
			n++;
		}
		//memset(((*ptr)+(sizeof(ulong)*n)),0,(blk_ulong*(sizeof(ulong))));
	}
}


//-----------------------------------------------------------------
//Asignar espacio si hace falta para estructura de rotulos
//-----------------------------------------------------------------
//ptr apuntador a estructura para asignar memoria
//n cantidad de elementos actuales en la estructura
//end_index cantidad de lugares ya asignados para la estructura ptr
//blk_rotulos bloques de asignación de memoria
void IFSPACE_rotulos(char **ptr,ulong n,ulong *end_index, int blk_rotulos) {

	if ( *end_index == n )//
	{
		int i=0;
        *end_index=*end_index+blk_rotulos;//
		*ptr=(char *)realloc(*ptr,((*end_index)*sizeof(char)));
		verif(*ptr);
		memset(((*ptr)+(sizeof(char)*n)),0,blk_rotulos);
	}
}


//-----------------------------------------------
//Funciones de pilas:
//-----------------------------------------------
void Push(tipoPila **l, tipoPila v){
	int bloque;
	bloque=block_pila;
	IFSPACE_pila(&l, stope+1, &end_index_pila, bloque);
	stope++;
	(*l)[stope].elem= v.elem;
	(*l)[stope].cant_bit = v.cant_bit;
	(*l)[stope].end_index_ptr_arr_parent = v.end_index_ptr_arr_parent;
	(*l)[stope].nro_nodos = v.nro_nodos;
	(*l)[stope].ptr_arr_parent = v.ptr_arr_parent;

};

//int Pop(tipoPila *l) {
int Pop() {	
	//LIBERAR(l[stope].ptr_arr_parent);
	stope--;
	return 1;	
};

int empty () {
	if (stope == -1)
		return 1;	
	else
		return 0;
};
 
int full () {
	if (stope == 99)
		return 1;	
	else
		return 0;
};

/* -----------------------Fin de Funciones con pilas:--------------------------*/


//-----------------------------------------------------------------
//Asigna agrega una hoja al arreglo
//-----------------------------------------------------------------
// arr es el arreglo donde se encuentra los datos y posición es la posición en la cual se insertará los bits
void asignar_hoja(byte *arr, ulong posicion)
{ 	ulong pos_arr; //mantiene posición del arreglo a la que devería asignar 
	ulong pos_bits; //posición del bits dentro de la posición del arreglo
	byte mask=128; //Se utiliza como mascara para asignar el bits correspondiente 128 en binario 10000000

	pos_arr=DIV(posicion);
	pos_bits=MOD(posicion);

	mask=mask >> pos_bits;
	arr[pos_arr]=arr[pos_arr] | mask;
	//para asignar una hoja solo se coloca un 1 en al última posición ya que se asume que todas los posiciones vacias esta completadas con 0 (ceros)
  
}


//-----------------------------------------------------------------
//Borra una hoja del arreglo
//-----------------------------------------------------------------

//posición contiene la posición correspondiente a nivel de bits, Es decir la posición corresponde al nro de bits que hay anterior al bits +1
void borrar_hoja(byte *arr, ulong posicion)
{ 	ulong pos_arr; //mantiene posición del arreglo a la que devería borrar la hoja 
	ulong pos_bits; //posición del bits dentro de la posición del arreglo
	byte mask=255; //Se utiliza como mascara para asignar el bits correspondiente 255 en binario 11111111

	pos_arr=DIV(posicion);
	pos_bits=MOD(posicion);

	if(pos_bits==0)
	{
		mask<<=2;
		pos_arr-=1;
	}
	else
	{
		if(pos_bits==1)
		{
			pos_arr-=1;//Resto una a la posicion del arreglo a la cual debo sacar la hoja esto sucede porque la posicion dentro del bloque es la primera pero esto se refiere a la posicion del cero correspondiente a la hoja por lo que el uno correspondiente se encuentra en el bloque anterior
			mask<<=1;
		}
		else
			mask<<=(8-(pos_bits-2));
	}
	 arr[pos_arr]&=mask;
	//para borrar una hoja solo se coloca 00 (ceros) en la posiciçon de la hoja correspondiente Como en nuestro caso siempre borramos hojas que estan al final, no es necesario realizar corrimientos.
  
}


//-----------------------------------------------------------------
//carga en memoria principal el proximo rotulo
//Esta función recive el sa y lcp para poder moverme dentro del archivo de texto hasta la posición donde se encuentra el rotulo que debe ser leido.
//tam_texto es el tamaño del texto en bytes
//-----------------------------------------------------------------
void loadRotulo(elemRotulo *rotulo, int sizeB, FILE *f, elemSA sa, elemLCP lcp, ulong tam_texto)
{
	if ((sa+lcp) < tam_texto){
    	if (fseek(f, sa+lcp, SEEK_SET)) //Se mueve dentro del archivo desde el inicio (SEEK_SET) hasta la posición indicada (sa+lcp)
    		error("en fseek");
    	if (fread(rotulo, sizeof(elemRotulo), sizeB, f) != sizeB)
    		if(!feof(f))
    			error("en loadRotulo")
    }
   else
	   if ((sa+lcp) == tam_texto)
		   *rotulo = 0;
       else
    	   error("en loadRotulo")

}

//-----------------------------------------------------------------
//carga en memoria principal el proximo bloque del SA
//-----------------------------------------------------------------

 void loadSA (elemSA *sa, int sizeB, FILE* f)
{
   if (fread(sa, sizeof(elemSA), sizeB, f) != sizeB)
	if(!feof(f))
        	error("en loadSA");

}


 //-----------------------------------------------------------------
 //carga en memoria principal el proximo bloque del LCP
 //-----------------------------------------------------------------

 //elemLCP *lcp
 void loadLCP(elemLCP *lcp, int sizeB, FILE *f)
 {
	 if (fread(lcp, sizeof(elemLCP), sizeB, f) != sizeB)
		 if(!feof(f))
         	error("en loadLCP");

 }



 //--------------------------------------------------------------------------
 //  Copia nbitsToCopy desde source a dest, asume que dest tiene espacio y que todo el espacio libre esta inicializado en cero
 //  fromBit y toBit: indican desde y hacia cual bit se realiza la copia.
 //  fromBit contiene el nro de bits correspondiente de donde se debe empezar a copiar. Ej si hay 28 bits y se debe empezar a copiar desde el bits 16 frombit contiene el valor 16. Idem para toBit con la diferencia que este último es para el destino.
 //  flag: se utiliza para saber si corresponde  reinicializar en 0 el espacio que ya se hayan copiado desde (source)

 //
 //--------------------------------------------------------------------------

void CopyBits(byte *dest, ulong toBit, byte *source, ulong fromBit, ulong nbitsToCopy)
 {
 	ulong  i, bitsSourceCurrent, pbitCopyDest, pdest, bitsDestCurrent;
 	byte maskSource, maskDest, aux;
 	int p, pbitCopy,contador_bit;

 	bitsSourceCurrent=fromBit; //bitsSourceCurrent mantiene el valor que corresponde al bit que se tiene que copiar desde el fuente al destino

 	bitsDestCurrent=toBit;
 	contador_bit=0;
 	while (contador_bit < nbitsToCopy)
 	{
 		p=DIV(bitsSourceCurrent); //p contiene la posición del arreglo fuente
 		pbitCopy=MOD(bitsSourceCurrent);//pbitCopy contiene la posicion del bit a ser copiado dentro del byte
 		maskSource=128>>pbitCopy;
 		aux=source[p]&maskSource; //aux es cero si el bit a asignar es cero de lo contrario es distinto de cero

 		pdest=DIV(bitsDestCurrent); //pdest contiene la posición del arreglo destino
 		pbitCopyDest=MOD(bitsDestCurrent);//pbitCopyDest contiene la posicion del bit en el cual se debe copiar del fuente
 		maskSource=128>>pbitCopyDest;

 		if(aux)
 			dest[pdest]|=maskSource;

 		bitsSourceCurrent +=1;
 		bitsDestCurrent +=1;
 		contador_bit+=1;
 	}

 }



void agregar_un_bits(byte *dest, ulong can_elemn)
{
	ulong p, pbitCopy;
	p=DIV(can_elemn); //p contiene la posición del arreglo fuente
	byte mask;
	mask='0';

	pbitCopy=MOD(can_elemn);//pbitCopy contiene la posicion del bit a ser copiado dentro del byte
	mask=128>>pbitCopy;
	dest[p] |=mask;
}



void inicializar_nodo_pila(tipoPila *nodo_pila)
{
	nodo_pila->elem=0;
	nodo_pila->cant_bit=0;
	nodo_pila->end_index_ptr_arr_parent=0;
	nodo_pila->nro_nodos=0;
	nodo_pila->ptr_arr_parent=NULL;

}


//-------------------Función load_index()----------------------------------------
//## filename_dat= Nombre archivo que contiene el texto
//## filename_lcp= Nombre archivo lcp
//## filename_sa= Nombre archivo sa
//## **index= Estructura que representa el índice a crear
//## Verbose= <1> Si imprime resultado
//## **f_result_size= Puntero a archivo donde se almacenan las estadísticas - Tiempo creación - Tamaño índice.
//## size_text=Tamaño en MB del texto indexado - Sólo para Estadísticas.

int load_index (char *filename_dat, char *filename_lcp, char *filename_sa,  void **index, char Verbose, FILE **f_result_size, char *size_text)
{
	FILE *f_lcp, *f_sa, *f_rotulo ;
	//FILE *f_resultado, *f_resultado_locate, *f_patt;
	//int blk_bis,sizeofbits_bis;
	//blk_bis=blk;
	//sizeofbits_bis=sizeofbits;

	//---------------Declaración de Variable-----------------//
	int sizeB =1;
	
	elemLCP lcp, lcp_anterior; //tanto lcp_anterior como sa_anterior son variables que se usan para la creación de la estrutura Rotulos_text
	elemSA sa, sa_anterior;
	elemRotulo rotulo;
	struc_index *index_common;

	ulong count, locate, final_archivo, nro_nodos_nivel=0;
	char cadena_b[10];

	ulong aux_asig=0; //Utilizada para saber cuantos lugares debemos asignar

	//-----------------Pedimos memoria para la estructura que contendrá todos los punteros a las estructura que conforman el indice-----------------------//
	index_common = (struc_index *)malloc(sizeof(struc_index));



//-------inicialización de Variables
	int stope_anterior=-1, cant_bits=0;
	stope=-1;
	cursor_saltos=0;

	ulong salto=0;
	tipoPila nodo_pila, struc_the_tree;
	
	//-------------------------estructura Cantidad de hijos--------------------/
	cursor_h_inicio=0;
	cursor_h_ultimo=0;
	cursor_h=0;
	aux_h_correr=0;

	//-------------------------estructura Rotulos--------------------//
	int lim_inf_rotulo=0;
	//rotulo_text[1000];
	cursor_rotulo=0; cursor_rotulo_ultimo=0; aux_rotulo_correr=0;


	cursor_SA=0;


	ulong pos=0,last=0,nroNodo=0,nroHoja=0, final=1;//usada en la función findclose(). pos representa la posición del 1 al cual queres buscarle el cero que lo cierra. Last representa la posición del último bit util, "nro_Nodo" representa el nro de nodos que hay entre el bit 1 y su 0 correspondiente, incluyendo a las hojas. "nroHojas" representa el nro de hojas que hay entre el bit 1 y su 0 correspondiente.
	//---------------Fin de Variable-----------------//


//---------------Apertura de archivos y carga de datos----------//

	//--------------LCP----------------------//
	f_lcp = fopen(filename_lcp, "r" );

	if(!f_lcp)
		error("No se abrio el archivo .lcp");



	//-------------SA---------------------//
	f_sa = fopen(filename_sa, "r" );
	if(!f_sa)
		error("No se abrio el archivo .sa");



	//------------Texto---------------------//
	f_rotulo = fopen(filename_dat, "r" );
	if(!f_rotulo)
	error("No se abrio el archivo .dat");
	fseek( f_rotulo, 0, SEEK_END );
	final_archivo = ftell( f_rotulo ); //Obtenemos la última posición del archivo la cual lo usamos en la función "loadRotulo"




	initRankExcLeavesTables();

	inicializar_nodo_pila(&nodo_pila);

	int i=0;//COMENTAR

	while (!feof(f_lcp))
	{
		loadLCP (&lcp,sizeB,f_lcp);//cargo en lcp el bloque correspondiente de LCP

		loadSA (&sa,sizeB,f_sa);



		nodo_pila.elem=lcp;
		nodo_pila.cant_bit=0;

		//if (lcp==0)
			//	printf("para");


		IFSPACE_struc_ulong(&SA,cursor_SA,&end_index_SA,block_SA);
		SA[cursor_SA]=sa;
		cursor_SA+=1;

		if (empty())
		{
			ASIGNAR(nodo_pila.ptr_arr_parent,(sizeof(byte)*blk_arr_parent));
			agregar_un_bits(nodo_pila.ptr_arr_parent,0);
			nodo_pila.cant_bit+=1;
			nodo_pila.end_index_ptr_arr_parent=blk_arr_parent; //Le damos el número de bytes pedidos para la estructura que contiene los bit, en este caso es blk_arr_parent porque es la primera vez que pedimos
			Push(&pila, nodo_pila);
			//ASIGNAR(nodo_pila.ptr_arr_parent,(sizeof(byte)*blk_arr_parent));
			inicializar_nodo_pila(&nodo_pila);
			asignar_hoja(pila[stope].ptr_arr_parent,1);
			pila[stope].cant_bit+=2;
			//ASIGNAR(nodo_pila.ptr_arr_parent,(sizeof(byte)*blk_arr_parent));
			
			//-----------------Creación Estructura posiciones finalización de hijos Raiz---------------//
			nro_nodos_raiz=1;
			nro_hojas_raiz=1;


			//-------------------Creación de estructura Saltos----------------//
			IFSPACE_struc_ulong(&saltos,cursor_saltos,&end_index_saltos, block_saltos);
			saltos[cursor_saltos]=lcp;
			cursor_saltos+=1;
			
			
			//-------------------Creación de estructura Cantidad de hijos--------------------------//
			IFSPACE_struc_ulong(&cant_hijos,cursor_h_ultimo,&end_index_hijos,block_can_hijos);
			cant_hijos[cursor_h_inicio]+=1;
			cursor_h_ultimo+=1;
					      

			//------------------Creación de estructura Rotulos_text--------------------------------//
			loadRotulo (&rotulo,sizeB,f_rotulo,sa,lcp, final_archivo);
			IFSPACE_rotulos(&rotulo_text,cursor_rotulo,&end_index_rotulos,block_rotulos);
			rotulo_text[cursor_rotulo]=rotulo;
			cursor_rotulo+=1;
			lcp_anterior=lcp;
			sa_anterior=sa;
			
			
		}
			
		else
		{		

			if ((pila[stope].elem)==(lcp)) //El valor del LCP es igual al que se encuetra en el tope de la pila
			{		
				cant_bits=pila[stope].cant_bit;
				IFSPACE(&pila[stope].ptr_arr_parent,cant_bits,2,&pila[stope].end_index_ptr_arr_parent, blk_arr_parent);
				

				asignar_hoja(pila[stope].ptr_arr_parent,cant_bits);
				pila[stope].cant_bit+=2;
				
				
			//-------------------Creación de estructura Cantidad de hijos--------------------------//
				if(lcp == 0){
				  //-----------------Estructura posiciones de cierre hijos raiz--------//
				  IFSPACE_struc_ulong(&pos_term_raiz,cursor_pos_raiz,&end_pos_raiz, block_pos_raiz);
				  pos_term_raiz[cursor_pos_raiz]=cant_bits-1; //Asignamos posición de cierre
				  cursor_pos_raiz+=1;
				  pos_term_raiz[cursor_pos_raiz]=nro_nodos_raiz;//Asignamos cantidad de Nodos en el subárbol
				  cursor_pos_raiz+=1;
				  pos_term_raiz[cursor_pos_raiz]=nro_hojas_raiz;//Asignamos cantidad de Hojas en el subárbol
				  cursor_pos_raiz+=1;
				  nro_nodos_raiz=0;
				  nro_hojas_raiz=0;
				  //-----------------FIN Estructura posiciones de cierre hijos raiz--------//


				  cant_hijos[cursor_h_inicio]+=1;
				}
				else
				  cant_hijos[cursor_h]+=1;
			//------------------------------------------------------------------------------------//
			//-----------------Estructura posiciones de cierre hijos raiz--------//
				nro_nodos_raiz+=1;
				nro_hojas_raiz+=1;
			//-----------------FIN Estructura posiciones de cierre hijos raiz--------//


			//------------------Creación de estructura Rotulos_text--------------------------------//
				loadRotulo (&rotulo,sizeB,f_rotulo,sa,lcp,final_archivo);
				IFSPACE_rotulos(&rotulo_text,cursor_rotulo,&end_index_rotulos,block_rotulos);
				rotulo_text[cursor_rotulo]=rotulo;
				cursor_rotulo+=1;
				lcp_anterior=lcp;
				sa_anterior=sa;
				


					
			}
			else
			{
				if (pila[stope].elem<lcp) // El valor del LCP es mayor que el valor que se encuentra en el tope de la pila
				{	
					cant_bits=pila[stope].cant_bit;
					borrar_hoja(pila[stope].ptr_arr_parent,cant_bits);
					//-----------------Estructura posiciones de cierre hijos raiz--------//
						nro_nodos_raiz-=1;
						nro_hojas_raiz-=1;
					//-----------------FIN Estructura posiciones de cierre hijos raiz--------//
					pila[stope].cant_bit-=2;
					ASIGNAR(nodo_pila.ptr_arr_parent, (sizeof(byte)*blk_arr_parent));//asigno memoria para un nuevo arreglo de bits
					nodo_pila.end_index_ptr_arr_parent=blk_arr_parent; //Le damos el número de bytes pedidos para la estructura que contiene los bit, en este caso es blk_arr_parent porque es la primera vez que pedimos
					Push(&pila, nodo_pila);
					inicializar_nodo_pila(&nodo_pila);

					cant_bits=pila[stope].cant_bit;
					IFSPACE(&pila[stope].ptr_arr_parent,cant_bits,4, &pila[stope].end_index_ptr_arr_parent,blk_arr_parent);
					asignar_hoja(pila[stope].ptr_arr_parent,0);
					//-----------------Estructura posiciones de cierre hijos raiz--------//
						nro_nodos_raiz+=1;
						nro_hojas_raiz+=1;
					//-----------------FIN Estructura posiciones de cierre hijos raiz--------//

					pila[stope].cant_bit+=2;
					cant_bits=pila[stope].cant_bit;
					asignar_hoja(pila[stope].ptr_arr_parent,cant_bits);
					//-----------------Estructura posiciones de cierre hijos raiz--------//
						nro_nodos_raiz+=1;
						nro_hojas_raiz+=1;
					//-----------------FIN Estructura posiciones de cierre hijos raiz--------//

					pila[stope].cant_bit+=2;
					

				//-------------------Creación de estructura Saltos----------------//
					IFSPACE_struc_ulong(&saltos,cursor_saltos,&end_index_saltos, block_saltos);
					saltos[cursor_saltos]=lcp;
					cursor_saltos+=1;
					
														
				//-------------------Creación de estructura Cantidad de hijos--------------------------//
					IFSPACE_struc_ulong(&cant_hijos,cursor_h_ultimo,&end_index_hijos,block_can_hijos);
					cursor_h=cursor_h_ultimo; //Le damos el último antes de sumarlo ya que cursor_h puedo haber quedado en medio del arreglo de hijos
					cursor_h_ultimo+=1;
					cant_hijos[cursor_h]+=2;

				//------------------Creación de estructura Rotulos_text--------------------------------//
					
					loadRotulo (&rotulo,sizeB,f_rotulo,sa_anterior,lcp,final_archivo); //Cargamos el rotulo correspondiente al sufijo anterior
					IFSPACE_rotulos(&rotulo_text,cursor_rotulo,&end_index_rotulos,block_rotulos);
					rotulo_text[cursor_rotulo]=rotulo;
					cursor_rotulo+=1;
					loadRotulo (&rotulo,sizeB,f_rotulo,sa,lcp, final_archivo); //Cargamos el rotulo que corresponde en este caso
					IFSPACE_rotulos(&rotulo_text,cursor_rotulo,&end_index_rotulos,block_rotulos);
					rotulo_text[cursor_rotulo]=rotulo;
					cursor_rotulo+=1;
					lcp_anterior=lcp;
					sa_anterior=sa;

				}
				else
				{
					//lim_inf_salto=cursor_saltos-1;//Le damos a lim_inf_salto la última posicion de la estructura saltos que contiene un valor
					lim_inf_salto=cursor_saltos;
					lim_inf_rotulo=cursor_rotulo;//Le damos a lim_inf_rotulo la última posicion de la estructura rotulo_text que contiene un valor
					
					if (lcp!=0)
					{ int desp;//Variable utilizada para moverme sobre la pila
						desp=	stope;							
						while (pila[desp].elem > lcp)
						{
						lim_inf_rotulo-=((pila[desp].cant_bit/2));//Restamos la cantidad de lugares de rotulos con el mismo lcp
						desp-=1;	 
						}
												
					}



					while (pila[stope].elem > lcp)
					{						
						
						stope_anterior=stope - 1;  
						if(pila[stope_anterior].elem < lcp) //este caso es cuando en la pila tenemos primero un 0 luego un 2 y ahora viene un 1
						{
							//-------------------Creación de estructura Rotulos--------------------------//
							// lim_inf_rotulo-=(pila[stope].cant_bit/2)-1;//Restamos la cantidad de lugares de rotulos con el mismo lcp	
							//-------------------------------------------------------------------------------------//

							nodo_pila.elem=lcp;
							aux_asig= (((((pila[stope].cant_bit+4)/8)+1)/blk_arr_parent)+1)*blk_arr_parent; //Calculamos la cantidad de espacio necesarios para poder copiar los bites necesarios teniendo en cuenta que asignamos de a bloques "blk_arr_parent"

							ASIGNAR(nodo_pila.ptr_arr_parent,sizeof(byte)*aux_asig);
							agregar_un_bits(nodo_pila.ptr_arr_parent,0);
							nro_nodos_raiz+=1;//Para estructura posiciones de cierre hijos raiz

							nodo_pila.end_index_ptr_arr_parent=aux_asig; //Le damos el número de bytes pedidos para la estructura que contiene los bit, en este caso aux_asig contiene la cantidad de bytes necesarios para copiar los bites respetando que la asignacion de realiza de a bloques "blk_arr_parent"
							aux_asig=0;
							CopyBits(nodo_pila.ptr_arr_parent, 1, pila[stope].ptr_arr_parent, 0, pila[stope].cant_bit);
							nodo_pila.cant_bit=pila[stope].cant_bit+2;//Sumo el bit del inicio y el bit del final 	

							nodo_pila.nro_nodos=pila[stope].nro_nodos+1;//Esto es para trabajar con la estructura cantidad de hijos
							//nro_nodos_nivel=pila[stope].nro_nodos;//Esto es para trabajar con la estructura cantidad de hijos

							//Pop(pila);
							LIBERAR(pila[stope].ptr_arr_parent);
							Pop();
							Push(&pila,nodo_pila);
							inicializar_nodo_pila(&nodo_pila);

							//IFSPACE(&pila[stope].ptr_arr_parent,pila[stope].cant_bit,2,&pila[stope].end_index_ptr_arr_parent, blk_arr_parent);
							asignar_hoja(pila[stope].ptr_arr_parent,pila[stope].cant_bit);
							//-----------------Estructura posiciones de cierre hijos raiz--------//
								nro_nodos_raiz+=1;
								nro_hojas_raiz+=1;
							//-----------------FIN Estructura posiciones de cierre hijos raiz--------//
							pila[stope].cant_bit+=2;
					
							//-------------------Creación de estructura Saltos----------------//

							/*aux_saltos_correr=cursor_saltos;
							while (aux_saltos_correr >= lim_inf_salto) //Hace los corrimientos necesarios para hacer lugar al nro de hijos del nodo intermedio.
							{
							  saltos[aux_saltos_correr+1]= saltos[aux_saltos_correr];
							  aux_saltos_correr-=1;
							}*/
							pos=0;
							aux_saltos_correr=0;
							while (pos<pila[stope].cant_bit)//Esta iteración nos da cuantos nodos, que no son hojas, existen en un nivel de la pila particular
							{
								nroNodo=0;
								nroHoja=0;
								pos=FindClose_byte (pila[stope].ptr_arr_parent, pos, pila[stope].cant_bit, &nroNodo, &nroHoja);
								aux_saltos_correr+=nroNodo-nroHoja+1;
								pos+=1;
							}
							//aux_saltos_correr representa la cantiadad de nodos No hojas que existen en un cierto nivel de la pila
							//Al salir de la iteración debemos restar 1 porque siempre existe una hoja al final de este nivel que es contada dentro de la iteración, Tener en cuenta que es el único caso en el que se cuenta una hoja
							aux_saltos_correr-=1;
							IFSPACE_struc_ulong(&saltos,cursor_saltos,&end_index_saltos, block_saltos);
							while (aux_saltos_correr > 0) //Hace los corrimientos necesarios para hacer lugar al nro de hijos del nodo intermedio.
							{
							  saltos[lim_inf_salto]= saltos[lim_inf_salto-1];
							  aux_saltos_correr-=1;
							  lim_inf_salto-=1;
							}


							saltos[lim_inf_salto]=lcp;
							cursor_saltos+=1;
							//---------------------------------------------------------------//
							
							
							//-------------------Creación de estructura Cantidad de hijos--------------------------//
							  aux_h_correr=cursor_h_ultimo-1;
							  IFSPACE_struc_ulong(&cant_hijos,cursor_h_ultimo,&end_index_hijos,block_can_hijos);
							  cursor_h_ultimo+=1;
							  while (aux_h_correr >= cursor_h) //Hace los corrimientos necesarios para hacer lugar al nro de hijos del nodo intermedio.
							  {
							    cant_hijos[aux_h_correr+1]= cant_hijos[aux_h_correr];
							    aux_h_correr-=1;
							  }
							  cant_hijos[cursor_h]=2;


							//-------------------------------------------------------------------------------------//



							//-------------------Creación de estructura Rotulos--------------------------//
							aux_rotulo_correr=cursor_rotulo-1;
							IFSPACE_rotulos(&rotulo_text,cursor_rotulo,&end_index_rotulos,block_rotulos);
							while (aux_rotulo_correr >= lim_inf_rotulo) //Hace los corrimientos necesarios para hacer lugar al rotulo correspondiente.
							{
							  rotulo_text[aux_rotulo_correr+1]= rotulo_text[aux_rotulo_correr];

							  aux_rotulo_correr-=1;
							}
							cursor_rotulo+=1; //corremos el cursor un lugar mas ya que en la iteraciòn anterior se hizo un corrimiento correspondiente
							loadRotulo (&rotulo,sizeB,f_rotulo,sa_anterior,lcp,final_archivo); //Cargamos el rotulo correspondiente al sufijo anterior
							rotulo_text[lim_inf_rotulo]=rotulo; //agregamos el rotulo correspondiente en el lugar luego del desplazamiento
							loadRotulo (&rotulo,sizeB,f_rotulo,sa,lcp,final_archivo);
							IFSPACE_rotulos(&rotulo_text,cursor_rotulo,&end_index_rotulos,block_rotulos);
							rotulo_text[cursor_rotulo]=rotulo;
							cursor_rotulo+=1;
							lcp_anterior=lcp;
							sa_anterior=sa;
							
							//-------------------------------------------------------------------------------------//
								
						}
						else
						{
							//-------------------Creación de estructura Rotulos--------------------------//
							 lim_inf_rotulo-=1;

							

							//-------------------------------------------------------------------------------------//
						

							cant_bits=pila[stope].cant_bit;
							IFSPACE(&pila[stope_anterior].ptr_arr_parent, pila[stope_anterior].cant_bit,pila[stope].cant_bit+2, &pila[stope_anterior].end_index_ptr_arr_parent, blk_arr_parent);
							agregar_un_bits(pila[stope_anterior].ptr_arr_parent,pila[stope_anterior].cant_bit);
							nro_nodos_raiz+=1;//Para estructura posiciones de cierre hijos raiz

							pila[stope_anterior].cant_bit +=1;	
							CopyBits(pila[stope_anterior].ptr_arr_parent, pila[stope_anterior].cant_bit, pila[stope].ptr_arr_parent, 0, pila[stope].cant_bit);
							pila[stope_anterior].cant_bit=pila[stope_anterior].cant_bit+pila[stope].cant_bit+1;							
							//Pop(pila);
							nro_nodos_nivel=pila[stope].nro_nodos;
							LIBERAR(pila[stope].ptr_arr_parent);
							Pop();

							//-------------------Creación de estructura Saltos--------------------------//

							/*if (!(pila[stope].nro_nodos))//Si es cero entra
								lim_inf_salto-=1;
							else
							{
								lim_inf_salto=cursor_saltos-1;//Le damos a lim_inf_salto la última posicion de la estructura saltos que contiene un valor
								lim_inf_salto-=pila[stope].nro_nodos+nro_nodos_nivel+1;//curso_h lleva el indice de la estructura cant_hijos y debe retroceder tantos lugares como nodo no hojas tenga en este nivel
							}*/

							//-------------------------------------------------------------------------------------//  
							  
							
							//-------------------Creación de estructura Cantidad de hijos--------------------------//
							if (!(pila[stope].nro_nodos))//Si es cero entra
								cursor_h-=1;
							else
							{
								cursor_h=cursor_h_ultimo-1;
							    cursor_h-=pila[stope].nro_nodos+nro_nodos_nivel+1;//curso_h lleva el indice de la estructura cant_hijos y debe retroceder tantos lugares como nodo no hojas tenga en este nivel
							}
							pila[stope].nro_nodos+=nro_nodos_nivel+1;//sumamos uno al número de nodos en este nivel, lo hacemos cada vez que bajamos un nivel en la pila...
																	//nro_nodos_nivel contiene el nro de nodos que existen en el nivel superior al que nos encontramos, los cuales ahora pasan a formar parte del nivel actual es por eso que los sumamos en este nivel
							//-------------------------------------------------------------------------------------//  
							  
							

							if (pila[stope].elem == lcp) //VER QUE PASA CON EOF fin de archivo en este caso
							{
								cant_bits= pila[stope].cant_bit;
								IFSPACE(&pila[stope].ptr_arr_parent,cant_bits,2,&pila[stope].end_index_ptr_arr_parent, blk_arr_parent);
								asignar_hoja(pila[stope].ptr_arr_parent,pila[stope].cant_bit);
								pila[stope].cant_bit+=2;

								


								
								
								//-------------------Creación de estructura Cantidad de hijos y estructura rotulos--------------------------//
								if(lcp == 0){
								//-----------------Estructura posiciones de cierre hijos raiz--------//
									IFSPACE_struc_ulong(&pos_term_raiz,cursor_pos_raiz,&end_pos_raiz, block_pos_raiz);
									pos_term_raiz[cursor_pos_raiz]=cant_bits-1; //Asignamos posición de cierre
									cursor_pos_raiz+=1;
									pos_term_raiz[cursor_pos_raiz]=nro_nodos_raiz;//Asignamos cantidad de Nodos en el subárbol
									cursor_pos_raiz+=1;
									pos_term_raiz[cursor_pos_raiz]=nro_hojas_raiz;//Asignamos cantidad de Hojas en el subárbol
									cursor_pos_raiz+=1;
									nro_nodos_raiz=0;
									nro_hojas_raiz=0;
								 //-----------------FIN Estructura posiciones de cierre hijos raiz--------//

								 	cant_hijos[cursor_h_inicio]+=1;
							    }
								else
								  	cant_hijos[cursor_h]+=1;



								//-----------------Estructura posiciones de cierre hijos raiz--------//
									nro_nodos_raiz+=1;
									nro_hojas_raiz+=1;
								//-----------------FIN Estructura posiciones de cierre hijos raiz--------//
								loadRotulo (&rotulo,sizeB,f_rotulo,sa,lcp,final_archivo); //Cargamos el rotulo correspondiente al sufijo anterior
								IFSPACE_rotulos(&rotulo_text,cursor_rotulo,&end_index_rotulos,block_rotulos);
								rotulo_text[cursor_rotulo]=rotulo;
								cursor_rotulo+=1;
								lcp_anterior=lcp;
								sa_anterior=sa;
								
									
								//-------------------------------------------------------------------------------------//  
							}
						}
					}
				}
			}
		}
	}

while (pila[stope].elem > 0)
{						
	stope_anterior=stope-1;
	IFSPACE(&pila[stope_anterior].ptr_arr_parent, pila[stope_anterior].cant_bit,pila[stope].cant_bit+2,&pila[stope_anterior].end_index_ptr_arr_parent, blk_arr_parent);
	agregar_un_bits(pila[stope_anterior].ptr_arr_parent,pila[stope_anterior].cant_bit);
	nro_nodos_raiz+=1;//Para estructura posiciones de cierre hijos raiz
	pila[stope_anterior].cant_bit +=1;	
	CopyBits(pila[stope_anterior].ptr_arr_parent, pila[stope_anterior].cant_bit, pila[stope].ptr_arr_parent, 0, pila[stope].cant_bit);
	pila[stope_anterior].cant_bit =pila[stope_anterior].cant_bit+pila[stope].cant_bit+1;
	//Pop(pila);
	LIBERAR(pila[stope].ptr_arr_parent);
	Pop();
	
}
IFSPACE(&pila[stope].ptr_arr_parent, pila[stope].cant_bit,1,&pila[stope].end_index_ptr_arr_parent, blk_arr_parent);
pila[stope].cant_bit+=1;

(*index_common).ptr_arr_parent=pila[stope].ptr_arr_parent;
(*index_common).cant_bit=pila[stope].cant_bit;
(*index_common).rotulo_text=rotulo_text;
(*index_common).saltos=saltos;
(*index_common).cant_hijos=cant_hijos;
(*index_common).SA=SA;

*index=&(*index_common);


uchartoulong(pila[stope].ptr_arr_parent, &bitmap_ulong, pila[stope].cant_bit);

//bitmap=(ulong *)pila[stope].ptr_arr_parent;

initExcBitmapTables(bitmap_ulong, pila[stope].cant_bit);//Llamamos a la función que crea la tabla de excesos minimos del bitmap que mantiene la estructura del árbol.

/*
///////////////////////-----------------------------Probar Ranck orden constante//////////////////7--------------
byte *bitmap_byte;
bitmap_byte=pila[stope].ptr_arr_parent;

ulong *bitmap;
bitmap=(ulong *)pila[stope].ptr_arr_parent;

ulong num_ranck=0,nrobit=0, cantidad_bit_n, num_ranck_static=0;
cantidad_bit_n=pila[stope].cant_bit;
ulong *Rs,*Rb;
static double Load_time_ranck;
static double Load_time_ranck_constante;

BuildRank(&Rs,&Rb,bitmap,cantidad_bit_n);

Load_time_ranck = getTime ();
for (nrobit=1; nrobit<10000; nrobit++){
    num_ranck=Rank(bitmap_byte,nrobit)+ num_ranck;
}
Load_time_ranck = getTime () - Load_time_ranck;

fprintf (stderr, "Ranck Nuestro = %lu\t\t Tiempo=%.4f \n", num_ranck, Load_time_ranck);

Load_time_ranck_constante = getTime ();
for (nrobit=1; nrobit<4000000; nrobit++){
	num_ranck_static=Rank_Estatico(nrobit,Rs,Rb,bitmap)+num_ranck_static;
}
Load_time_ranck_constante = getTime () - Load_time_ranck_constante;
fprintf (stderr, "Ranck Tiempo constante= %lu\t\t Tiempo=%.4f \n", num_ranck_static, Load_time_ranck_constante);

///////////////////////-----------------------------FIN Probar Ranck orden constante//////////////////7--------------
*/




/*double size_index=0, size_index_total=0, tam_representacion_parentesis=0;
int tam_text=10; //Valores para imprimir en la salida

tam_representacion_parentesis=sizeof(byte)*(CEIL((*index_common).cant_bit));//(Tam. estructura Representación Parentesis)--Calculo la cantidad de bytes de a cuerdo a la cantidad de bit en la estructura con la funcion CEIL y lo mult al tamaño de un byte
//size_index=size_index+sizeof(byte);
size_index=size_index+tam_representacion_parentesis;
size_index=size_index+sizeof(elemSaltos)*cursor_saltos;
size_index=size_index+sizeof(elemCant_Hijos)*cursor_h_ultimo;
size_index=size_index+sizeof(elemRotulo)*cursor_rotulo;
size_index=size_index+sizeof(elemSaltos)*cursor_SA; //Tamaño de la estructura SA contiene las direcciones de las palabras en el archivo
size_index=size_index+sizeof(ulong)*(DIVb(pila[stope].cant_bit)+1);*/



double size_index=0, size_index_total=0, tam_representacion_parentesis=0, tam_saltos=0, tam_Cant_hijos=0, tam_rotulos=0, tam_SA=0, tam_MinExcessBitmap=0,tam_MinExcessBitmap_RS=0, tam_pos_term_raiz=0;
//int tam_text=size_text; //Valores para imprimir en la salida

ulong ulong_size_bitmap=0, Rs_size_bitmap=0;
ulong_size_bitmap=DIVb(pila[stope].cant_bit)+1;  //Ver el tema si modificamos b #define DIVS(a) ((a)>> 5)
Rs_size_bitmap=DIVs(pila[stope].cant_bit)+1;


tam_representacion_parentesis=sizeof(byte)*(CEIL((*index_common).cant_bit));//(Tam. estructura Representación Parentesis)--Calculo la cantidad de bytes de a cuerdo a la cantidad de bit en la estructura con la funcion CEIL y lo mult al tamaño de un byte
tam_saltos=sizeof(elemSaltos)*cursor_saltos;
tam_Cant_hijos=sizeof(elemCant_Hijos)*cursor_h_ultimo;
tam_rotulos=sizeof(elemRotulo)*cursor_rotulo;
tam_SA=sizeof(elemSaltos)*cursor_SA;//Tamaño de la estructura SA contiene las direcciones de las palabras en el archivo
//tam_exe_min_32b=sizeof(ulong)*(DIVb(pila[stope].cant_bit)+1);

tam_MinExcessBitmap=sizeof(char)*ulong_size_bitmap;//Tamaño de estructura que mantiene exceso minimos para longitud b=32
tam_MinExcessBitmap_RS=sizeof(char)*Rs_size_bitmap;//Tamaño de estructura que mantiene exceso minimos para longitud b*s donde b=32 y s=4; b*s=128

tam_pos_term_raiz=sizeof(ulong)*cursor_pos_raiz;//Tamaño de estructura que mantiene las posiciones de cierre de cada hijo de la raiz más la cantidad de nodos y hojas de cada hijo.

size_index=tam_representacion_parentesis + tam_saltos + tam_Cant_hijos + tam_rotulos + tam_SA+ tam_MinExcessBitmap + tam_MinExcessBitmap_RS + tam_pos_term_raiz;


//----------------------Tamaño del Indice------------//
if (Verbose){
	//fprintf (f_result_size, "Size_text\tT_rp\tT_saltos\tT_grado\tT_rotulos\tT_SA\tT_MinExc\tT_MinExcRS\tT_Term_Raiz\tT_Index\tTiemp_Creacion\n");
	fprintf (f_result_size, "%s\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t", size_text, tam_representacion_parentesis, tam_saltos, tam_Cant_hijos, tam_rotulos, tam_SA, tam_MinExcessBitmap, tam_MinExcessBitmap_RS, tam_pos_term_raiz, size_index);
}
//----------------------FIN Tamaño del Indice------------//





fclose(f_rotulo);
fclose(f_lcp);
fclose(f_sa);
return 0;
}
