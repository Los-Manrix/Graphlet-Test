#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// Define limits based on your large dataset
#define MAX_VERTICES 100005
#define MAX_EDGES    25000005  // Total directed edges (multiply by 2 for bidirectional)
#define MAX_NEI 50000
int num_nodes,num_edges;

// Array-based Adjacency List Structure
struct node;
typedef struct node node;

struct node //nodes
{
  int id;
  int head;
  unsigned grade; 
  short color;
  unsigned iter;
  short tsn;
  short inq;
  int pos;
};

struct toSort;
typedef struct toSort toSort;

struct toSort //nodes
{
  int id;
  unsigned grade; 
};

// Función de comparación segura para tipos unsigned (Mayor a Menor)
int compararPorGradeDescendente(const void *a, const void *b) {
    // 1. Convertir los punteros genéricos al tipo de tu estructura
    const struct toSort *nodoA = (const struct toSort *)a;
    const struct toSort *nodoB = (const struct toSort *)b;

    // 2. Lógica para orden descendente (Mayor a Menor)
    if (nodoA->grade < nodoB->grade) return 1;  // Si A es menor, va después
    if (nodoA->grade > nodoB->grade) return -1; // Si A es mayor, va antes
    return 0;                                   // Si son iguales, se quedan igual
}

int compararPorGradeAscendente(const void *a, const void *b) {
    // 1. Convertir los punteros genéricos al tipo de tu estructura
    const struct toSort *nodoA = (const struct toSort *)a;
    const struct toSort *nodoB = (const struct toSort *)b;

    // 2. Lógica para orden descendente (Mayor a Menor)
    if (nodoA->grade > nodoB->grade) return 1;  // Si A es menor, va después
    if (nodoA->grade < nodoB->grade) return -1; // Si A es mayor, va antes
    return 0;                                   // Si son iguales, se quedan igual
}

node nodes[MAX_VERTICES];
toSort order[MAX_VERTICES];
int pos[MAX_VERTICES];
int toEdge[MAX_EDGES];     // Stores the destination vertex ID for each edge index
int toEdgeType[MAX_EDGES];    
int nextEdge[MAX_EDGES];   // Links to the previous edge index for the same source vertex
int edgeCount = 0;         // Global counter to track total edges added
int G[MAX_EDGES]; //node succ
int Gtype[MAX_EDGES]; //type
int Gini[MAX_VERTICES]; //initial position for nodes
int Ggrade[MAX_VERTICES]; //number of nodes
int Gp[MAX_VERTICES]; //pater of a node 
int Gtuv[MAX_VERTICES]; //nodes in queue 1:true
int Gposfree = 0;// // Global counter to add new nodes

int edge[MAX_EDGES][3]; //source, target, type
int Nodes[MAX_VERTICES][5];//n1,n2,n3,pos,grade

// 1. Initialize arrays
void initGraph(int vertices) {
    int i;
	for (i = 0; i < vertices; i++) {
        nodes[i].id = i;
        nodes[i].head = -1; // -1 means no edges connected yet
        nodes[i].grade = 0;
        nodes[i].color = 1;
        nodes[i].iter = 0;
        nodes[i].tsn = 0;
        nodes[i].inq = 0;
        nodes[i].pos = i;
        
		order[i].id = i;
        order[i].grade = 0;
        
        Gini[i] = -1;
        Gp[i] = -1;
		int j;
        for (j=0; j<5; j++) 
			Nodes[i][j] = 0;
    }
    edgeCount = 0;
}


