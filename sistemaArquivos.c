#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// qtd e tamanho dos setores
#define QTD_SETORES 256
#define TAMANHO_SETOR 512
#define MAX_STMT 256

char* pegaData()
{
  struct timeval tv;
  struct tm* ptm;
  char time_string[40];
  long milliseconds;
  // Obtém data e hora atual, e converte-as em uma estrutura tm.
  gettimeofday (&tv, NULL);
  ptm = localtime (&tv.tv_sec);
  //Formata data e hora, até os segundos.
  strftime (time_string, sizeof (time_string), "%d/%m/%Y %H:%M:%S", ptm);

  // Imprime a hora formatada, até os segundos.
  return time_string;
  }

//funcao encontrada na internet que substitui string passada
char* replace_str(const char *string, const char *search, const char *replacement ){

	char *tok = NULL;
	char *newstr = NULL;

    //procura search em string
	tok = strstr(string, search);
	if(tok == NULL) return strdup(string);
	//alocando espaco para nova string
	newstr = malloc(strlen(string) - strlen(search) + strlen(replacement) + 1);
	if(newstr == NULL) return NULL;
	memcpy(newstr, string, tok - string);
	memcpy(newstr + (tok - string), replacement, strlen(replacement));
	memcpy(newstr + (tok - string) + strlen(replacement), tok + strlen(search), strlen(string) - strlen(search) - (tok - string));
	memset(newstr + strlen(string) - strlen(search) + strlen(replacement), 0, 1);
	
	return newstr;
	
}

//funcao que monta o prompt
void prompt() {
	printf("$ ");
}
// funcao que separa os argumentos pelo " " espaço
char** listaArgumentos(char* stmt) {

	int i = 1;
	char** retorno;
	char* s;
	char stmt_bkp[MAX_STMT];
	
	//realiza uma copia do stmt
	strcpy(stmt_bkp, stmt);
	
	//separa o stmt  pelo espaco com a funcao strtok e conta quantos tem
	s = strtok(stmt_bkp, " ");	
	while(s) {	
		i++;
		s = strtok(NULL, " ");
	}
	// aloca memoria com base na quantidade de argumentos encontrados
	retorno = malloc((i + 1) * sizeof(char*));
	
	i = 0;
	s = strtok(stmt, " ");
	
	//adiciona os argumentos que foram separados, em um ponteiro de char indexado pela posicao i, de entrada 
	while(s) {	
		retorno[i++] = s;
		s = strtok(NULL, " ");
	}
	//atribui nulo na ultima posição do vetor
	retorno[i] = NULL;
	
	return retorno;

}

// funcao que separa os argumentos pelo "/"
char** listaDiretorios(char* stmt) {

	int i = 1;
	char** retorno;
	char* s;
	char stmt_bkp[MAX_STMT];
	
	//realiza uma copia do stmt
	strcpy(stmt_bkp, stmt);
	
	//separa o stmt  pelo espaco com a funcao strtok e conta quantos tem
	s = strtok(stmt_bkp, "/");	
	while(s) {	
		i++;
		s = strtok(NULL, "/");
	}
	// aloca memoria com base na quantidade de argumentos encontrados
	retorno = malloc((i + 1) * sizeof(char*));
	
	i = 0;
	s = strtok(stmt, "/");
	
	//adiciona os argumentos que foram separados, em um ponteiro de char indexado pela posicao i, de entrada 
	while(s) {	
		retorno[i++] = s;
		s = strtok(NULL, "/");
	}
	//atribui nulo na ultima posição do vetor
	retorno[i] = NULL;
	
	return retorno;

}


// estrutura da tabela do arquivo
typedef struct arquivo {
	struct arquivo *prox;
	int posicao;
} Arquivo;

// estrutura da bloco
typedef struct bloco {
	char nome[100];
	int tamanho;
	char data[20];
	Arquivo *arq;
	int posicao;
	struct bloco *prox;
	struct bloco *filho;
} Bloco;

