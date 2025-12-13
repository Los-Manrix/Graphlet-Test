#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_NODES 50000
#define MAX_QUEUE 50000

struct snode;
typedef struct snode snode;

struct snode //search nodes
{
  int id;
  short color;
  unsigned iter;
  unsigned nsuccs;
  short tpc;
  short depth;
  snode *parent;
};

struct AdjListNode {
    int dest;
    int type;
    struct AdjListNode* next;
};

struct Graph {
    int V;
    struct AdjListNode** array;
};

struct Graph* graph;
snode* searchNodes[MAX_NODES];
snode* queue[MAX_QUEUE];

int front = 0;
int rear = 0;

void clear_queue(){
	front = 0;
	rear = 0;
}


int empty_queue(){
	return (rear >= front);
}
void queue_insert(snode* node){
	queue[front++] = node;
}
snode* queue_pop(){
	return queue[rear++]; 
}

// Function to create a new adjacency list node
struct AdjListNode* newAdjListNode(int dest,int type) {
    struct AdjListNode* newNode = malloc(sizeof(struct AdjListNode));
    newNode->dest = dest;
    newNode->type = type;
    newNode->next = NULL;
    return newNode;
}

// Function to create a graph of V vertices
struct Graph* createGraph(int V) {
    struct Graph* graph = malloc(sizeof(struct Graph));
    graph->V = V;
    graph->array = calloc(V, sizeof(struct AdjListNode*));
    return graph;
}

// Function to add an edge to an undirected graph
void addEdge(struct Graph* graph, int src, int dest, int type) {
    
    // Add an edge from src to dest
    struct AdjListNode* node = newAdjListNode(dest,type);
    node->next = graph->array[src];
    graph->array[src] = node;

    // Since the graph is undirected, add an edge from dest to src
   /* node = newAdjListNode(src);
    node->next = graph->array[dest];
    graph->array[dest] = node;*/
}

// Function to print the adjacency list
void printGraph(struct Graph* graph) {
    int i;
    struct AdjListNode* cur;
	for (i = 0; i < graph->V; i++) {
        printf("%d: nsuccs:%d", i,searchNodes[i]->nsuccs);
        for (cur = graph->array[i]; cur; cur = cur->next) {
            printf(" [%d %d]", cur->dest,cur->type);
        }
        printf("\n");
    }
}

snode* new_SearchNode(int id) {
    snode* node = (snode*)malloc(sizeof(snode));
  	node->id = id;
  	node->color = 0;
  	node->iter = 0;
  	node->tpc = 0;
  	node->nsuccs = 0;
  	node->depth = 0;
	node->parent = NULL;
    return node;
}

void initializeSearchNodes() {
    int i;
    struct AdjListNode* cur;
	for (i = 0; i < graph->V; i++) {
        searchNodes[i] = new_SearchNode(i); 
    }
}