void printNeighbors(int u) {
}
long long int truenum_edges = 0;
void ReadGraph(const char* filename) {
	FILE* f;
	int i, ori, dest, dist, t;
	f = fopen(filename, "r");
	if (f == NULL) 	{
		printf("Cannot open file %s.\n", filename);
	//	exit(1);
	}
	fscanf(f, "%d %d", &num_nodes, &num_edges);
	fscanf(f, "\n");
//	printf("%d %d %d\n", num_gnodes, num_arcs,INT_MAX);
//	getchar();
    initGraph(num_nodes);
	for (i = 0; i < num_edges; i++) {
		fscanf(f, "%d %d %d\n", &ori, &dest, &t);
		//addDirectedEdge(ori-1, dest-1, t);
		if (ori != dest){
			edge[truenum_edges][0] = ori-1; //source
			edge[truenum_edges][1] = dest-1; //target
			edge[truenum_edges][2] = t; //type

			if (t == 1)
				Nodes[ori-1][0]++;//n1
			else
				if (t == 2)		
					Nodes[ori-1][1]++;//n2
				else
					Nodes[ori-1][2]++;//n3,grade
			order[ori-1].id = ori-1;
        	order[ori-1].grade++;
        	Nodes[ori-1][4]++;//grade
			truenum_edges++;
		}

	//	printf("%d %d %d\n", ori-1, dest-1, t);
	}
	fclose(f);
}


// 2. Add a directed edge from 'u' to 'v'
void addDirectedEdge(int u, int v, int t) {
	if (Gini[u] == -1){
		Gini[u] = Gposfree;
		Gposfree += Nodes[u][4];
		Ggrade[u] = 0;
	} 
	int pos = Gini[u] + Ggrade[u]; 
	G[pos] = v;
	Gtype[pos] = t;
	Ggrade[u]++;	
}


void Make_Graph(){
	int i,u,v,t;
	for (i = 0; i < truenum_edges; i++) {
		u = edge[i][0];
		v = edge[i][1];
		t = edge[i][2];
		int posu = Nodes[u][3];
		int posv = Nodes[v][3];
		if (posv > posu){
			addDirectedEdge(u,v,t);
			edgeCount++;
		}
	}
} 

//long long int type[4][4][4];
long long int type[4][4][4] __attribute__((aligned(64)));
void initialize_type(){
	int i,j,k;
	for(i = 0;i < 4;i++)
		for(j = 0;j < 4;j++)
			for(k = 0;k < 4;k++)
				type[i][j][k] = 0;
}


void print_types(){
	int i,j,k;
	for(i = 0;i < 4;i++)
		for(j = 0;j < 4;j++)
			for(k = 0;k < 4;k++)
				printf("[%d][%d][%d] : %lld\n",i,j,k,type[i][j][k]);
}

long long comb2(int n) {
    if (n < 2) return 0;
    return ((long long)n * (n - 1)) >> 1;
}
long long unsigned touchNodes = 0;
long long unsigned procesedNodes = 0;




void UpdatePos(){
	int i;
	for (i = 0; i < num_nodes; i++) {
		int k = order[i].id;
		Nodes[k][3]= i;
    }
	
}

void printG(){
	int i,k,j;
	for (j = 0; j < num_nodes; j++) {
		k =  order[j].id;
		printf("\n node %d, succs:\n",k);
		for (i = Gini[k];i < Gini[k]+Ggrade[k];i++)
			printf("succ: %d type:%d\n",G[i],Gtype[i]);
		getchar();
	}
}


void printNodes(){
	int i,k;
	printf("edgeCount:%d\n",edgeCount);
	for (k = 0; k < num_nodes; k++) {
		i = order[k].id;
		printf("id: %d n1:%d n2:%d n3:%d pos:%d grade:%d\n",i,Nodes[i][0],Nodes[i][1],Nodes[i][2],Nodes[i][3],Nodes[i][4]);
		getchar();
	}
}
/*
int inv(int t){
	if (t == 1)
		return 2;
	if (t == 2)
		return 1;
	return 3;
}
*/
/*int inv(int t) {
    // Si t es 1 o 2, calcula (3 - t). Si es cualquier otra cosa, devuelve 3.
    return (t == 1 || t == 2) ? (3 - t) : 3;
}*/

int inv(int t) {
    // Tabla indexada para t = 0, 1, 2, ...
    // Se marcan como 'static const' para que resida permanentemente en la memoria rápida
    static const int tabla[] = {3, 2, 1}; 
    
    // Si 't' está fuera de rango (0, 1, 2), devolvemos 3 de forma segura
    return (t >= 0 && t <= 2) ? tabla[t] : 3;
}

