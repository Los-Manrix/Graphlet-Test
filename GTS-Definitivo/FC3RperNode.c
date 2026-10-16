#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Define limits based on your large dataset
#define MAX_VERTICES 2500005
#define MAX_EDGES    55000005  // Total directed edges (multiply by 2 for bidirectional)
#define MAX_NEI 50000
int num_nodes,num_edges;


int values[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 2, 3, 2, 5, 9, 10,
        4, 5, 5, 6, 7, 11, 10, 12,
        0, 2, 4, 7, 1, 5, 5, 11,
        2, 9, 5, 10, 3, 10, 6, 12,
        0, 3, 7, 8, 3, 6, 10, 12,
        7, 10, 11, 12, 8, 12, 12, 13
};


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
unsigned int TypePerNode[MAX_VERTICES][14];

long long int ntriagles = 0;


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

void InitializeGraphletsperNode(){
	int i,j;
	for (i = 0; i < num_nodes; i++)
		for (j=0; j<14; j++) 
			TypePerNode[i][j] = 0;

}

void printGraphletsperNode() {
	int i,j;
	for (i = 0; i < num_nodes; i++){
		for (j=0; j<14; j++)
			printf("[%d-%d]",j,TypePerNode[i][j]); 
			printf("\n\n");
	}
}

int GraphletType(int c){
    return values[c];
}

