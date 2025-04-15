/*******************************************************************************
 * SERVIDOR PARA SISTEMA DE TRANSFERÊNCIA DE ARQUIVOS
 * 
 * Descrição: Implementa o servidor para transferência de arquivos via sockets TCP/IP
 *            que gerencia operações de arquivos remotos.
 * 
 * Funcionalidades:
 * - Aceita conexões de múltiplos clientes
 * - Gerencia upload/download de arquivos
 * - Lista arquivos disponíveis
 * - Remove arquivos do servidor
 * - Suporte a caracteres acentuados e Unicode
 * 
 * Autor: [Seu Nome]
 * Data: [Data]
 * Versão: 1.0
 ******************************************************************************/

/*--------------------------------------------------------------
 * INCLUSÕES DE BIBLIOTECAS
 *------------------------------------------------------------*/
#include <stdio.h>      // Para funções de entrada/saída padrão
#include <stdlib.h>     // Para alocação de memória e outras utilidades
#include <string.h>     // Para manipulação de strings
#include <winsock2.h>   // Para sockets no Windows
#include <windows.h>    // Para funções específicas do Windows
#include <direct.h>     // Para manipulação de diretórios
#include <locale.h>     // Para configuração de localização (acentos)

// Linkar com a biblioteca de sockets do Windows
#pragma comment(lib, "ws2_32.lib")

/*--------------------------------------------------------------
 * DEFINIÇÕES DE CONSTANTES
 *------------------------------------------------------------*/
#define PORT 8888               // Porta padrão para conexão
#define BUFFER_SIZE 1024        // Tamanho do buffer para transferência
#define SERVER_STORAGE "C:\\Users\\ld388\\Desktop\\SD\\server_storage" // Diretório de armazenamento
#define MAX_CONNECTIONS 3       // Número máximo de conexões simultâneas

/*--------------------------------------------------------------
 * DECLARAÇÕES DE FUNÇÕES
 *------------------------------------------------------------*/

/**
 * Configura a codificação do console para suportar UTF-8 e acentos
 * 
 * Por que foi feito:
 * - Garantir que caracteres acentuados sejam exibidos corretamente
 * - Configurar o locale para português do Brasil
 */
void set_console_encoding() {
    system("chcp 65001 > nul");     // Muda a página de código para UTF-8
    SetConsoleOutputCP(CP_UTF8);    // Configura saída do console para UTF-8
    SetConsoleCP(CP_UTF8);          // Configura entrada do console para UTF-8
    setlocale(LC_ALL, "Portuguese_Brazil.1252"); // Configura localização
}

/**
 * Cria o diretório de armazenamento do servidor se não existir
 * 
 * Por que foi feito:
 * - Garantir que o local para armazenar arquivos exista
 * - Evitar erros ao tentar salvar arquivos recebidos
 */
void create_storage_directory() {
    if (_access(SERVER_STORAGE, 0) == -1) {
        _mkdir(SERVER_STORAGE); // Cria o diretório se não existir
        printf("Diretório de armazenamento criado: %s\n", SERVER_STORAGE);
    }
}

/**
 * Lista arquivos disponíveis no servidor e envia ao cliente
 * 
 * @param client_socket Socket conectado ao cliente
 * 
 * Por que foi feito:
 * - Permitir que clientes vejam quais arquivos estão disponíveis
 * - Interface consistente com o cliente
 */
