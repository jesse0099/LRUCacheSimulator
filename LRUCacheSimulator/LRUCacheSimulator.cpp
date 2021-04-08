// LRUCacheSimulator.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include "pch.h"
#include <windows.h> 
#include <iostream> 
#include <thread>  //Manejo de hilos
#include <mutex>  //Manejo de hilos



#define color_green      10
#define color_white		 15


using namespace std;

mutex mtx;


#pragma region Definicion de estructuras

#pragma region Paginas
struct pagina {
	int nPag;
	int idProceso;
	float memoriaRequerida;
	bool  enUso;
	bool  enEspera;
	pagina * sig;
};
//Tipo representando una lista de Paginas
typedef pagina * Paginas;
#pragma endregion

#pragma region Procesos
struct proceso {
	string nombre;
	int id;
	float memoriaRequerida;
	proceso * sig;
	pagina  * pags;
};

/*	
	Estructura de datos representando una lista de Procesos.
	Cada Proceso posee asociada una lista de 
	páginas de tamaño fijo.
*/
typedef proceso * Procesos;
#pragma endregion

#pragma region Marcos de memoria
struct marcoDeMemoria {
	float espacioInicial;
	float espacioDisponible;
	bool enUso;
	int idMarco;
	pagina * paginaAlojada;
	marcoDeMemoria * sig;
};
/*	
	Estructura de datos representando los Marcos de Memoria (Frames).
	Cada marco posee una referencia a la pagina que 
	actualmente lo ocupa, de estar disponible,
	esta referencia posee valor NULL.
*/
typedef marcoDeMemoria * MarcosDeMemoria;
#pragma endregion

#pragma region LRUCache
struct lruCache {
	int proceso;
	int pagina;
	lruCache * sig;
};
/*
	Estructura de datos para el manejo del algoritmo LRU.
*/
typedef lruCache * LRUCache;
#pragma endregion

#pragma endregion


#pragma region Memoria compartida 
MarcosDeMemoria mrme = NULL;
LRUCache cache = NULL;
#pragma endregion



#pragma region Creacion de nodos
/*
	Métodos para la creación de instancias de
	las estructuras definidas para el simulador.
*/

marcoDeMemoria * crearMarcoMemoria(float espacioInicial,int idMarco) {
	marcoDeMemoria * nuevo = new(struct marcoDeMemoria);
	nuevo->enUso = false;
	nuevo->espacioInicial = espacioInicial;
	nuevo->espacioDisponible = nuevo->espacioInicial;
	nuevo->paginaAlojada = NULL;
	nuevo->idMarco = idMarco;
	nuevo->sig = NULL;
	return nuevo;
}

lruCache * crearLRUCache(int proceso,int pagina) {
	lruCache * nuevo = new (struct lruCache);
	nuevo->pagina = pagina;
	nuevo->proceso = proceso;
	nuevo->sig = NULL;
	return nuevo;
}

pagina * crearPagina(float memoriaReq,int nPag,int idProceso) {
	pagina * nuevo = new (struct pagina);
	nuevo->idProceso = idProceso;
	nuevo->enEspera = false;
	nuevo->enUso = false;
	nuevo->memoriaRequerida = memoriaReq;
	nuevo->nPag = nPag;
	nuevo->sig = NULL;
	return nuevo;
}

proceso * crearProceso(string nombre,float memoriaReq,int id) {
	proceso * nuevo = new (struct proceso);
	nuevo->memoriaRequerida = memoriaReq;
	nuevo->id = id;
	nuevo->nombre = nombre;
	nuevo->sig = NULL;
	nuevo->pags = NULL;
	return nuevo;
}

#pragma endregion

#pragma region Utilidades

/*
	Cambio de foreground para la salida en consola
*/
void setColor(int value) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), value);
}