long long int truenum_edges = 0;
void ReadEdges(const char* filename) {
	FILE* f;
	int i, ori, dest, t;
	f = fopen(filename, "r");
	if (f == NULL) 	{
		printf("Cannot open file %s.\n", filename);
		exit(1);
	}
	if (fscanf(f, "%d %d", &num_nodes, &num_edges) != 2) {
		printf("Error reading header N M from %s.\n", filename);
		exit(1);
	}
	fscanf(f, "\n");
    initGraph(num_nodes);

    long data_start = ftell(f);

    // Auto-detección dinámica de Base 0 vs Base 1 para evitar Core Dumped
    int shift = 1;
    for (i = 0; i < num_edges; i++) {
        if (fscanf(f, "%d %d %d\n", &ori, &dest, &t) != 3) break;
        if (ori == 0 || dest == 0) {
            shift = 0;
            break;
        }
    }
    fseek(f, data_start, SEEK_SET);

	for (i = 0; i < num_edges; i++) {
		if (fscanf(f, "%d %d %d\n", &ori, &dest, &t) != 3) break;
		if (ori != dest){
			int u = ori - shift;
			int v = dest - shift;
			if (u < 0 || v < 0 || u >= num_nodes || v >= num_nodes) continue;

			edge[truenum_edges][0] = u; //source
			edge[truenum_edges][1] = v; //target
			edge[truenum_edges][2] = t; //type

			if (t == 1)
				Nodes[u][0]++;//n1
			else
				if (t == 2)		
					Nodes[u][1]++;//n2
				else
					Nodes[u][2]++;//n3,grade
			order[u].id = u;
        	order[u].grade++;
        	Nodes[u][4]++;//grade
			truenum_edges++;
		}
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

void Make_Graph_Node(){
	int i,u,v,t,j,k,n;
	for (i = 0; i < truenum_edges; i++) {
		u = edge[i][0];
		v = edge[i][1];
		t = edge[i][2];

		//int n1 = Nodes[u][0];
		//int n2 = Nodes[u][1];
		//int n3 = Nodes[u][2];

		if (t == 1)
			for (k = 1; k < 4; k++){
				n = (1 * 16) + (0 * 4) + k;
				j = GraphletType(n);
				if(k == 1)
					TypePerNode[v][j] += Nodes[u][k-1]-1;
				else 
					TypePerNode[v][j] += Nodes[u][k-1];
			}
		else 
			if (t == 2)
				for (k = 1; k < 4; k++){
					n = (2 * 16) + (0 * 4) + k;
					j = GraphletType(n);
					if(k == 2)
						TypePerNode[v][j] += Nodes[u][k-1]-1;
					else 
						TypePerNode[v][j] += Nodes[u][k-1];
				}
			else 
				if (t == 3)
					for (k = 1; k < 4; k++){
						n = (3 * 16) + (0 * 4) + k;
						j = GraphletType(n);
						if(k == 3)
							TypePerNode[v][j] += Nodes[u][k-1]-1;
						else 
							TypePerNode[v][j] += Nodes[u][k-1];
					}
		int posu = Nodes[u][3];
		int posv = Nodes[v][3];
		if (posv > posu){
			addDirectedEdge(u,v,t);
			edgeCount++;
		}
	}
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


int inv(int t) {
    static const int tabla[] = {3, 2, 1}; 
    return (t >= 0 && t <= 2) ? tabla[t] : 3;
}




void SearchGraphletsTotal(){
	int i,j,k,u;
	struct timeval tstart, tend;
	
	long long int type1[4][4][4];
	long long int type[64];
	
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

		for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
			int v = G[i];
			Gtuv[v] = Gtype[i]; //type from u to v		
			Gp[v] = u;
			touchNodes;
		}

		for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
			int v = G[i];
			for (j = Gini[v];j < Gini[v]+Ggrade[v];j++){
				int w = G[j];
				touchNodes++;
				if (Gp[v] == Gp[w]){ //Triangle detected
					short x = Gtuv[v];
					short y = Gtype[j];
					short z = Gtuv[w];
					ntriagles++;
				
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

	printf("touchNodes:%llu, ntriagles:%llu t:%f\n",touchNodes,ntriagles,tiempo_transcurrido);
	for(i = 0;i < 4;i++)
		for(j = 0;j < 4;j++)
			for(k = 0;k < 4;k++)
				printf("[%d][%d][%d] : %lld\n",i,j,k,type1[i][j][k]);
		
	return;
}

void SearchGraphletsTotal2(){
	int i,j,k,u;
	
	long long int type1[4][4][4];
	long long int type[64];
    
	for(i = 0;i < 64;i++)
    	type[i] = 0;

	for (k = 0; k < num_nodes; k++) {
		u =  order[k].id;	
		int n1 = Nodes[u][0];
		int n2 = Nodes[u][1];
		int n3 = Nodes[u][2];
		type[(1 * 16) + (0 * 4) + 1] += comb2(n1);
		type[(2 * 16) + (0 * 4) + 2] += comb2(n2);
		type[(3 * 16) + (0 * 4) + 3] += comb2(n3);
		type[(1 * 16) + (0 * 4) + 2] += n1 * n2;
		type[(1 * 16) + (0 * 4) + 3] += n1 * n3;
		type[(2 * 16) + (0 * 4) + 3] += n2 * n3;
		
		for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
			int v = G[i];
			Gtuv[v] = Gtype[i]; //type from u to v		
			Gp[v] = u;
			touchNodes;
		}

		for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
			int v = G[i];
			for (j = Gini[v];j < Gini[v]+Ggrade[v];j++){
				int w = G[j];
				touchNodes++;
				if (Gp[v] == Gp[w]){ //Triangle detected
					short x = Gtuv[v];
					short y = Gtype[j];
					short z = Gtuv[w];
					short i1 = (x * 16) + (y * 4) + z;
					short i2 = (x * 16) + (0 * 4) + z;
					short i3 = (inv(x) * 16) + (0 * 4) + y;
					short i4 = (inv(z) * 16) + (0 * 4) + inv(y);
					
					ntriagles++;					
					type[i1]++;
					type[i2]--;
					type[i3]--;
					type[i4]--;										
				}
			}
		}
	}

	for(i = 0;i < 64;i++)
		printf("[%d] : %lld\n",i,type[i]);
		
	return;
}




void SearchGraphletsTotalPerNode(){
	int i,j,k,u,n;
	
	
	long long int type[64];
    
	for(i = 0;i < 64;i++)
    	type[i] = 0;

	for (k = 0; k < num_nodes; k++) {
		u =  order[k].id;	
		int n1 = Nodes[u][0];
		int n2 = Nodes[u][1];
		int n3 = Nodes[u][2];
		n= (1 * 16) + (0 * 4) + 1;j = GraphletType(n);
		TypePerNode[u][j] += comb2(n1);
		n= (2 * 16) + (0 * 4) + 2;j = GraphletType(n);
		TypePerNode[u][j] += comb2(n2);
		n= (3 * 16) + (0 * 4) + 3;j = GraphletType(n);
		TypePerNode[u][j] += comb2(n3);
		n= (1 * 16) + (0 * 4) + 2;j = GraphletType(n);
		TypePerNode[u][j] += n1 * n2;
		n= (1 * 16) + (0 * 4) + 3;j = GraphletType(n);
		TypePerNode[u][j] += n1 * n3;
		n= (2 * 16) + (0 * 4) + 3;j = GraphletType(n);
		TypePerNode[u][j] += n2 * n3;
		
		
		for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
			int v = G[i];
			Gtuv[v] = Gtype[i]; //type from u to v		
			Gp[v] = u;
			touchNodes;
		}

		for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
			int v = G[i];
			for (j = Gini[v];j < Gini[v]+Ggrade[v];j++){
				int w = G[j];
				touchNodes++;
				if (Gp[v] == Gp[w]){ //Triangle detected
					short x = Gtuv[v];
					short y = Gtype[j];
					short z = Gtuv[w];
					short a1 = (x * 16) + (y * 4) + z;
					short a2 = (x * 16) + (0 * 4) + z;
					short a3 = (inv(x) * 16) + (0 * 4) + y;
					short a4 = (inv(z) * 16) + (0 * 4) + inv(y);

					short i1 = GraphletType(a1);
					short i2 = GraphletType(a2);
					short i3 = GraphletType(a3);
					short i4 = GraphletType(a4);
					
					ntriagles++;					
					TypePerNode[u][i1]++;
					TypePerNode[v][i1]++;
					TypePerNode[w][i1]++;
										
					TypePerNode[u][i2]--;
					TypePerNode[v][i2]--;
					TypePerNode[w][i2]--;
					
					TypePerNode[v][i3]--;
					TypePerNode[u][i3]--;
					TypePerNode[w][i3]--;					
					
					
					TypePerNode[w][i4]--;
					TypePerNode[v][i4]--;
					TypePerNode[u][i4]--;
					
					
															
				}
			}
		}
	}
		
	return;
}