int blocosLivres[QTD_SETORES];
int espacosLivres = QTD_SETORES - 10;
Bloco *raiz;
Bloco *anterior;
Bloco *pai;

// função que libera bloco
void liberaBloco(int i) {
     blocosLivres[i] = 1;
     espacosLivres++;
}

// função que remove os blocos do arquivo recursivamente
void removeArquivo(Arquivo* arq) {
     if (arq->prox!=NULL) removeArquivo(arq->prox);
     liberaBloco(arq->posicao);
     free(arq);
}

// função que encontra o primeiro bloco disponivel e aloca
int pegaBloco() {
	int i;
	
	for(i=10; i<QTD_SETORES; i++) {
		if(blocosLivres[i]) {
			blocosLivres[i] = 0;
			espacosLivres--;
			return i;
		}
	}
	return -1;
}

// função que localiza o bloco de um diretorio, dado seu nome e pai (raiz)
Bloco* localizaDir(Bloco* inicio, char nome[]) {
	Bloco* b;
	pai = inicio;
	b = inicio->filho;
	anterior = NULL;
	while (b!=NULL) {
		//printf("b:%s\nnome:%s\n",b->nome,nome);
		//b->arq==NULL -> certifica que é diretorio
		if(b->arq==NULL && strcmp(b->nome, nome)==0) {
			return b;
		}
		anterior = b;
		b = b->prox;
	}
	return NULL;
}

// função que localiza o bloco de um arquivo, dado seu nome e pai (raiz)
Bloco* localizaArq(Bloco* inicio, char nome[]) {
	Bloco* b;
	pai = inicio;
	b = inicio->filho;
	anterior = NULL;
	while (b!=NULL) {
		//printf("b:%s\nnome:%s\n",b->nome,nome);
		//b->arq!=NULL -> certifica que é arquivo
		if(b->arq!=NULL && strcmp(b->nome, nome)==0) {
			return b;
		}
		anterior = b;
		b = b->prox;
	}
	return NULL;
}

// imprime a arvore de diretorios de maneira recursiva
void imprimiLvlArvore(Bloco* bloco, int nivel) {
     Bloco* atual;
     atual = bloco;
     int nNivel,i;
     nNivel = nivel;
     if(atual->arq==NULL) printf(" |--%s\n",atual->nome);
     if(atual->arq==NULL && bloco->filho!=NULL) {
         nNivel++;
         for(i=0; i<nivel; i++)
             printf(" | ");
         imprimiLvlArvore(atual->filho, nNivel);
         nNivel--;
     }
     atual = atual->prox;
     if(atual!=NULL) {
         //printf("\nproximo.nNivel:%d\n",nNivel);
         for(i=0; i<nNivel-1; i++)
             printf(" | ");
         imprimiLvlArvore(atual, nNivel);
     }
}