void updateTotalGraphlets(int x, int y, int z, int c){
	type[x][y][z] +=c; 
}
long long int graphletID[64], num0, num1, num2;

int increaseg(int n){
/*	switch (n){
		case 0:
			num0++;
			break;
		case 1:
			num1++;
			break;
		default:
			num2++;
	}	*/
	graphletID[n]++;
}

int itoa_custom(long long val, char *buf) {
    char temp[25];
    int i = 0, p = 0;
    
    // Manejo de números negativos
    if (val < 0) {
        buf[p++] = '-';
        val = -val;
    }
    // Caso especial para el cero
    if (val == 0) {
        buf[p++] = '0';
        return p;
    }
    // Extraer dígitos en reversa
    while (val > 0) {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }
    // Invertir los dígitos al buffer final
    while (i > 0) {
        buf[p++] = temp[--i];
    }
    return p; // Retorna cuántos caracteres escribió
}

//long long int*** SearchGraphlets(){
void SearchGraphlets(){
	int i,j,k,u;
	long long int ntriagles = 0;
	struct timeval tstart, tend;
	
	long long int type1[4][4][4] ;

//long long int g1=0,g2=0,g3=0,g4=0,g5=0,g6=0,g7=0,g8=0,g9=0,g10=0,g11=0,g12=0,g13=0;

    gettimeofday(&tstart, NULL);
    
	for(i = 0;i < 4;i++)
		for(j = 0;j < 4;j++)
			for(k = 0;k < 4;k++)
				type1[i][j][k] = 0;

	for (k = 0; k < num_nodes; k++) {
		u =  order[k].id;	
		int n1 = Nodes[u][0];
		int n2 = Nodes[u][1];
		int n3 = Nodes[u][2];
		type1[1][0][1] += comb2(n1);
		type1[2][0][2] += comb2(n2);  
		type1[3][0][3] += comb2(n3); 
		type1[1][0][2] += n1 * n2;
		type1[1][0][3] += n1 * n3;
		type1[2][0][3] += n2 * n3;

		int q[MAX_NEI];
		int nq = 0;
		for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
			int v = G[i];
			Gtuv[v] = Gtype[i]; //type from u to v		
			q[nq++] = v;
			Gp[v] = u;
			touchNodes;
		}
	    for (i = 0; i < nq; i++){
   			int v = q[i];
			for (j = Gini[v];j < Gini[v]+Ggrade[v];j++){
				int w = G[j];
				touchNodes++;
				if (Gp[v] == Gp[w]){ //Triangle detected
					short x = Gtuv[v];
					short y = Gtype[j];
					short z = Gtuv[w];
					short invx = inv(x);

					ntriagles++;
				//	g1++;g2++;g3++;g4++;g5++;g6++;g7++;g8++;g9++;g10++;g11++;g12++;g13++;
					
					type1[x][y][z]++;
					type1[x][0][z]--; //descuento para u
					type1[inv(x)][0][y]--; //descuento para v
					type1[inv(z)][0][inv(y)]--; //descuento para w				
				}
			}
		}
	}
	gettimeofday(&tend, NULL);
    double tiempo_transcurrido = 1.0 * (tend.tv_sec - tstart.tv_sec) + 1.0 * (tend.tv_usec - tstart.tv_usec) / 1000000.0;

	//printf("touchNodes:%llu, ntriagles:%llu t:%f\n",touchNodes,ntriagles,tiempo_transcurrido);
	printf("touchNodes:%llu\n",touchNodes);
//	printf("ntriagles:%llu\n",ntriagles);
//	printf("%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",g1,g2,g3,g4,g5,g6,g7,g8,g9,g10,g11,g12,g13);
/*	for(i = 0;i < 4;i++)
		for(j = 0;j < 4;j++)
			for(k = 0;k < 4;k++)
				printf("[%d][%d][%d] : %lld\n",i,j,k,type1[i][j][k]);*/

	return;
}