unsigned int**  SearchGraphletsTotalPerNode2(){
	int i,u,v,t,j,k,n;

	
//	unsigned int TypePerNode2[MAX_VERTICES][14];

	unsigned int** TypePerNode2 = (unsigned int**) malloc(MAX_VERTICES * sizeof(unsigned int*));
	for (i = 0; i < num_nodes; i++)
		TypePerNode2[i] = (unsigned int*) malloc(14 * sizeof(unsigned int));

	for (i = 0; i < num_nodes; i++)
		for (j=0; j<14; j++) 
			TypePerNode2[i][j] = 0;
	
	for (i = 0; i < truenum_edges; i++) {
		u = edge[i][0];
		v = edge[i][1];
		t = edge[i][2];

		if (t == 1)
			for (k = 1; k < 4; k++){
				n = (1 * 16) + (0 * 4) + k;
				j = GraphletType(n);
				if(k == 1)
					TypePerNode2[v][j] += Nodes[u][k-1]-1;
				else 
					TypePerNode2[v][j] += Nodes[u][k-1];
			}
		else 
			if (t == 2)
				for (k = 1; k < 4; k++){
					n = (2 * 16) + (0 * 4) + k;
					j = GraphletType(n);
					if(k == 2)
						TypePerNode2[v][j] += Nodes[u][k-1]-1;
					else 
						TypePerNode2[v][j] += Nodes[u][k-1];
				}
			else 
				if (t == 3)
					for (k = 1; k < 4; k++){
						n = (3 * 16) + (0 * 4) + k;
						j = GraphletType(n);
						if(k == 3)
							TypePerNode2[v][j] += Nodes[u][k-1]-1;
						else 
							TypePerNode2[v][j] += Nodes[u][k-1];
					}
		int posu = Nodes[u][3];
		int posv = Nodes[v][3];
		if (posv > posu){
			addDirectedEdge(u,v,t);
			edgeCount++;
		}
	}
    


	for (k = 0; k < num_nodes; k++) {
		u =  order[k].id;	
		int n1 = Nodes[u][0];
		int n2 = Nodes[u][1];
		int n3 = Nodes[u][2];
		n= (1 * 16) + (0 * 4) + 1;j = GraphletType(n);
		TypePerNode2[u][j] += comb2(n1);
		n= (2 * 16) + (0 * 4) + 2;j = GraphletType(n);
		TypePerNode2[u][j] += comb2(n2);
		n= (3 * 16) + (0 * 4) + 3;j = GraphletType(n);
		TypePerNode2[u][j] += comb2(n3);
		n= (1 * 16) + (0 * 4) + 2;j = GraphletType(n);
		TypePerNode2[u][j] += n1 * n2;
		n= (1 * 16) + (0 * 4) + 3;j = GraphletType(n);
		TypePerNode2[u][j] += n1 * n3;
		n= (2 * 16) + (0 * 4) + 3;j = GraphletType(n);
		TypePerNode2[u][j] += n2 * n3;
		
		
		for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
			int v = G[i];
			Gtuv[v] = Gtype[i]; //type from u to v		
			Gp[v] = u;
			touchNodes;
		}

		for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
			int v = G[i];
			for (j = Gini[v];j < Gini[v]+Ggrade[v];j++){
				int w = G[j];
				touchNodes++;
				if (Gp[v] == Gp[w]){ //Triangle detected
					short x = Gtuv[v];
					short y = Gtype[j];
					short z = Gtuv[w];
					short a1 = (x * 16) + (y * 4) + z;
					short a2 = (x * 16) + (0 * 4) + z;
					short a3 = (inv(x) * 16) + (0 * 4) + y;
					short a4 = (inv(z) * 16) + (0 * 4) + inv(y);

					short i1 = GraphletType(a1);
					short i2 = GraphletType(a2);
					short i3 = GraphletType(a3);
					short i4 = GraphletType(a4);
					
					ntriagles++;					
					TypePerNode2[u][i1]++;
					TypePerNode2[v][i1]++;
					TypePerNode2[w][i1]++;
										
					TypePerNode2[u][i2]--;
					TypePerNode2[v][i2]--;
					TypePerNode2[w][i2]--;
					
					TypePerNode2[v][i3]--;
					TypePerNode2[u][i3]--;
					TypePerNode2[w][i3]--;					
					
					
					TypePerNode2[w][i4]--;
					TypePerNode2[v][i4]--;
					TypePerNode2[u][i4]--;
					
					
															
				}
			}
		}
	}
		
	return TypePerNode2;
}