/*
	Cantidad de procesos en una lista.
*/
int numeroProcesos(Procesos cab) {
	if (cab == NULL)
		return 0;
	else {
		int count = 0;
		Procesos aux = cab;
		while (aux != NULL) {
			count++;
			aux = aux->sig;
		}
		return count;
	}
}

/*
	Cantidad de páginas en cada proceso.
*/
int numeroPaginasProceso(Paginas cab) {
	if (cab == NULL) {
		return  0;
	}
	else {
		int count = 0;
		Paginas aux = cab;
		while (aux != NULL) {
			count++;
			aux = aux->sig;
		}
		return count;
	}
}


/*
	Los marcos de memoria son una estructura de datos compartida,
	al igual que la LRU, por lo que su acceso debería estar
	protegido de condiciones de carrera.
*/


/*
	Memoria inicial total del marco.
*/
float memoriaToTalMarco(MarcosDeMemoria cab) {
	if (cab == NULL) {
		return 0;
	}
	else {
		float totalM = 0.0;
		MarcosDeMemoria aux = cab;
		while (aux!=NULL)
		{
			totalM = totalM + aux->espacioInicial;
			aux = aux->sig;
		}
		return totalM;
	}

}

/*
	Espacio disponible total de los marcos de memoria fisica.
*/
float memoriaDispoMarco(MarcosDeMemoria cab) {
	if (cab == NULL) {
		return  0;
	}
	else {
		float totalDisp = 0.0;
		MarcosDeMemoria aux = cab;
		while (aux!=NULL) {
			totalDisp = totalDisp + aux->espacioDisponible;
			aux = aux->sig;
		}
		return totalDisp;
	}
}

/*
	Total de la memoria solicitada por las 
	paginas de un proceso.

	Se solicita el id del proceso como parametro
	para asignar el valor al criterio
	de busqueda.
*/
float memoriaSolicitadaProc(Procesos cab,int idProc) {
	Procesos aux = cab;
	float solic = 0.0;
	while (aux!=NULL) {
		if (aux->id == idProc) {
			Paginas auxP = aux->pags;
			while (auxP!=NULL) {
				if (!auxP->enEspera) {
					solic = solic + auxP->memoriaRequerida;
				}
				auxP = auxP->sig;
			}
		}
		aux = aux->sig;
	}
	return solic;
}


/*
	Revisa si el proceso y la pagina solicitados mediante sus correspondientes
	identificadores  estan alojados en algun marco de memoria.
*/
bool existsLRUCache(LRUCache cab,int pag,int proc) {
	if (cab == NULL) {
		return false;
	}
	else {
		LRUCache aux = cab;
		while (aux!=NULL) {
			if (aux->pagina == pag && aux->proceso == proc) {

				return true;
			}
			aux = aux->sig;
		}
		return false;
	}
}

/*
	Cantidad de elementos en la lista para simular
	LRU.
*/
int cacheSize(LRUCache cab) {
	if (cab == NULL) {
		return 0;
	}
	else {
		int count = 0;
		LRUCache aux = cab;
		while (aux!=NULL) {
			count++;
			aux = aux->sig;
		}
		return count;
	}
}
#pragma endregion

#pragma region Eliminaciones

/*
	Elimina un elemento de una lista de strcuturas LRUCache
*/
void deleteFromCache(LRUCache cache, int proc, int pag) {


	if (cache == NULL) {
		
	}
	else {
		//Buscar 
		LRUCache aux = cache;
		LRUCache anterior = NULL;
		if (aux == NULL) {

		}
		else {
			while ((aux!=NULL) && !(aux->pagina == pag && aux->proceso == proc)) {
				anterior = aux;
				aux = aux->sig;
			}
			if (aux == NULL) {
				//No se ha encontrado el elemento
			}
			else if (anterior == NULL) {
				//El elemento a eliminar esta en la cabeza
				cache = cache->sig;
				delete(aux);
			}
			else {
				//El elemento a eliminar ocupa posiciones distintas a la cabeza y la cola
				anterior->sig = aux->sig;
				aux = NULL;
				delete(aux);
			}
		}
	}


}