int main() {
	
	int i;
	
	// iniciando a lista de blocos livres
	// 10 primeiros setores reservados
	for(i=0; i<QTD_SETORES; i++) {
		if(i<10) {
			blocosLivres[i] = 0;
		} else {
			blocosLivres[i] = 1;
		}
	}
	
	// iniciando raiz
	raiz = (Bloco*) malloc (sizeof(Bloco));
	strcpy(raiz->nome, "raiz");
	raiz->arq = NULL; // arq NULL = diretorio
	raiz->prox = NULL; //raiz não tem ngm no nivel
	raiz->filho = NULL; // nenhum filho por enquanto
	
	char stmt[MAX_STMT], stmt_bkp[MAX_STMT], argumentos[MAX_STMT];
	
	char **argList;
	char **dirList;
	
	Bloco* atual;
	Bloco* temp;
	int naoEncontrado, j, pos;
	naoEncontrado = 0;
		
	Bloco *novoBloco;
    Arquivo* arq;
    Arquivo* novoArq;
    Arquivo* tempArq;
    int qtdBlocos;
    int tamanhoArq;
    int verSet[QTD_SETORES];
    int totalArq, totalDir, tamanhoLivre, tamArq;
    char* saida;
	
	printf("------------------------------------------------\n");
	printf("|   Simulador - Sistema de Arquivos            |\n");
	printf("| -> Digite: \"ajuda\" para ver os comandos      |\n");
	printf("------------------------------------------------\n\n");
	
	//funcao que exibe o prompt de comando para o usuario
	prompt();
	
	while(1) {
		
		fflush(stdin);
		gets(stmt);
		
		//tratativa para o CTRL+D, se chegou no fim do arquivo sai como exit
		if (feof(stdin)) {
		  printf("\n");
		  exit(0);
		}
		
		//salva o valor do stmt digitado pelo usuario
		strcpy(stmt_bkp, stmt);
		
		argList = NULL;
		//obtem o argumento do stmt
		argList = listaArgumentos(stmt_bkp);
		if(argList[1]!=NULL) strcpy(argumentos, argList[1]);
		
		//printf("argList[0]:%s, argList[1]:%s\n", argList[0], argList[1]);
		
		// se o argumento contiver 'sair', sai do programa
		if(!strcmp(argList[0], "sair")) exit(0);
		
		// se o argumento contiver 'ajuda', exibe help do programa
		if(!strcmp(argList[0], "ajuda")) {
			printf("\n criad   [path/]diretorio         -> cria diretorio");
			printf("\n criaa   [path/]arquivo tamanho   -> cria arquivo   ");
			printf("\n removed [path/]diretorio         -> remove diretorio");
			printf("\n removea [path/]arquivo           -> remove arquivo");
			printf("\n verd    [path]                   -> mostra diretorio");
			printf("\n mapa                             -> mostra tabela de setores");
			printf("\n arvore                           -> mostra a arvore de diretorios");
			printf("\n verset  [path/]arquivo           -> mostra setores ocupados pelo arquivo");
			printf("\n sair                             -> finaliza o programa \n\n");
			
            prompt();
		    continue;
		}
		
		// arvore de diretorios
		if(!strcmp(argList[0], "arvore")) {
            atual = raiz->filho;
            if(atual!=NULL) {
                printf("\nraiz\n");
                imprimiLvlArvore(atual, 1);
                printf("\n");
            }
            
            prompt();
		    continue;
        }
        
        
		
		// mapa de livres e ocupados
		if(!strcmp(argList[0], "mapa")) {
			
			for(i=0; i<QTD_SETORES; i++) {
				if(i<10) {
					printf(" ");
				} else {
					if(blocosLivres[i]) {
						printf("_.");
					} else {
						printf("#.");
					}
				}
			}
			printf("\n\n");
			
			prompt();
			continue;
		}
		
		
		// mostra os setores ocupado por um arquivo
		if(!strcmp(argList[0], "verset")) {
                               
             //inicia vetor para desenho do mapa
             for(i=0; i<QTD_SETORES; i++) {
        		if(i<10) {
        			verSet[i] = 0;
        		} else {
        			verSet[i] = 1;
        		}
        	 }
        	 
        	 // sem argumento
			if(argList[1]==NULL) {
				printf("Erro: digite o nome do arquivo\n\n");
				prompt();
				continue;
			}
			
			dirList = NULL;
			
			dirList = listaDiretorios(argList[1]);
			
			//printf("dirList[1]:%s\n",dirList[1]);
			
		    atual = raiz;		
			
			naoEncontrado = 0;
			j = 0;
			
			// se não for na raiz
			if(dirList[1]!=NULL) {
				do {
                    //localiza diretorio digitado
					atual = localizaDir(atual,dirList[j]);
					//printf("localizaDir:%s\n",atual->nome);
					if(atual==NULL) {
						printf("Diretorio nao encontrado: %s\n\n", argumentos);
						naoEncontrado = 1;
						break;
					}
					j++;
				} while(dirList[j+1]!=NULL);
				
			}
			
			// localiza arquivo final
            if(!naoEncontrado) {
                temp = localizaArq(atual,dirList[j]);
                if(temp!=NULL) {
                       tempArq = temp->arq;
                       while(tempArq!=NULL) {
                            verSet[tempArq->posicao] = 0;
                            tempArq = tempArq->prox;
                       }
                       
                       printf("Setores ocupados pelo arquivo: %s\n", argumentos);
                       
                       for(i=0; i<QTD_SETORES; i++) {
            				if(i<10) {
            					printf(" ");
            				} else {
            					if(verSet[i]) {
            						printf("_.");
            					} else {
            						printf("#.");
            					}
            				}
            			}
            			printf("\n\n");
            			
				} else {
                       printf("Arquivo nao encontrado: %s\n\n", argumentos);
                }
            }
            
				
				prompt();
				continue;
        }
		
		// mostra as informacoes do diretorio
		if(!strcmp(argList[0], "verd")) {
			naoEncontrado = 0;
			totalArq=0;
            totalDir=0;
            tamanhoLivre = espacosLivres*TAMANHO_SETOR;
            tamArq=0;
			
			atual = raiz;  // se não digitar o caminho, lista a raiz
			
			if(argList[1]!=NULL) {
                   
                // encontra o diretorio digitado
                
    			dirList = NULL;
    			dirList = listaDiretorios(argList[1]);
    			j = 0;

    				do {
    					atual = localizaDir(atual,dirList[j]);
    					if(atual==NULL) {
    						printf("Diretorio nao encontrado: %s!\n\n", argumentos);
    						naoEncontrado = 1;
    						break;
    					}
    					j++;
    				} while(dirList[j]!=NULL);
            }
            
            if(!naoEncontrado) {
                atual = atual->filho;
                
                if(atual==NULL) {
                     printf("Nenhum arquivo ou diretorio encontrado para listar.\n\n");
                     prompt();
				     continue;
                }
                //lista arquivos e diretorios
               while(atual!=NULL) {
                     if(atual->arq==NULL) { 
                          // então é um diretorio
                          printf("%s    <DIR>    %s\n",atual->data, atual->nome );
                          totalDir++;
                     } else {
                          // arquivo  
                          printf("%s    %d    %s\n",atual->data, atual->tamanho, atual->nome );
                          totalArq++;
                          tamArq += atual->tamanho;
                     }
                     atual = atual->prox;
                }
                printf("%d arquivo(s)     %d bytes\n", totalArq, tamArq);
                printf("%d diretorio(s)   %d bytes disponiveis\n", totalDir, tamanhoLivre);
                printf("\n");
                prompt();
				continue;
            }
		}
		
		// cria diretorio
		if(!strcmp(argList[0], "criad")) {
			// sem argumento
			if(argList[1]==NULL) {
				printf("Erro: digite o nome do diretorio\n\n");
				prompt();
				continue;
			}
			
			dirList = NULL;
			
			dirList = listaDiretorios(argList[1]);
			
			//printf("dirList[1]:%s\n",dirList[1]);
			
		    atual = raiz;		
			
			naoEncontrado = 0;
			j = 0;
			
			// se não for na raiz
			if(dirList[1]!=NULL) {
				do {
                    //localiza diretorio digitado
					atual = localizaDir(atual,dirList[j]);
					//printf("localizaDir:%s\n",atual->nome);
					if(atual==NULL) {
						printf("Diretorio nao encontrado: %s\n\n", argumentos);
						naoEncontrado = 1;
						break;
					}
					j++;
				} while(dirList[j+1]!=NULL);
				
			}
			
			// testa duplicidade
            if(!naoEncontrado) {
                //printf("testa dup:%s\n",dirList[j]);
                temp = localizaDir(atual,dirList[j]);
                if(temp!=NULL) {
			           printf("Ja existe um diretorio com esse nome neste caminho: %s\n\n", argumentos);
                       naoEncontrado = 1;
                       prompt();
                       continue;
				}
            }
            
            // tenta alocar o espaço pro diretorio
			if(!naoEncontrado) {
                pos = pegaBloco();
				
    			if(pos==-1) {
    				printf("Erro: não há mais espaço!\n\n");
    				naoEncontrado = 1;
    				prompt();
				    continue;
    			}
            }
            
			if(!naoEncontrado) {
                //printf("criando em :%s\n",atual->nome);
                
				Bloco *novoBloco = (Bloco*) malloc (sizeof(Bloco));
				novoBloco->prox = atual->filho;
				strcpy(novoBloco->nome, dirList[j]);
				strcpy(novoBloco->data, pegaData());
				novoBloco->filho = NULL;
				novoBloco->arq = NULL;
				novoBloco->posicao = pos;
				atual->filho = novoBloco;
				//printf("nome:%s,data:%s\n",novoBloco->nome,novoBloco->data);
				
				prompt();
				continue;
			}
		}
		
		// remove diretorio
		if(!strcmp(argList[0], "removed")) {
			// sem argumento
			if(argList[1]==NULL) {
				printf("Erro: digite o nome do diretorio\n\n");
				prompt();
				continue;
			}
			
			dirList = NULL;
			
			dirList = listaDiretorios(argList[1]);
			
			//printf("dirList[1]:%s\n",dirList[1]);
			
		    atual = raiz;		
			
			naoEncontrado = 0;
			j = 0;
			
			// se não for na raiz
			if(dirList[1]!=NULL) {
				do {
                    //localiza diretorio digitado
					atual = localizaDir(atual,dirList[j]);
					//printf("localizaDir:%s\n",atual->nome);
					if(atual==NULL) {
						printf("Diretorio nao encontrado: %s\n\n", argumentos);
						naoEncontrado = 1;
						break;
					}
					j++;
				} while(dirList[j+1]!=NULL);
				
			}
			
			// localiza dir final
            if(!naoEncontrado) {
                temp = localizaDir(atual,dirList[j]);
                if(temp!=NULL) {
                       //verifica se está vazio
                       if(temp->filho!=NULL) {
                           printf("Nao foi possivel remover este diretorio, pois ele nao esta vazio.\n\n");
            			   prompt();
            			   continue;
                       }
                               
			           if(anterior!=NULL) {
                           // nao é o primeiro item da lista: remocao do meio e fim
                           anterior->prox = temp->prox;
                       } else {
                           //primeiro da lista: remocao no inicio
                           pai->filho = temp->prox;
                       }
                       liberaBloco(temp->posicao);
			           free(temp);
			           printf("Diretorio %s removido\n\n", argumentos);
				} else {
                       printf("Diretorio nao encontrado: %s\n\n", argumentos);
                }
            }
            
				
				prompt();
				continue;
		}
		
		// cria arquivo
		if(!strcmp(argList[0], "criaa")) {
            // sem argumento
			if(argList[1]==NULL || argList[2]==NULL) {
				printf("Erro: digite o nome e o tamanho do arquivo\n criaa [path/]arquivo tamanho \n\n");
				prompt();
				continue;
			}
			
            
			atual = raiz;
			
			dirList = NULL;
			
			dirList = listaDiretorios(argList[1]);
			
			//printf("dirList[1]:%s\n",dirList[1]);
		
			
			naoEncontrado = 0;
			j = 0;
			
			// se não for na raiz, localiza Dir
			if(dirList[1]!=NULL) {
				do {
                    //localiza diretorio digitado
					atual = localizaDir(atual,dirList[j]);
					//printf("localizaDir:%s\n",atual->nome);
					if(atual==NULL) {
						printf("Diretorio nao encontrado: %s\n\n", argumentos);
						naoEncontrado = 1;
						break;
					}
					j++;
				} while(dirList[j+1]!=NULL);
				
			}
			
			// testa duplicidade
            if(!naoEncontrado) {
                //printf("testa dup:%s\n",dirList[j]);
                temp = localizaArq(atual,dirList[j]);
                if(temp!=NULL) {
			           printf("Ja existe um arquivo com esse nome neste caminho: %s\n\n", argumentos);
                       naoEncontrado = 1;
                       prompt();
                       continue;
				}
            }
            
            // aloca a tabela de arquivos
			if(!naoEncontrado) {
                // calcula a qtd de blocos que o arq vai ocupar
                tamanhoArq = atoi(argList[2]);
                qtdBlocos = (tamanhoArq / TAMANHO_SETOR) + 1;
                if(qtdBlocos > espacosLivres) {
                    printf("Nao ha mais espaco suficiente!\n\n");
    				naoEncontrado = 1;
    				prompt();
    			    continue;
                }
                
                arq = (Arquivo*) malloc (sizeof(Arquivo));
                pos = pegaBloco();
                arq->posicao = pos;
                arq->prox = NULL;
                
                for(i=1; i<qtdBlocos; i++) {
    				novoArq = (Arquivo*) malloc (sizeof(Arquivo));
    				novoArq->posicao = pegaBloco();
    				novoArq->prox = arq->prox;
    				arq->prox = novoArq;
    				
        			if(pos==-1) {
        				printf("Erro: nao ha mais espaco!\n\n");
        				naoEncontrado = 1;
        				prompt();
    				    break;
        			}
                }
            }
            
			if(!naoEncontrado) {
                //printf("criando em :%s\n",atual->nome);
                
				novoBloco = (Bloco*) malloc (sizeof(Bloco));
				novoBloco->prox = atual->filho;
				strcpy(novoBloco->nome, dirList[j]);
				strcpy(novoBloco->data, pegaData());
				novoBloco->filho = NULL;
				novoBloco->arq = arq;
				novoBloco->tamanho = tamanhoArq;
				novoBloco->posicao = pos;
				atual->filho = novoBloco;
				//printf("nome:%s,data:%s\n",novoBloco->nome,novoBloco->data);
			}
			
		    	prompt();
				continue;
		}
		
		// remove arquivo
		if(!strcmp(argList[0], "removea")) {
			// sem argumento
			if(argList[1]==NULL) {
				printf("Erro: digite o nome do arquivo\n\n");
				prompt();
				continue;
			}
			
			dirList = NULL;
			
			dirList = listaDiretorios(argList[1]);
			
			//printf("dirList[1]:%s\n",dirList[1]);
			
		    atual = raiz;		
			
			naoEncontrado = 0;
			j = 0;
			
			// se não for na raiz
			if(dirList[1]!=NULL) {
				do {
                    //localiza diretorio digitado
					atual = localizaDir(atual,dirList[j]);
					//printf("localizaDir:%s\n",atual->nome);
					if(atual==NULL) {
						printf("Diretorio nao encontrado: %s\n\n", argumentos);
						naoEncontrado = 1;
						break;
					}
					j++;
				} while(dirList[j+1]!=NULL);
				
			}
			
			// localiza arquivo final
            if(!naoEncontrado) {
                temp = localizaArq(atual,dirList[j]);
                if(temp!=NULL) {
                       // remove todos blocos de arquivos recursivamente
                       removeArquivo(temp->arq);
                       
			           if(anterior!=NULL) {
                           // nao é o primeiro item da lista: remocao do meio e fim
                           anterior->prox = temp->prox;
                       } else {
                           //primeiro da lista: remocao no inicio
                           pai->filho = temp->prox;
                       }
			           free(temp);
			           printf("Arquivo %s removido\n\n", argumentos);
				} else {
                       printf("Arquivo nao encontrado: %s\n\n", argumentos);
                }
            }
            
				
				prompt();
				continue;
		}
		
		
		// se chegou aqui é pq nenhum comando foi reconhecido
		printf("Comando Invalido!\nPara listar os comandos validos digite: ajuda\n\n");
		prompt();
	} // fim while

}