int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <ruta_grafo> [version: 1 o 2 (default 2)] [orden: asc o desc (default asc)]\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    int modo = 2;
    if (argc >= 3) {
        modo = atoi(argv[2]);
        if (modo != 1 && modo != 2) modo = 2;
    }

    struct timeval tstart, tend;

    ReadEdges(filename);

    gettimeofday(&tstart, NULL);

    if (argc >= 4 && strcmp(argv[3], "desc") == 0) {
        qsort(order, num_nodes, sizeof(struct toSort), compararPorGradeDescendente);
    } else {
        qsort(order, num_nodes, sizeof(struct toSort), compararPorGradeAscendente);
    }
	
    UpdatePos();

    if (modo == 1) {
        InitializeGraphletsperNode();
        Make_Graph_Node();
        SearchGraphletsTotalPerNode();
    } else {
        SearchGraphletsTotalPerNode2();
    }
	
    gettimeofday(&tend, NULL);

    double tiempo_transcurrido = 1.0 * (tend.tv_sec - tstart.tv_sec) + 1.0 * (tend.tv_usec - tstart.tv_usec) / 1000000.0;
    printf("touchNodes:%llu, ntriagles:%llu tiempo_transcurrido:%f\n", touchNodes, ntriagles, tiempo_transcurrido);
	
    return 0;
}