/*
	Pop inverso == Pull 
	Pull = elimina el elemento mas antiguo de la cola LRU, es
	decir, el agregado primero.
*/
void popInverso(LRUCache &cab,LRUCache &popped) {
	

	if (cab == NULL) {

	}
	else {
		LRUCache aux = cab;
		LRUCache ant = NULL;
		while (aux->sig != NULL) {
			ant = aux;
			aux = aux->sig;
		}
		popped = ant->sig;
		ant->sig = NULL;
		delete(ant->sig);
		
	}

	
}

/*
	
	libera el elemento indicado mediante los identificadores de proceso y pagina 
	de la cola Marco de memoria, además, reinicia las siguientes propiedades:
		1-enUso
		2-espacioDisponible
		3-paginaAlojada
*/
void liberarMarco(MarcosDeMemoria &cab,int proc,int pag) {
	

	if (cab == NULL) {

	}
	else {
		MarcosDeMemoria aux = cab;
		while (aux!=NULL) {
			if (aux->paginaAlojada->idProceso == proc && aux->paginaAlojada->nPag == pag) {
				aux->paginaAlojada = NULL;
				aux->espacioDisponible = aux->espacioInicial;
				aux->enUso = false;
				break;
			}
			aux = aux->sig;
		}
	}

	
}
#pragma endregion

#pragma region Inserciones
/*
	Insercion de un proceso a la estructura Procesos
*/
void insertProceso(Procesos &cab,proceso *  nuevo) {
	if (cab == NULL) {
		cab = nuevo;
	}
	else {
		Procesos aux = cab;
		while (aux->sig != NULL)
			aux = aux->sig;
		aux->sig = nuevo;
	}
}

/*
	Insercion de un elemento al final de la  estructua Paginas, en
	la estructura Procesos, en el espacio correspondiente 
	al identificador solicitado como parametro.
*/
void insertarPagina(Procesos &cab,pagina * nuevo,int idProceso) {
	if (cab->pags == NULL) {
		cab->pags = nuevo;
	}
	else {
		Procesos auxP = cab;
		while (auxP != NULL) {
			if (auxP->id == idProceso) {
				Paginas aux = auxP->pags;
				if (aux == NULL)
					auxP->pags = nuevo;
				else {
					while (aux->sig != NULL) {
						aux = aux->sig;
					}
					aux->sig = nuevo;
				}
				break;
			}
			auxP = auxP->sig;
		}
	}
}


/*
	Inserta un elemento al final de la estructura MarcosDeMemoria.
*/
void insertarMarcoDeMemoria(MarcosDeMemoria &cab,marcoDeMemoria * nuevo) {
	mtx.lock();
	if (cab == NULL) {
		cab = nuevo;
	}
	else {
		MarcosDeMemoria  aux = cab;
		while (aux->sig!=NULL) {
			aux  = aux->sig;
		}
		aux->sig = nuevo;
	}
	mtx.unlock();
}


/*
	Inserta un elemento al final de la estructura LRUCache.
*/
void insertarLru(LRUCache &cabLRU,lruCache * nuevo) {
	
	if (cabLRU==NULL) {
		cabLRU = nuevo;
	}
	else {
		//Chequear si existe la pagina del proceso indicado en la cola, y eliminar en dicho caso
		if (existsLRUCache(cabLRU, nuevo->pagina, nuevo->proceso)) {
			//La pagina ya se encuentra encolada
			//Eliminando la ultima referencia, es decir, la menos recientemente utilizada
			deleteFromCache(cabLRU,nuevo->proceso,nuevo->pagina);
			//Apilando la nueva referencia, de forma que se actualice su estado de uso a uno mas reciente
			nuevo->sig = cabLRU;
			cabLRU = nuevo;
		}
		else {
			nuevo->sig = cabLRU;
			cabLRU = nuevo;
		}
	}
	
}
#pragma endregion

