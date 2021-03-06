/* Sen Chai - NUSP 10727830
	trabalho de alg2, B tree com insercao e busca apenas
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFERSIZE 4096 
#define ORDER 13
#define INTFIELD 10
#define STRFIELD 16
#define STUDENTSIZE (2*INTFIELD+3*STRFIELD-1)

typedef struct STUDENT STUDENT;
typedef struct PAGE PAGE;
typedef struct BTREE BTREE;

struct STUDENT{
	int numUSP;
	char name[STRFIELD];
	char surname[STRFIELD];
	char course[STRFIELD];
	float grade;
} ;

struct PAGE{ // indexFile page, not dataFile page
	int isparent;
	int pagerrn;
	int nkeys;
	int *keys;
	int *rrns;
	int *children;
} ;

struct BTREE {
	int last_page_rrn; // n de paginas/ nos 
	int last_data_rrn; // arquivo de dados
	int height;
	int root;
};

FILE *bringFile(char* fileName) {
    FILE* file= fopen(fileName, "r+");

    if (file == NULL){
        file = fopen(fileName, "w");
        fclose(file);
	    file = fopen(fileName, "r+");
	}
    return file;
}

BTREE *loadIndexHeader(FILE*file){
	// guarda o cabecalho da btree, contem as especificacoes, da arvore,  
	// retorna a arvore B
	BTREE *tree = NULL;
	fseek(file,0,SEEK_END);
	int fileEnd = ftell(file);
	if(fileEnd == 0){
		tree = (BTREE*) calloc(1,sizeof(BTREE));
	}
	else{
		tree = (BTREE*) malloc(sizeof(BTREE));
		fseek(file,0,SEEK_SET);
		fscanf(file,"%d%d%d%d",&tree->last_page_rrn,&tree->last_data_rrn,&tree->height,&tree->root);
	}
	return tree;
}

void storeIndexHeader(FILE*file,BTREE*tree){
	fseek(file,0,SEEK_SET);
	fprintf(file,"%-*d %-*d %-*d %-*d ",INTFIELD, tree->last_page_rrn,INTFIELD, tree->last_data_rrn,INTFIELD, tree->height,INTFIELD, tree->root);
}

PAGE *createEmptyIndexPage(){
	PAGE *pg = (PAGE*) calloc(1,sizeof(PAGE));
	pg->keys = (int*) calloc(ORDER,sizeof(int));
	pg->rrns = (int*) calloc(ORDER,sizeof(int));
	pg->children = (int*) calloc(ORDER+1,sizeof(int));

	for(int i = 0; i< ORDER ; i++){
		pg->keys[i] = -1;
		pg->rrns[i] = -1;
		pg->children[i] = -1;
	}
	pg->children[ORDER] = -1;
	
	return pg;
}
void free_page(PAGE **page) {

    if(*page == NULL) return;

    if((*page)->keys!= NULL)
        free((*page)->keys);
    
    if((*page)->rrns!= NULL)
        free((*page)->rrns);
        
    if((*page)->children!= NULL)
        free((*page)->children);

    free(*page);
    *page = NULL;
    return;
}

PAGE *getIndexPage(FILE*file,PAGE*pg,int pos){
	fseek(file,pos*BUFFERSIZE,SEEK_SET);
	fscanf(file,"%d", &pg->isparent);
	fscanf(file,"%d", &pg->pagerrn);
	fscanf(file,"%d", &pg->nkeys);
	for(int i = 0; i<pg->nkeys; i++){
		fscanf(file,"%d",&pg->keys[i]);
	}
	for(int i = 0; i<pg->nkeys ; i++){
		fscanf(file,"%d",&pg->rrns[i]);
	}
	for(int i = 0; i<pg->nkeys ; i++){
		fscanf(file,"%d",&pg->children[i]);
	}
	// ultima childern
	fscanf(file,"%d",&pg->children[pg->nkeys]);
	return pg;
}

void storeIndexPage(FILE*file, PAGE *pg){
	fseek(file,pg->pagerrn*BUFFERSIZE,SEEK_SET);
	int start = ftell(file);
	fprintf(file,"%-*d%-*d%-*d",INTFIELD, pg->isparent,INTFIELD, pg->pagerrn,INTFIELD, pg->nkeys);
	for(int i = 0; i<pg->nkeys ; i++){
		fprintf(file,"%-*d",INTFIELD,pg->keys[i]);
	}
	for(int i = 0; i<pg->nkeys ; i++){
		fprintf(file,"%-*d",INTFIELD,pg->rrns[i]);
	}
	for(int i = 0; i<pg->nkeys ; i++){
		fprintf(file,"%-*d",INTFIELD,pg->children[i]);
	}
	// registrar ultima child rrn
	fprintf(file,"%-*d",INTFIELD, pg->children[pg->nkeys]);
	int end = ftell(file);
	int free_space = BUFFERSIZE-(end-start)-1;
	fprintf(file,"%*s\n",free_space,"");
}

void printPage(PAGE *pg){
	// auxilia debug
	printf("isparent:%-*dpagerrn:%-*dnkeys:%-*d\n",INTFIELD, pg->isparent,INTFIELD, pg->pagerrn,INTFIELD, pg->nkeys);
	for(int i = 0; i<pg->nkeys ; i++){
		printf("%-*d",INTFIELD,pg->keys[i]);
	}
	for(int i = 0; i<pg->nkeys ; i++){
		printf("%-*d",INTFIELD,pg->rrns[i]);
	}
	for(int i = 0; i<pg->nkeys ; i++){
		printf("%-*d",INTFIELD,pg->children[i]);
	}
	printf("%-*d\n\n",INTFIELD, pg->children[pg->nkeys]);
}

void printStudent(STUDENT st){
	printf("%d|%s|%s|%s|%f\n",st.numUSP,st.name,st.surname,st.course,st.grade);
}

void getStudent(FILE*dataFile,int rrn){
	fseek(dataFile,rrn*(STUDENTSIZE),SEEK_SET);
	STUDENT st;
	fscanf(dataFile,"%d",&st.numUSP);
	fscanf(dataFile,"%s",st.name);
	fscanf(dataFile,"%s",st.surname);
	fscanf(dataFile,"%s",st.course);
	fscanf(dataFile,"%f",&st.grade);
	printf("\n%d|%s|%s|%s|%f\n",st.numUSP,st.name,st.surname,st.course,st.grade);
}

int searchStudentByKey(FILE*indexFile,BTREE*tree,int key){
	PAGE*pg=createEmptyIndexPage();
	getIndexPage(indexFile,pg,tree->root);
	int i = 0;
	while(pg->isparent>0){
		for(i = 0; i<pg->nkeys ; i++){
			if(key==pg->keys[i]){
				return pg->rrns[i];
			}
			if(key < pg->keys[i] ){ 
				break;
			}
			else if(i==pg->nkeys-1){
			}
		}
		getIndexPage(indexFile,pg,pg->children[i]);
	}
	for(int j=0; j<pg->nkeys; j++){
		if(key==pg->keys[j])
			return pg->rrns[j];
	}
	return -1;
	// not found
}

STUDENT *enterStudent(STUDENT*st){
	// aceita entrada no terminal
	printf("Insira os dados na seguinte Ordem:\n \t 1.numUSP \t 2.Nome \t 3.Sobrenome  \t 4.Curso  \t 5.nota \n");
	printf("Numero USP: " );
	scanf("%d",&st->numUSP);
	printf("\nNome: " );
	scanf("%s",st->name);
	printf("\nSobrenome: " );
	scanf("%s",st->surname);
	printf("\nCurso: " );
	scanf("%s",st->course);
	printf("\nNota: " );
	scanf("%f",&st->grade);
	return st;
}

void insertKeyToLeaf(BTREE*tree,PAGE*pg,int key){
	// insercao da chave em pagina usando insertion sort
	// retorna 1 ou 0, se tiver lotacao total (overflow) ou nao
	if(pg->isparent) {
		printf("* Tried inserting on a parent\n");
		return;
	}
	for(int i = 0; i<pg->nkeys ; i++){
		if(key<pg->keys[i]){
			// deslocar insercao a esquerda
			memcpy(&pg->keys[i+1],&pg->keys[i],(pg->nkeys-i)*sizeof(int));
			memcpy(&pg->rrns[i+1],&pg->rrns[i],(pg->nkeys-i)*sizeof(int));
			pg->keys[i] = key;
			pg->rrns[i] = tree->last_data_rrn++;
			pg->nkeys++;
			return;
		}
	}
	pg->keys[pg->nkeys] = key;
	pg->rrns[pg->nkeys] = tree->last_data_rrn++;
	pg->nkeys++;
}
void promote(PAGE*parent,int prom_key,int prom_rrn,int child){
	// insere valores do overflow
	// OBS todos os vetores tem expaco extra para comportar overflow, facilitando organizacao
	for(int i = 0; i<parent->nkeys ; i++){
		if(prom_key<parent->keys[i]){
			// deslocar insercao a esquerda
			memcpy(&parent->keys[i+1],&parent->keys[i],(parent->nkeys-i)*sizeof(int));
			memcpy(&parent->rrns[i+1],&parent->rrns[i],(parent->nkeys-i)*sizeof(int));
			parent->keys[i] = prom_key;
			parent->rrns[i] = prom_rrn; 

			// children tem comportamento diferente dos demais
			memcpy(&parent->children[i+2],&parent->children[i+1],(parent->nkeys-i)*sizeof(int));
			parent->children[i+1] = child;

			parent->nkeys++;
			return ;
		}
	}
	parent->keys[parent->nkeys] = prom_key;
	parent->rrns[parent->nkeys] = prom_rrn;
	parent->children[parent->nkeys+1] = child;
	parent->nkeys++;
}

int overflow(FILE*indexFile,BTREE *tree,PAGE*pg,int height,int parent_tracks[]){
	// lida com splits e overflows de folhas e parents que nao sejma root
	// height: ordem de parent_tracks eh do parent mais proximo do filho para a raiz, por isso height inicia do zero, queremos recuperar o lastro de parents

	PAGE*current_root = createEmptyIndexPage();
	getIndexPage(indexFile,current_root,tree->root);

	PAGE *newpg = createEmptyIndexPage();
	// split de folhas
	int half = (int) pg->nkeys/2;
	newpg->nkeys = (int) pg->nkeys/2;
	if(pg->nkeys%2 == 0) newpg->nkeys--;
	// quando o original for par, newpg split tera um elemento a menos
	for(int i = 0; i<newpg->nkeys ; i++){
		newpg->keys[i] = pg->keys[half+i+1];
		newpg->rrns[i] = pg->rrns[half+i+1];
		newpg->children[i] = pg->children[half+i+1];
	}
	newpg->children[newpg->nkeys] = pg->children[pg->nkeys];
	if(height!=0){
		newpg->isparent = 1;
	}
	newpg->pagerrn = ++tree->last_page_rrn;
	storeIndexPage(indexFile,newpg);

	pg->nkeys = half;
	int overflowed_rrn = pg->pagerrn;
	storeIndexPage(indexFile,pg);

	int promoted_key = pg->keys[half];
	int promoted_rrn = pg->rrns[half];


	if(height==tree->height && current_root->nkeys>=ORDER){
		// root overflow: estou no root && rootOverflow
		PAGE*newroot = createEmptyIndexPage();
		newroot->isparent = 1;
		newroot->pagerrn = ++tree->last_page_rrn;
		newroot->nkeys= 1;
		newroot->keys[0] = promoted_key;
		newroot->rrns[0] = promoted_rrn;
		newroot->children[0] = overflowed_rrn;
		newroot->children[1] = newpg->pagerrn ;
		tree->height++;
		tree->root = newroot->pagerrn;
		storeIndexPage(indexFile,newroot);
		free_page(&newroot);
	}
	else{
		getIndexPage(indexFile,pg,parent_tracks[height]);
		// promocao
		// ascendete mais proximo da folha que teve overflow
		promote(pg,promoted_key,promoted_rrn,newpg->pagerrn);
		storeIndexPage(indexFile,pg);
	}
	free_page(&newpg);
	free_page(&current_root);
	return height;
}

BTREE insertStudent(FILE *dataFile, FILE*indexFile, BTREE *tree,PAGE*pg, STUDENT *st){
	pg = getIndexPage(indexFile,pg,tree->root) ;

	int parent_tracks[tree->height];
	int i =0;
	int key = st->numUSP;
	// eoncontrar pagina folha correta
	if(tree->height == 0 ){
		parent_tracks[0] = 0;
	}
	int height = tree->height;
	// lastro de quais parents foram percorridos, ordenados inversamente a trajetoria percorrida
	while(pg->isparent>0){
		// enquanto houver nos pais redirecione
		for(i = 0; i<pg->nkeys ; i++){
			if(key < pg->keys[i] ){
				// necessidade de procurar proxima pagina
				break;
			}
			else if(i==pg->nkeys-1){
			}
		}
		parent_tracks[--height] = pg->pagerrn;
		// lastro dos pais percorridos
		getIndexPage(indexFile,pg,pg->children[i]);
	}
	insertKeyToLeaf(tree,pg,st->numUSP);
	storeIndexPage(indexFile,pg);
	// insercao em arquivo de dados
	fseek(dataFile,0,SEEK_END);

	fprintf(dataFile,"%-*d%-*s%-*s%-*s%-8.2f\n",INTFIELD, st->numUSP,STRFIELD, st->name,STRFIELD, st->surname,STRFIELD, st->course, st->grade);

	height = 0;
	while(height<=tree->height && pg->nkeys >= ORDER){ 
		// se tiver 1 overflow , 
		height = overflow(indexFile,tree,pg,height,parent_tracks);
		height++;
		// verificar se o pg volta com valores novos
	}
}
