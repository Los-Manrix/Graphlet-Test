#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
int Gp[MAX_VERTICES]; //father of a node 
int Gtuv[MAX_VERTICES]; //nodes in queue 1:true
int Gposfree = 0;// // Global counter to add new nodes
int Gcolor[MAX_VERTICES]; //color of a node
int Giter[MAX_VERTICES]; //iter of a node

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
        Gcolor[i] = 0;
		Giter[i] = 0;
		int j;
        for (j=0; j<5; j++) 
			Nodes[i][j] = 0;
    }
    edgeCount = 0;
}


void printNeighbors(int u) {
}

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
		edge[i][0] = ori-1; //source
		edge[i][1] = dest-1; //target
		edge[i][2] = t; //type

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
	for (i = 0; i < num_edges; i++) {
		u = edge[i][0];
		v = edge[i][1];
		t = edge[i][2];
		int posu = Nodes[u][3];
		int posv = Nodes[v][3];
//		if (posv > posu){
			addDirectedEdge(u,v,t);
			edgeCount++;
//		}
	}
} 

long long int type[4][4][4];

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

int inv(int t){
	if (t == 1)
		return 2;
	if (t == 2)
		return 1;
	return 3;
}

void updateTotalGraphlets(int x, int y, int z, int c){
	type[x][y][z] +=c; 
}
void SearchGraphlets_3(int u, int iter){
	int i,j;
	int q[MAX_NEI];
	int nq = 0, n1=0, n2=0, n3=0;
	Gcolor[u] = 1;
	for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
		int v = G[i];
		if (Gcolor[v] == 0){
			int ty = Gtype[i];
			Gtuv[v] = ty; //type from u to v		
			q[nq++] = v;
			Gp[v] = u;
			if (ty == 1)
				n1++;
			else
				if (ty == 2)
					n2++;
				else
					n3++;
		}
	}
		
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

    for (i = 0; i < nq; i++){
   		int v = q[i];
   		Giter[v] = iter;
		for (j = Gini[v];j < Gini[v]+Ggrade[v];j++){
			int w = G[j];
			//touchNodes++;
			//Nodes[v][3]
			if (Gcolor[w] == 0 && Giter[w] != iter){
				int x = Gtuv[v];
				int y = Gtype[j];
				int z = Gtuv[w];
				if (Gp[v] == Gp[w]){ //Triangle detected
				//touchNodes++;
					type[x][y][z]++;
					type[x][0][z]--;
				}
				else
					type[x][y][0]++;
			}
		}
	}

}

void SearchGraphlets_3V2(int u, int iter){
	int i,j;
	int q[MAX_NEI];
	int nq = 0, n1=0, n2=0, n3=0;
	for (i = Gini[u];i < Gini[u]+Ggrade[u];i++){
		int v = G[i];
		int ty = Gtype[i];
		Gtuv[v] = ty; //type from u to v		
		q[nq++] = v;
		Gp[v] = u;
		if (ty == 1)
			n1++;
		else
			if (ty == 2)
				n2++;
			else
				n3++;
	}
		
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

    for (i = 0; i < nq; i++){
   		int v = q[i];
   		Giter[v] = iter;
		for (j = Gini[v];j < Gini[v]+Ggrade[v];j++){
			int w = G[j];
			if (w == u){
				int last = Gini[v]+Ggrade[v]-1;
				G[j] = G[last];
				Gtype[j] = Gtype[last];
				Ggrade[v]--;
				printf("se corto v:%d a u:%d se reemplaso %d por %d \n",v,u, w,G[j]);
				getchar();
				continue;
			}
				
			//touchNodes++;
			//Nodes[v][3]
			if (Giter[w] != iter){
				int x = Gtuv[v];
				int y = Gtype[j];
				int z = Gtuv[w];
				if (Gp[v] == Gp[w]){ //Triangle detected
				//touchNodes++;
					type[x][y][z]++;
					type[x][0][z]--;
				}
				else
					type[x][y][0]++;
			}
		}
	}

}



void SearchGraphletsDriver(){
	int i,u,j;
	int iter = 1;
	for (j = 0; j < num_nodes; j++) {
		u =  order[j].id;
		SearchGraphlets_3(u,iter);
		iter++;
	}
}

int main() {
    int i,e;


    // Build the graph using array indexes
    initialize_type();
//	ReadGraph("./Benchmarks/outs/7nodos_procesado.txt");
//	ReadGraph("./Benchmarks/outs/TFLink_Drosophila_melanogaster_interactions_LS_simpleFormat_v1.0_procesado.txt");
	ReadGraph("./Benchmarks/outs/TFLink_Homo_sapiens_interactions_LS_simpleFormat_v1.0.tsv_procesado.txt");
//	printNodes();
//	getchar();
    time_t inicio = time(NULL);
    qsort(order, num_nodes, sizeof(struct toSort), compararPorGradeDescendente);
	UpdatePos();
	Make_Graph();
//	printG();

 	SearchGraphletsDriver();

	
//	SearchGraphletsDriver();
//	print_types();
	time_t fin = time(NULL);
	double tiempo_transcurrido = difftime(fin, inicio);
	print_types();
	printf("touchNodes:%llu, procesedNodes:%llu tiempo_transcurrido:%f\n",touchNodes,procesedNodes,tiempo_transcurrido);


	
    return 0;
}