#pragma region Listados
/*
	En la buena teoria, no son metodos que deba proteger, 
	pues las lecturas y escrituras van a solicitar la pro
	-piedad de un Mutex.
*/
void listarProcesos(Procesos cab) {
	if (cab==NULL) {
		setColor(4);
		cout << "---No hay procesos disponibles---"<<endl;
	}
	else {
		Procesos aux = cab;
		while (aux!=NULL) {
			setColor(7);
			cout << "Proceso : ";
			setColor(9);
			cout << aux->nombre.c_str() <<endl;
			cout << endl;
			Paginas  auxP = aux->pags;
			if (auxP==NULL) {
				setColor(4);
				cout << "		-No hay paginas disponibles-" << endl;
			}
			else {
				setColor(3);
				while (auxP!=NULL) {
					string enUso = "No";
					string enEspera = "No";
					if (auxP->enUso)
						enUso = "Si";
					if (auxP->enEspera)
						enEspera = "Si";

					cout <<"		Numero de pagina   : "<< auxP->nPag << endl;
					cout <<"		Memoria requerida  : "<< auxP->memoriaRequerida << endl;
					cout <<"		En uso             : "<< enUso.c_str() << endl;
					cout <<"		En espera          : "<< enEspera.c_str() << endl;
					cout << endl;
					setColor(1);
					cout <<"		_____________________________" <<endl;
					cout << endl;
					auxP = auxP->sig;
					setColor(3);
				}
			}
			aux = aux->sig;
		}
	}

}

void listarMarcos(MarcosDeMemoria cab) {
	if (cab == NULL) {
		setColor(4);
		cout << "--No hay marcos de memoria disponibles--" << endl;
	}
	else {
		MarcosDeMemoria aux = cab;
		cout << "		Marcos de memoria" << endl;
		setColor(3);
		while (aux!=NULL) {

			string estado = "Libre";
			if (aux->enUso)
				estado = "En uso";

			cout << "ID Marco		  : " << aux->idMarco << endl;
			cout << "Capacidad almacenamiento  : " << aux->espacioInicial<<" MB"<<endl;
			cout << "Almacenamiento disponible : " << aux->espacioDisponible<<" MB"<<endl;
			cout << "Estado			  : " << estado.c_str() << endl;
			if (aux->paginaAlojada != NULL) {
				cout << endl;
				cout << endl;
				cout << "			Alojando            : ";
				setColor(2);
				cout << "	Proceso -> " << aux->paginaAlojada->idProceso << " Pagina- > " << aux->paginaAlojada->nPag<<endl;
				setColor(3);
				cout << "			Memoria Requerida   :" << aux->paginaAlojada->memoriaRequerida <<" MB"<< endl;
			}
			setColor(1);
			cout << "_________________________________" << endl;
			setColor(3);
			aux = aux->sig;
		}
	}
}
#pragma endregion

#pragma region Gestion de procesos