void list_files(SOCKET client_socket) {
    WIN32_FIND_DATA findFileData;  // Estrutura para dados do arquivo
    HANDLE hFind;                  // Handle para busca
    char file_list[BUFFER_SIZE] = {0}; // Buffer para lista de arquivos
    char searchPath[MAX_PATH];     // Caminho para busca
    
    // Prepara o caminho de busca (todos os arquivos)
    sprintf(searchPath, "%s\\*", SERVER_STORAGE);
    
    // Inicia a busca
    hFind = FindFirstFile(searchPath, &findFileData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        send(client_socket, "Nenhum arquivo encontrado.", 25, 0);
        return;
    }
    
    // Constrói a lista de arquivos, ignorando "." e ".."
    do {
        if (strcmp(findFileData.cFileName, ".") != 0 && 
            strcmp(findFileData.cFileName, "..") != 0) {
            strcat(file_list, findFileData.cFileName);
            strcat(file_list, "\n"); // Separa por linhas
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    
    FindClose(hFind); // Libera o handle de busca
    
    // Envia a lista para o cliente
    send(client_socket, file_list, strlen(file_list), 0);
}

/**
 * Recebe um arquivo enviado pelo cliente e armazena no servidor
 * 
 * @param client_socket Socket conectado ao cliente
 * @param filename Nome do arquivo a ser recebido
 * 
 * Por que foi feito:
 * - Permitir upload de arquivos para o servidor
 * - Armazenar dados recebidos de forma confiável
 */
void upload_file(SOCKET client_socket, char *filename) {
    char filepath[MAX_PATH];
    // Constrói o caminho completo do arquivo
    sprintf(filepath, "%s\\%s", SERVER_STORAGE, filename);
    
    // Abre o arquivo para escrita binária
    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        send(client_socket, "Erro ao criar arquivo.", 22, 0);
        return;
    }
    
    char buffer[BUFFER_SIZE];
    int bytes_received;
    long total_received = 0;
    
    // Recebe dados em chunks até a conexão ser fechada
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        total_received += bytes_received;
    }
    
    fclose(file);
    // Envia confirmação para o cliente
    send(client_socket, "Upload concluído com sucesso.", 29, 0);
    printf("Arquivo recebido: %s (%ld bytes)\n", filename, total_received);
}

/**
 * Envia um arquivo solicitado para o cliente
 * 
 * @param client_socket Socket conectado ao cliente
 * @param filename Nome do arquivo a ser enviado
 * 
 * Por que foi feito:
 * - Permitir download de arquivos do servidor
 * - Transferência eficiente em chunks
 */
void download_file(SOCKET client_socket, char *filename) {
    char filepath[MAX_PATH];
    // Constrói o caminho completo do arquivo
    sprintf(filepath, "%s\\%s", SERVER_STORAGE, filename);
    
    // Abre o arquivo para leitura binária
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        send(client_socket, "Arquivo não encontrado.", 23, 0);
        return;
    }
    
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    // Lê e envia o arquivo em chunks
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) == SOCKET_ERROR) {
            printf("Erro ao enviar arquivo.\n");
            break;
        }
    }
    
    fclose(file);
    printf("Arquivo enviado: %s\n", filename);
}

/**
 * Remove um arquivo do servidor
 * 
 * @param client_socket Socket conectado ao cliente
 * @param filename Nome do arquivo a ser removido
 * 
 * Por que foi feito:
 * - Permitir exclusão remota de arquivos
 * - Feedback sobre sucesso/falha da operação
 */
void delete_file(SOCKET client_socket, char *filename) {
    char filepath[MAX_PATH];
    // Constrói o caminho completo do arquivo
    sprintf(filepath, "%s\\%s", SERVER_STORAGE, filename);
    
    // Tenta deletar o arquivo e envia resposta apropriada
    if (DeleteFile(filepath)) {
        send(client_socket, "Arquivo excluído com sucesso.", 28, 0);
        printf("Arquivo excluído: %s\n", filename);
    } else {
        send(client_socket, "Erro ao excluir arquivo.", 23, 0);
        printf("Falha ao excluir: %s\n", filename);
    }
}

/*******************************************************************************
 * FUNÇÃO PRINCIPAL
 ******************************************************************************/
