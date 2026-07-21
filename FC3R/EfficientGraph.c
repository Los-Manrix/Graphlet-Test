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
};

struct toSort;
typedef struct toSort toSort;

struct toSort //nodes
{
  int id;
  unsigned grade; 
};

// Funci�n de comparaci�n segura para tipos unsigned (Mayor a Menor)
int compararPorGradeDescendente(const void *a, const void *b) {
    // 1. Convertir los punteros gen�ricos al tipo de tu estructura
    const struct toSort *nodoA = (const struct toSort *)a;
    const struct toSort *nodoB = (const struct toSort *)b;

    // 2. L�gica para orden descendente (Mayor a Menor)
    if (nodoA->grade < nodoB->grade) return 1;  // Si A es menor, va despu�s
    if (nodoA->grade > nodoB->grade) return -1; // Si A es mayor, va antes
    return 0;                                   // Si son iguales, se quedan igual
}

int compararPorGradeAscendente(const void *a, const void *b) {
    // 1. Convertir los punteros gen�ricos al tipo de tu estructura
    const struct toSort *nodoA = (const struct toSort *)a;
    const struct toSort *nodoB = (const struct toSort *)b;

    // 2. L�gica para orden descendente (Mayor a Menor)
    if (nodoA->grade > nodoB->grade) return 1;  // Si A es menor, va despu�s
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
        
		order[i].id = i;
        order[i].grade = 0;
    }
    edgeCount = 0;
}

// 2. Add a directed edge from 'u' to 'v'
void addDirectedEdge(int u, int v, int t) {
    if (edgeCount >= MAX_EDGES) {
        printf("Error: Edge array capacity exceeded!\n");
        return;
    }
    toEdge[edgeCount] = v;          // Set destination
    toEdgeType[edgeCount] = t;
    nextEdge[edgeCount] = nodes[u].head;  // Link current edge to vertex u's old edge list
    nodes[u].head = edgeCount;            // Move head pointer to this new edge
    order[u].grade++;
	nodes[u].grade++;
	edgeCount++;
}

void printNeighbors(int u) {
    int e, n = nodes[u].id;
    
	printf("Vertex %d grade, %d neighbors: \n", n,nodes[n].grade);
    // Traverse the array-linked list until we hit -1
    
	for (e = nodes[n].head; e != -1; e = nextEdge[e]) {
        printf("e: %d toEdge[e] %d \n",e, toEdge[e]);
        getchar();
    }
    printf("\n");
/*	for (e = nodes[n].head; e != -1; e = nextEdge[e]) {
        printf("%d ", toEdgeType[e]);
    }*/
	
    printf("\n");
    //getchar();
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
		addDirectedEdge(ori-1, dest-1, t);
	//	printf("%d %d %d %d\n", ori, dest, dist, t);
	}
	fclose(f);
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
				printf("[%d][%d][%d] : %llu\n",i,j,k,type[i][j][k]);
}

long long comb2(int n) {
    if (n < 2) return 0;
    return ((long long)n * (n - 1)) >> 1;
}
long long unsigned touchNodes = 0;
long long unsigned procesedNodes = 0;

void SearchGraphlets(int i, int iter){
	int e, s = nodes[i].id;
	int n1 = 0, n2 = 0, n3 = 0;
	int q[MAX_NEI];
	int nq = 0;
	
	nodes[s].color = 0;
	for (e = nodes[s].head; e != -1; e = nextEdge[e]) {
		int n = toEdge[e];
		touchNodes++;
	//	printf("n:%d type:%d\n",n,toEdgeType[e]);
		if (nodes[n].color){
			procesedNodes++;
			nodes[n].tsn = toEdgeType[e]; 
			if (nodes[n].tsn == 1)
				n1++;
			else 
				if (nodes[n].tsn == 2)
					n2++;
				else
					n3++;
			q[nq++] = n;
			nodes[n].inq = 1;		
		}
	}
//	printf("n1:%d n2:%d n3:%d nq:%d (%llu,%llu)\n",n1,n2,n3,nq,touchNodes,procesedNodes);
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
//	printf("type[2][0][3]:%d\n",type[2][0][3]);
//	getchar();

    int sumagrade = 0;
    for (i = 0; i < nq; i++){
    	int n = q[i];
    	nodes[n].inq = 0;
		nodes[n].iter = iter;
		nodes[n].color = 0;
		sumagrade += nodes[n].grade;
		//printf("nodes[n].id %d nodes[n].grade %d\n",nodes[n].id,nodes[n].grade);
    	for (e = nodes[n].head; e != -1; e = nextEdge[e]) {
    		int m = toEdge[e];
    		touchNodes++;
    		if (nodes[m].color)
    			nodes[n].color = 1;
    		if (nodes[m].color && nodes[m].iter != iter){
    			procesedNodes++;
    			if (nodes[m].inq){
    			//	printf("Triangulo se quita [%d,%d,%d] se agrega [%d,%d,%d]\n",
				//	nodes[n].tsn,0,nodes[m].tsn,nodes[n].tsn,toEdgeType[e],nodes[m].tsn);
    				type[nodes[n].tsn][0][nodes[m].tsn]--;
    				type[nodes[n].tsn][toEdgeType[e]][nodes[m].tsn]++;
				}else{
				//	printf("Se agrega [%d,%d,%d]\n",nodes[n].tsn,0,toEdgeType[e]);
					type[nodes[n].tsn][toEdgeType[e]][0]++;
				}
					
			}
		}
	}

}


void SearchGraphletsDriver(){
	int iter = 1,i;
	for (i = 0; i < num_nodes; i++) {
		int k = order[i].id;
		
		if(nodes[k].color){
			printf("(%d) ENTRA %d: nsuccs:%d \n",i, nodes[k].id,nodes[k].grade);
			SearchGraphlets(k, iter);
			printf("touchNodes:%llu, procesedNodes:%llu\n",touchNodes,procesedNodes);
			//getchar();
		}
			
		iter++;
    }
	
}

int main(int argc, char *argv[]) {
    int i,e;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_grafo>\n", argv[0]);
        return 1;
    }

    // Build the graph using array indexes
    initialize_type();

	ReadGraph(argv[1]);

    time_t inicio = time(NULL);
    qsort(order, num_nodes, sizeof(struct toSort), compararPorGradeDescendente);
	SearchGraphletsDriver();
	print_types();
	time_t fin = time(NULL);
	double tiempo_transcurrido = difftime(fin, inicio);
	
	printf("touchNodes:%llu, procesedNodes:%llu tiempo_transcurrido:%f\n",touchNodes,procesedNodes,tiempo_transcurrido);


	
    return 0;
}