void SearchGraphlets_3(int u){
	int i,j,k;
	int q[MAX_NEI];
	int nq = 0;
	long long int ntriagles = 0;

	for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
		int v = G[i];
		Gtuv[v] = Gtype[i]; //type from u to v		
		q[nq++] = v;
		Gp[v] = u;
		touchNodes;
	}
	
    for (i = 0; i < nq; i++){
   		int v = q[i];
		for (j = Gini[v];j < Gini[v]+Ggrade[v];j++){
			int w = G[j];
			//touchNodes++;
			if (Gp[v] == Gp[w]){ //Triangle detected
				int x = Gtuv[v];
				int y = Gtype[j];
				int z = Gtuv[w];
				int invx = inv(x);
				//int invy = inv(y);
				touchNodes++;
				ntriagles++;
				//int pos1 = x+(4*y)+(16*z);  
				//graphletID[x+(4*y)+(16*z)]++;
				//graphletID[x+(16*y)]--;
				//graphletID[invx+(16*y)]--;
				//graphletID[inv(x)+(16*y)]--;
				//graphletID[invx+(16*inv(y))]--;
				/*int pos2 = x+(16*y);
				graphletID[pos2]--;
				int pos3 = inv(x)+(16*y);
				graphletID[pos3]--;
				int pos4 = inv(x)+(16*inv(y));  
				graphletID[pos4]--;*/
			//	__builtin_prefetch(&type[0][0][0], 0, 3); 
				type[x][y][z]++;
				type[x][0][z]--; //descuento para u
				type[inv(x)][0][y]--; //descuento para v
				type[inv(z)][0][inv(y)]--; //descuento para w				
			}
		}
	}
}

void SearchGraphletsDriver(){
		int i,u,j,l;
		int k = num_nodes/2;
	for (j = 0; j < num_nodes; j++) {
//	for (j = num_nodes-1; j >= 0; j--) {
		u =  order[j].id;
		int n1 = Nodes[u][0];
		int n2 = Nodes[u][1];
		int n3 = Nodes[u][2];
		type[1][0][1] += comb2(n1);
//	printf("type[1][0][1]:%d\n",type[1][0][1]);
		type[2][0][2] += comb2(n2);  
//	printf("type[2][0][2]:%d\n",type[2][0][2]);
		type[3][0][3] += comb2(n3); 
//	printf("type[3][0][3]:%d\n",type[3][0][3]);
		type[1][0][2] += n1 * n2;
//	printf("type[1][0][2]:%d\n",type[1][0][2]);
		type[1][0][3] += n1 * n3;
//	printf("type[1][0][3]:%d\n",type[1][0][3]);
		type[2][0][3] += n2 * n3;
//print_types();	
//	getchar();		

		SearchGraphlets_3(u);
	//	return;
	}
}

int main() {
    int i,e;
	struct timeval tstart, tend;

    // Build the graph using array indexes
    __builtin_prefetch(&type[0][0][0], 0, 3); 
    initialize_type();

//	ReadGraph("./Benchmarks/outs/8nodos_procesado.txt");
	ReadGraph("./Benchmarks/outs/TFLink_Homo_sapiens_interactions_LS_simpleFormat_v1.0.tsv_procesado.txt");
//	printNodes();
//	getchar();
    //time_t inicio = time(NULL);
    gettimeofday(&tstart, NULL);
    qsort(order, num_nodes, sizeof(struct toSort), compararPorGradeAscendente);
	UpdatePos();
	Make_Graph();
//	printG();

 	//SearchGraphletsDriver();
 	SearchGraphlets();

	
//	SearchGraphletsDriver();
//	print_types();
	//time_t fin = time(NULL);
	gettimeofday(&tend, NULL);
    double tiempo_transcurrido = 1.0 * (tend.tv_sec - tstart.tv_sec) + 1.0 * (tend.tv_usec - tstart.tv_usec) / 1000000.0;
	//double tiempo_transcurrido = difftime(fin, inicio);
	
//	print_types();
	printf("touchNodes:%llu, procesedNodes:%llu tiempo_transcurrido:%f\n",touchNodes,procesedNodes,tiempo_transcurrido);


	
    return 0;
}