int main() {
    // Configura o console para suportar acentos e caracteres especiais
    set_console_encoding();
    
    // Variáveis para controle do servidor
    WSADATA wsa;                    // Estrutura para inicialização do Winsock
    SOCKET server_socket,          // Socket principal do servidor
           client_socket;          // Socket para comunicação com cliente
    struct sockaddr_in server,     // Estrutura com dados do servidor
                      client;      // Estrutura com dados do cliente
    int client_size;               // Tamanho da estrutura do cliente
    char buffer[BUFFER_SIZE];      // Buffer para comunicação
    
    /*--------------------------------------------------------------
     * INICIALIZAÇÃO DO WINSOCK
     *------------------------------------------------------------*/
    printf("Inicializando Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Falha. Código de erro: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Inicializado.\n");
    
    /*--------------------------------------------------------------
     * CRIAÇÃO DO SOCKET DO SERVIDOR
     *------------------------------------------------------------*/
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Não foi possível criar o socket: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Socket criado.\n");
    
    /*--------------------------------------------------------------
     * CONFIGURAÇÃO DO ENDEREÇO DO SERVIDOR
     *------------------------------------------------------------*/
    server.sin_family = AF_INET;           // Família IPv4
    server.sin_addr.s_addr = INADDR_ANY;   // Aceita conexões de qualquer IP
    server.sin_port = htons(PORT);         // Porta configurada
    
    /*--------------------------------------------------------------
     * VINCULAÇÃO DO SOCKET AO ENDEREÇO
     *------------------------------------------------------------*/
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Erro ao vincular. Código de erro: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Vinculação concluída.\n");
    
    /*--------------------------------------------------------------
     * COLOCA O SERVIDOR EM MODO DE ESCUTA
     *------------------------------------------------------------*/
    listen(server_socket, MAX_CONNECTIONS);
    printf("Aguardando conexões na porta %d...\n", PORT);
    
    /*--------------------------------------------------------------
     * CRIA O DIRETÓRIO DE ARMAZENAMENTO
     *------------------------------------------------------------*/
    create_storage_directory();
    
    /*--------------------------------------------------------------
     * LOOP PRINCIPAL - ACEITA CONEXÕES DE CLIENTES
     *------------------------------------------------------------*/
    client_size = sizeof(struct sockaddr_in);
    
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client, &client_size)) != INVALID_SOCKET) {
        printf("\nConexão aceita de %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        
        /*--------------------------------------------------------------
         * LOOP DE COMUNICAÇÃO COM O CLIENTE
         *------------------------------------------------------------*/
        while (1) {
            // Recebe comando do cliente
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            
            // Verifica se cliente desconectou
            if (bytes_received <= 0) {
                printf("Cliente desconectado.\n");
                break;
            }
            
            buffer[bytes_received] = '\0'; // Garante terminação da string
            printf("Comando recebido: %s\n", buffer);
            
            /*--------------------------------------------------------------
             * PROCESSAMENTO DE COMANDOS
             *------------------------------------------------------------*/
            if (strncmp(buffer, "LIST", 4) == 0) {
                // Lista arquivos disponíveis
                list_files(client_socket);
            } 
            else if (strncmp(buffer, "UPLOAD ", 7) == 0) {
                // Recebe upload de arquivo (remove "UPLOAD " do buffer)
                char *filename = buffer + 7;
                upload_file(client_socket, filename);
            } 
            else if (strncmp(buffer, "DOWNLOAD ", 9) == 0) {
                // Envia arquivo solicitado (remove "DOWNLOAD " do buffer)
                char *filename = buffer + 9;
                download_file(client_socket, filename);
            } 
            else if (strncmp(buffer, "DELETE ", 7) == 0) {
                // Remove arquivo (remove "DELETE " do buffer)
                char *filename = buffer + 7;
                delete_file(client_socket, filename);
            } 
            else if (strncmp(buffer, "EXIT", 4) == 0) {
                // Encerra conexão com este cliente
                printf("Cliente solicitou desconexão.\n");
                break;
            } 
            else {
                // Comando não reconhecido
                send(client_socket, "Comando inválido.", 17, 0);
            }
        }
        
        /*--------------------------------------------------------------
         * FINALIZAÇÃO DA CONEXÃO COM O CLIENTE
         *------------------------------------------------------------*/
        closesocket(client_socket);
    }
    
    /*--------------------------------------------------------------
     * TRATAMENTO DE ERROS DE ACEITAÇÃO
     *------------------------------------------------------------*/
    if (client_socket == INVALID_SOCKET) {
        printf("Erro ao aceitar conexão. Código de erro: %d\n", WSAGetLastError());
    }
    
    /*--------------------------------------------------------------
     * FINALIZAÇÃO DO SERVIDOR
     *------------------------------------------------------------*/
    closesocket(server_socket);
    WSACleanup();
    return 0;
}