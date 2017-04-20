#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <signal.h>
#include <sys/wait.h>

// tamanho maximo de stmt
#define MAX_STMT 255

// variavel de controle de um statement
sig_atomic_t sinal;
int qtdStmt;

//se chegar um dos sinais de  CTRL+C ou CTRL+Z muda o valor da flag sinal
void handler (int signal_number)
{
  sinal = 1;
}


//executa o comando (program), tendo como entrada pIn e saida pOut
int spawn(char* program, char** arg_list, int pIn, int pOut) {

	pid_t child_pid;
	
	//cria um processo filho
	child_pid = fork();
	//se pai
	if (child_pid != 0) {
		if(pIn!=-1) close(pIn);
		if(pOut!=-1) close(pOut); 
    	//retorna o pid do processo filho
		return child_pid;
	} else {
		// Se o descritor de arquivo de entrada for informado, redireciona a entrada para este descritor e o fecha
		if(pIn!=-1) {
			dup2(pIn, 0);
			close(pIn);
			//printf("\n\npIn:%d\n",pIn);
		} 
		// Se o descritor de arquivo de saida for informado, redireciona a saÃ­da para este descritor e o fecha
		if(pOut!=-1) {
			dup2(pOut, 1);
			close(pOut);
			//printf("pOut:%d\n\n",pOut);
		}
		
		//printf("programa:%s\n",program);
		//executa o comando  com o argumento passados
		execvp (program, arg_list);
		//se ocorreu uma falha na execucao do comando
		fprintf (stderr, "Error: comando não encontrado.\n");
		abort();
	}

}
//funcao que quebra a string, exemplo: cwd: /home/users/mburity/SOII
// search: /home/users/mburity
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
// funcao que obtem o diretorio atual
char* diretorio() {

	char *home = getenv("HOME");	
	char cwd[255];
	// funcao que copia o caminho absoluto do diretorio atual para cwd
	getcwd(cwd, 255);
	
	//troca home por ~
	return replace_str(cwd, home, "~");
}

//funcao que monta o prompt de comando através dos parametros passados: usuario, host e diretorio
void prompt(char* user, char* host, char* wd) {
	printf("[MySh] %s@%s:%s$ ", user, host, wd);
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
//funcao que recebe o stmt que o usuario digitou e separa a cada pipe os comandos que foram dados 
char** listaStmt(char* stmt) {

	int i = 1;
	char** retorno;
	char* s;
	char stmt_bkp[MAX_STMT];
	
	strcpy(stmt_bkp, stmt);
	
	// quebra o stmt passado, contando quantos pipes foram dados
	s = strtok(stmt_bkp, "|");	
	while(s) {	
		i++;
		s = strtok(NULL, "|");
	}
	
	//reserva espaço de memoria baseada na quantidade de | encontrados
	retorno = malloc((i + 1) * sizeof(char*));
	
	i = 0;
	s = strtok(stmt, "|");
	//adiciona os stmt que foram separados, em um ponteiro de char indexado pela posicao i, de entrada
	while(s) {
		retorno[i++] = s;
		s = strtok(NULL, "|");
	}
	//salva a quantidade de stmt em qtdStmt
	qtdStmt = i;
	
	//atribui nulo na ultima posição do vetor
	retorno[i] = NULL;
	
	return retorno;
}

int main() {
	
	struct sigaction sa,sb;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &handler;
	// tratamento para os sinais CTRL+C e CTRL+Z
	// se chegar um dos dois chama funcao handler que muda valor da flag sinal
	sigaction (SIGINT, &sa, NULL);
	sigaction (SIGTSTP, &sa, NULL);

	char stmt[MAX_STMT], stmt_bkp[MAX_STMT];
	
	char **argList;
	char **stmtList;
    // ponteiro que encontra o usuario
	char *user = getenv("USER");
	// encontra o hostname
	char host[255];
	gethostname(host, 255);
	//encontra o diretorio atual
	char *wd = diretorio();
	
	//criacao do pipe 
	int pipes[MAX_STMT][2];
	pid_t pids[MAX_STMT];
	
	//funcao que exibe o prompt de comando para o usuario
	prompt(user, host, wd);
	
	while(1) {
		sinal = 0;
		qtdStmt = 0;
		
		fflush(stdin);
		gets(stmt);
		
		//tratativa para o CTRL+D, se chegou no fim do arquivo sai como exit
		if (feof(stdin)) {
		  printf("\n");
		  exit(0);
		}
		
		//caso tenha recebido o sinal CTRL+C ou CTRL+Z a flag sinal é modificada para 1 para ser ignorado o sinal
		if(sinal==1) {
			printf("\n");
			prompt(user, host, wd);
			sinal = 0;
			argList = NULL;
			continue;
		}
		
		//salva o valor do stmt digitado pelo usuario
		strcpy(stmt_bkp, stmt);
		
		int stmtAtual = 0;
		//salva em stmtlist os comandos passados pelo usuario que foram separados pelo |
		stmtList = listaStmt(stmt);
		
		// cria um pipe para cada stmt
		for(stmtAtual=0; stmtAtual<qtdStmt; stmtAtual++) {
			pipe(pipes[stmtAtual]);
		}
		
		//busca em todos os stmt "comandos"
		for(stmtAtual=0; stmtAtual<qtdStmt; stmtAtual++) {
			argList = NULL;
			//obtem o argumento do stmt
			argList = listaArgumentos(stmtList[stmtAtual]);
			
			// se o argumento contiver 'exit', sai do programa
			if(!strcmp(argList[0], "exit")) exit(0);
			
			if(!strcmp(argList[0], "cd")) {
				
				// se o comando for cd ~, obtem a  home do usuario
				if(!argList[1] || strcmp(argList[1],"~")==0) argList[1] = getenv("HOME");
				
				//testa se o diretorio existe e muda de diretorio
				if(chdir(argList[1])) {
					printf("Erro: Arquivo ou diretÃ³rio nÃ£o encontrado.\n");
				}
				
				//obtem o novo diretorio
				wd = diretorio();
				
				//prompt(user, host, wd);
				
			} else if(strcmp(argList[0], "exit")) { 
				int status_id,pIn,pOut;
				//se possui mais de um stmt realiza as passadas da stdin e stdout dos pipes, caso contrario já executa o proprio comando
				
				pIn = -1;
				pOut = -1;
				
				// se mais de um stmt conecta os pipes
				if(qtdStmt>1) {
					//printf("Stmt:%d.\n",stmtAtual);
					
					// se nao for o primeiro stmt, conecta entrada
					if(stmtAtual>0) {
						pIn = pipes[stmtAtual-1][0];
					}
					// se nao for o ultimo, conecta saida
					if(stmtAtual<qtdStmt-1) {
						pOut = pipes[stmtAtual][1];
					}

				}
				
                //			
				pids[stmtAtual] = spawn(argList[0], argList, pIn, pOut);
				//wait(&status_id);
				//prompt(user, host, wd);
				
			}
		} // fim for
		
		
		int i;
		//espera todos os filhos terminarem
		for(i=0; i<qtdStmt; i++) { 
			waitpid(pids[i], NULL, 0);
		}
		
		prompt(user, host, wd);
	} // fim while

}