void ReadGraph(const char* filename) {
	FILE* f;
	int i, ori, dest, dist, t, num_gnodes,num_arcs;
	f = fopen(filename, "r");
	if (f == NULL) 	{
		printf("Cannot open file %s.\n", filename);
		exit(1);
	}
	fscanf(f, "%d %d", &num_gnodes, &num_arcs);
	fscanf(f, "\n");
	printf("%d %d %d\n", num_gnodes, num_arcs,INT_MAX);
	getchar();
	graph = createGraph(num_gnodes);
	initializeSearchNodes();
	for (i = 0; i < num_arcs; i++) {
		fscanf(f, "%d %d %d\n", &ori, &dest, &t);
		addEdge(graph, ori-1, dest-1, t);
		searchNodes[ori-1]->nsuccs++;
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
				printf("[%d][%d][%d] : %d\n",i,j,k,type[i][j][k]);
}


/**
 * Calcula la combinatoria C(n,2) = n*(n-1)/2
 * Usa desplazamiento de bits (>> 1) para dividir por 2
 */
long long comb2(int n) {
    if (n < 2) return 0;
    return ((long long)n * (n - 1)) >> 1;
}


void search_motif(snode* node, int iter){
	int n1 = 0, n2 = 0, n3 = 0;
	node->parent = NULL;
	node->color = 1; //1 is red
	node->depth = 0;
	struct AdjListNode* cur;
	//printf("\n current node %d\n", node->id);

	//printf("antes n1:%d comb2(n1):%d n2:%d n3:%d - %d %d %d %d %d %d\n",n1,comb2(n1),n2,n3,type[1][0][1],type[2][0][2],type[3][0][3],type[1][0][2],type[1][0][3],type[2][0][3]);		
	    
	for (cur = graph->array[node->id]; cur; cur = cur->next) {
		// Optimización: solo procesar sucesores con ID mayor (procesamiento direccional)
		if (cur->dest <= node->id) continue;
		
	//		printf(" [%d %d]", cur->dest,cur->type);
		snode* succ = searchNodes[cur->dest]; 
		if (succ->color != 1){
			succ->tpc = cur->type;
			succ->depth = 1; // Profundidad 1 desde el nodo raíz
			if (succ->tpc == 1)
				n1++;
			if (succ->tpc == 2)
				n2++;
			if (succ->tpc == 3)
				n3++;
			succ->parent = node;
			queue_insert(succ);
			//printf("Insert node %d\n", succ->id);
		}				
    }
	type[1][0][1] += comb2(n1);
	type[2][0][2] += comb2(n2);
	type[3][0][3] += comb2(n3);
	type[1][0][2] += comb2(n1 + n2) - comb2(n1) - comb2(n2);
	type[1][0][3] += comb2(n1 + n3) - comb2(n1) - comb2(n3);
	type[2][0][3] += comb2(n2 + n3) - comb2(n2) - comb2(n3);
    if (type[1][0][1] > INT_MAX){
		printf("llego %d\n",INT_MAX);
//	getchar();
	}

    if (type[1][0][1] > LLONG_MAX || type[1][0][1] < 0){
		printf("llego %d\n",LLONG_MAX);
	getchar();
	}
		

	//printf("n1:%d comb2(n1):%d n2:%d n3:%d - %d %d %d %d %d %d\n",n1,comb2(n1),n2,n3,type[1][0][1],type[2][0][2],type[3][0][3],type[1][0][2],type[1][0][3],type[2][0][3]);		
	while (!(empty_queue())){
		snode* s = queue_pop();
		
		// Optimización: marcar como procesado y evitar reprocesar
		if (s->color == 2) continue;
		s->color = 2; // Marcar como procesado
		s->iter = iter;
		
		// Optimización: limitar profundidad a 2 (motifs de 3 nodos)
		if (s->depth >= 2) continue;
		
		//printf("  Pop node %d\n", s->id);
		for (cur = graph->array[s->id]; cur; cur = cur->next) {
			// Optimización: solo procesar aristas hacia adelante
			if (cur->dest <= node->id) continue;
			
			snode* n = searchNodes[cur->dest];
			
			// Optimización: pre-filtrar nodos no relevantes
			if (n->parent == NULL) continue;
			
			if (n->color != 1 && n->iter != iter){
				//printf("    See node %d\n", n->id);
				if (n->parent == s->parent){
					//printf("Se resta 1 a [%d][%d][%d]\n",s->tpc,0,n->tpc);
					//printf("Se agrega 1 a [%d][%d][%d]\n",s->tpc,cur->type,n->tpc);
					type[s->tpc][0][n->tpc]--;
					type[s->tpc][cur->type][n->tpc]++;
					//if (type[s->tpc][0][n->tpc] < 0)
					//	getchar();
				}else{
					//printf("False Se agrega 1 a [%d][%d][%d]\n",s->tpc,cur->type,0);
					type[s->tpc][cur->type][0]++;
				}
					
			}
			
		}
 
	}
	//getchar();
}

void search_motif_driver(){
	int i,iter = 1;
	struct AdjListNode* cur;
	//printf("%d: nsuccs:%d\n", 2,searchNodes[2]->nsuccs);
	initialize_type();
//	search_motif(searchNodes[2], iter);
	for (i = 0; i < graph->V; i++) {
        printf("%d: nsuccs:%d %d\n", i,searchNodes[i]->nsuccs,type[1][0][1]);
        search_motif(searchNodes[i], iter);
        clear_queue();
	}
	print_types();
}
/*
for all s belong to G
	search_motif(s, iter)
	iter++
}*/

int main() {

    //ReadGraph("yeast_procesado_carlos.txt");
	//ReadGraph("graph_procesado.txt");
	//ReadGraph("TFLink_Homo_sapiens.txt");
	//ReadGraph("TFLink_Danio_rerio_interactions_LS_simpleFormat_v1.0_procesado.txt");
	//ReadGraph("TFLink_Rattus_norvegicus_interactions_LS_simpleFormat_v1.0_procesado.txt");
//	ReadGraph("TFLink_Mus_musculus_interactions_LS_simpleFormat_v1.0.tsv_procesado.txt");
	ReadGraph("TFLink_Drosophila_melanogaster_interactions_LS_simpleFormat_v1.0_procesado.txt");

	search_motif_driver();
	
    printf("Adjacency list representation:\n");
   // printGraph(graph);

    return 0;
}