void asignarMarco(Paginas auxPa) {
	/*
		Todas las operaciones de lectura y escritura sobre memoria compartida 
		suceden bajo el dominio de un Mutex
	*/
	mtx.lock();

	MarcosDeMemoria auxMarc = mrme;

	int memoriaTotal = memoriaToTalMarco(auxMarc);

	if (auxMarc == NULL) {

	}
	else {


		/*
			Aplicar LRU: Mientras el numero de paginas en la estructura LRU
			sea mayor o igual al numero de marcos totales (escenario con necesidad de reemplazo),
			se liberara una página del marco de memoria, en este caso, la  menos recientemente utilizada
		*/
		int cacheS = cacheSize(cache);
		while (cacheS >= (memoriaTotal / 64)) {
			//Pop inverso a la pila y al marco de memoria contenedor
			LRUCache popped = NULL;
			popInverso(cache, popped);

			liberarMarco(mrme, popped->proceso, popped->pagina);

			cacheS = cacheSize(cache);
		}
		/*Iterar sobre el los marcos de memoria*/
		while (auxMarc != NULL) {
			/*Si el marco no se encuentra en uso y la pagina no se encuentra asignada*/
			if (!auxMarc->enUso && !auxPa->enEspera) {
				//El marco esta libre y se le asigna una copia de la pagina a modo de referencia
				auxMarc->paginaAlojada = NULL;
				auxMarc->paginaAlojada = crearPagina(auxPa->memoriaRequerida, auxPa->nPag, auxPa->idProceso);
				auxMarc->enUso = true;
				//Se pueden colocar mas paginas ? 
				insertarLru(cache, crearLRUCache(auxPa->idProceso, auxPa->nPag));
				/*Aca se asume un escenario sin fragmentacion interna, todos los marcos estaran completamente ocupados*/
				auxMarc->espacioDisponible = auxMarc->espacioInicial - auxPa->memoriaRequerida;
				break;
			}
			auxMarc = auxMarc->sig;
		}


	}

	mtx.unlock();
}

void abrirProceso(Procesos procCab,int idProceso) {

	cout << "ID Hilo ->  " << std::this_thread::get_id() <<" Proceso ->"<<idProceso<< "|" <<endl;
	cout << endl;


	int memoriaTotal = memoriaToTalMarco(mrme);

	Procesos aux = procCab;
	while (aux!=NULL) {
		//Selección del proceso que se desea abrir
		if (aux->id == idProceso) {
				//Iterar sobre las paginas del proceso solicitado para la apertura
				Paginas auxPa = aux->pags;
				while (auxPa!=NULL) {

					
					asignarMarco(auxPa);
					auxPa = auxPa->sig;


				}
			
		}
		aux = aux->sig;
	}



}

void aperturaConcurrente(Procesos procs) {
	thread threads[3];
	/*
		Apertura concurrente de procesos
	*/

	for (size_t i = 0; i < 3; i++)
		threads[i] = thread(abrirProceso, procs, i + 1);

	/*Esperando a la finalizacion de la ejecucion de los hilos*/
	for (size_t i = 0; i < 3; i++)
		threads[i].join();
} 

void aperturaNoConcurrente(Procesos procs) {
	/*
		Apertura no concurrente de procesos
	*/

	for (size_t i = 0; i < 3; i++)
		abrirProceso(procs,i+1);

}

#pragma endregion



int main()
{
	/*Memoria no compartida*/
	Procesos procs = NULL;

	
	/*La carga inicial se realiza en un solo Hilo*/

	#pragma region Carga Inicial
	//Insertar procesos
	insertProceso(procs, crearProceso("VisualStudio.exe", 256, 1));
	insertProceso(procs, crearProceso("Spotify.exe", 384, 2));
	insertProceso(procs, crearProceso("Google Chrome.exe", 512, 3));
	insertProceso(procs, crearProceso("NetBeans.exe", 320, 4));
	//Insertar paginas por procesos
	for (int i = 0; i < (256 / 64); i++)
		insertarPagina(procs, crearPagina(64, (i + 1),1), 1);
	//Insertar paginas por procesos
	for (int i = 0; i < (384 / 64); i++)
		insertarPagina(procs, crearPagina(64, (i + 1),2), 2);
	for (int i = 0; i < (512 / 64); i++)
		insertarPagina(procs, crearPagina(64, (i + 1),3), 3);
	for (int i = 0; i < (320 / 64); i++)
		insertarPagina(procs, crearPagina(64, (i + 1),4), 4);
	//Cargando marcos de memoria
	for (int i = 0; i < (1024 / 64); i++)
		insertarMarcoDeMemoria(mrme,crearMarcoMemoria(64,(i+1)));
#pragma endregion

	aperturaConcurrente(procs);
	//aperturaNoConcurrente(procs);
	


	listarMarcos(mrme);
	system("Pause");

}

