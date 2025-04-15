/*******************************************************************************
 * CLIENTE PARA SISTEMA DE TRANSFERÊNCIA DE ARQUIVOS
 * 
 * Descrição: Implementa um cliente para transferência de arquivos via sockets TCP/IP
 *            com interface de console interativa.
 * 
 * Funcionalidades:
 * - Conexão com servidor remoto
 * - Listagem de arquivos no servidor e local
 * - Upload/download de arquivos com barra de progresso
 * - Exclusão de arquivos remotos
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
#include <conio.h>      // Para funções de console (getch, etc.)
#include <locale.h>     // Para configuração de localização (acentos)

// Linkar com a biblioteca de sockets do Windows
#pragma comment(lib, "ws2_32.lib")

/*--------------------------------------------------------------
 * DEFINIÇÕES DE CONSTANTES
 *------------------------------------------------------------*/
#define PORT 8888               // Porta padrão para conexão
#define BUFFER_SIZE 1024        // Tamanho do buffer para transferência
#define MAX_PATH 260            // Tamanho máximo de caminhos no Windows

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
 * Exibe uma barra de progresso gráfica no console
 * 
 * @param percentage Porcentagem completada (0-100)
 * 
 * Por que foi feito:
 * - Feedback visual durante transferências longas
 * - Melhor experiência do usuário
 */
void show_progress(int percentage) {
    printf("\r[");  // \r volta ao início da linha
    
    // Calcula posição do cursor na barra
    int pos = percentage / 5;
    
    // Desenha a barra
    for (int i = 0; i < 20; ++i) {
        if (i < pos) printf("=");       // Parte completada
        else if (i == pos) printf(">"); // Posição atual
        else printf(" ");               // Parte não completada
    }
    
    printf("] %d%%", percentage);  // Mostra porcentagem
    fflush(stdout);                // Força exibição imediata
}

/**
 * Exibe mensagem de conclusão de operação
 * 
 * @param operation Nome da operação realizada
 * @param filename Nome do arquivo envolvido
 * 
 * Por que foi feito:
 * - Feedback claro ao usuário sobre conclusão de operações
 */
void show_complete_message(const char* operation, const char* filename) {
    printf("\n%s %s concluído com sucesso!\n", operation, filename);
}

/**
 * Lista arquivos no diretório local
 * 
 * @param path Caminho do diretório a ser listado
 * 
 * Por que foi feito:
 * - Permitir ao usuário visualizar arquivos disponíveis para upload
 * - Interface amigável para navegação local
 */
void list_local_files(const char* path) {
    WIN32_FIND_DATA findFileData;  // Estrutura para dados do arquivo
    HANDLE hFind;                  // Handle para busca
    char searchPath[MAX_PATH];     // Caminho para busca
    
    // Prepara o caminho de busca (todos os arquivos)
    sprintf(searchPath, "%s\\*", path);
    
    // Inicia a busca
    hFind = FindFirstFile(searchPath, &findFileData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Nenhum arquivo encontrado no diretório.\n");
        return;
    }
    
    printf("\nArquivos no diretório local:\n");
    int count = 1;
    
    // Lista todos os arquivos, ignorando "." e ".."
    do {
        if (strcmp(findFileData.cFileName, ".") != 0 && 
            strcmp(findFileData.cFileName, "..") != 0) {
            printf("%d. %s\n", count++, findFileData.cFileName);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    
    FindClose(hFind);  // Libera o handle de busca
}

/**
 * Permite ao usuário selecionar um arquivo da lista local
 * 
 * @param path Diretório onde procurar arquivos
 * @param selectedFile Buffer para armazenar o nome do arquivo selecionado
 * @return 1 se seleção bem sucedida, 0 caso contrário
 * 
 * Por que foi feito:
 * - Interface interativa para seleção de arquivos
 * - Validação da entrada do usuário
 */
int select_file_from_list(const char* path, char* selectedFile) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    char searchPath[MAX_PATH];
    int fileCount = 0;
    int selectedIndex = 0;
    
    sprintf(searchPath, "%s\\*", path);
    hFind = FindFirstFile(searchPath, &findFileData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Nenhum arquivo encontrado no diretório.\n");
        return 0;
    }
    
    // Conta quantos arquivos existem
    do {
        if (strcmp(findFileData.cFileName, ".") != 0 && 
            strcmp(findFileData.cFileName, "..") != 0) {
            fileCount++;
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    
    FindClose(hFind);
    
    if (fileCount == 0) {
        printf("Nenhum arquivo encontrado no diretório.\n");
        return 0;
    }
    
    // Lista os arquivos com números para seleção
    hFind = FindFirstFile(searchPath, &findFileData);
    printf("\nSelecione um arquivo:\n");
    int count = 1;
    
    do {
        if (strcmp(findFileData.cFileName, ".") != 0 && 
            strcmp(findFileData.cFileName, "..") != 0) {
            printf("%d. %s\n", count++, findFileData.cFileName);
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    
    FindClose(hFind);
    
    // Obtém a seleção do usuário
    printf("\nDigite o número do arquivo: ");
    scanf("%d", &selectedIndex);
    getchar(); // Limpa o buffer do teclado
    
    // Valida a entrada
    if (selectedIndex < 1 || selectedIndex > fileCount) {
        printf("Seleção inválida.\n");
        return 0;
    }
    
    // Encontra o arquivo correspondente ao número selecionado
    hFind = FindFirstFile(searchPath, &findFileData);
    count = 1;
    
    do {
        if (strcmp(findFileData.cFileName, ".") != 0 && 
            strcmp(findFileData.cFileName, "..") != 0) {
            if (count++ == selectedIndex) {
                strcpy(selectedFile, findFileData.cFileName);
                FindClose(hFind);
                return 1;
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    
    FindClose(hFind);
    return 0;
}

/**
 * Obtém o caminho de destino para download
 * 
 * @param path Buffer para armazenar o caminho
 * 
 * Por que foi feito:
 * - Flexibilidade para o usuário escolher onde salvar arquivos
 * - Validação de existência do diretório
 */
void get_download_path(char* path) {
    printf("\nDigite o diretório de destino para o download (ou pressione Enter para usar o diretório atual): ");
    fgets(path, MAX_PATH, stdin);
    path[strcspn(path, "\n")] = '\0';  // Remove o \n do final
    
    // Se vazio, usa o diretório atual
    if (strlen(path) == 0) {
        GetCurrentDirectory(MAX_PATH, path);
    }
    
    // Verifica se o diretório existe
    if (_access(path, 0) == -1) {
        printf("Diretório não existe. Usando diretório atual.\n");
        GetCurrentDirectory(MAX_PATH, path);
    }
}

/**
 * Limpa o buffer de entrada
 * 
 * Por que foi feito:
 * - Evitar problemas com entradas pendentes no buffer
 * - Garantir leitura correta da próxima entrada
 */
void clear_input_buffer() {
    while (getchar() != '\n');
}

/*******************************************************************************
 * FUNÇÃO PRINCIPAL
 ******************************************************************************/
int main() {
    // Configura o console para suportar acentos e caracteres especiais
    set_console_encoding();
    
    // Variáveis para conexão
    WSADATA wsa;                    // Estrutura para inicialização do Winsock
    SOCKET s;                       // Socket para comunicação
    struct sockaddr_in server;      // Estrutura com dados do servidor
    
    // Buffers e variáveis de controle
    char buffer[BUFFER_SIZE];       // Buffer para transferência de dados
    char command[BUFFER_SIZE];      // Buffer para comandos
    char filename[MAX_PATH];        // Nome do arquivo
    char currentDir[MAX_PATH];      // Diretório atual
    char downloadPath[MAX_PATH];    // Caminho para download
    
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
     * CRIAÇÃO DO SOCKET
     *------------------------------------------------------------*/
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Não foi possível criar o socket: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Socket criado.\n");
    
    /*--------------------------------------------------------------
     * CONFIGURAÇÃO E CONEXÃO COM SERVIDOR
     *------------------------------------------------------------*/
    server.sin_addr.s_addr = inet_addr("127.0.0.1");  // IP do servidor
    server.sin_family = AF_INET;                      // Família IPv4
    server.sin_port = htons(PORT);                    // Porta
    
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Falha na conexão. Código de erro: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Conectado ao servidor.\n");
    
    // Obtém o diretório atual para operações locais
    GetCurrentDirectory(MAX_PATH, currentDir);
    
    /*--------------------------------------------------------------
     * LOOP PRINCIPAL - INTERFACE DO USUÁRIO
     *------------------------------------------------------------*/
    while (1) {
        // Exibe o menu de opções
        printf("\nComandos disponíveis:\n");
        printf("1. LIST - Listar arquivos no servidor\n");
        printf("2. UPLOAD - Enviar arquivo para o servidor\n");
        printf("3. DOWNLOAD - Baixar arquivo do servidor\n");
        printf("4. DELETE - Excluir arquivo no servidor\n");
        printf("5. LOCAL - Listar arquivos no diretório local\n");
        printf("6. EXIT - Desconectar do servidor\n");
        printf("Digite o número do comando: ");
        
        int choice;
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("Entrada inválida. Tente novamente.\n");
            continue;
        }
        clear_input_buffer();
        
        // Processa a escolha do usuário
        switch (choice) {
            case 1: { // LIST - Listar arquivos no servidor
                strcpy(command, "LIST");
                send(s, command, strlen(command), 0);
                
                // Limpa o buffer e recebe a lista de arquivos
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_received = recv(s, buffer, BUFFER_SIZE - 1, 0);
                
                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';
                    printf("\nArquivos no servidor:\n%s\n", buffer);
                }
                break;
            }
                
            case 2: { // UPLOAD - Enviar arquivo para o servidor
                int bytes_received;
                printf("\nDiretório atual: %s\n", currentDir);
                list_local_files(currentDir);
                
                if (select_file_from_list(currentDir, filename)) {
                    // Prepara o comando UPLOAD
                    sprintf(command, "UPLOAD %s", filename);
                    
                    // Abre o arquivo para leitura binária
                    FILE *file = fopen(filename, "rb");
                    if (file == NULL) {
                        printf("Arquivo não encontrado: %s\n", filename);
                        break;
                    }
                    
                    // Obtém o tamanho do arquivo
                    fseek(file, 0, SEEK_END);
                    long file_size = ftell(file);
                    fseek(file, 0, SEEK_SET);
                    
                    printf("\nEnviando %s (Tamanho: %ld bytes)\n", filename, file_size);
                    send(s, command, strlen(command), 0);
                    
                    long total_sent = 0;
                    size_t bytes_read;
                    
                    // Lê e envia o arquivo em chunks
                    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                        send(s, buffer, bytes_read, 0);
                        total_sent += bytes_read;
                        int progress = (int)((total_sent * 100) / file_size);
                        show_progress(progress);
                    }
                    
                    fclose(file);
                    shutdown(s, SD_SEND);  // Indica fim do envio
                    
                    // Aguarda confirmação do servidor
                    memset(buffer, 0, BUFFER_SIZE);
                    bytes_received = recv(s, buffer, BUFFER_SIZE - 1, 0);
                    if (bytes_received > 0) {
                        buffer[bytes_received] = '\0';
                        show_complete_message("Upload de", filename);
                        printf("Resposta do servidor: %s\n", buffer);
                    }
                }
                break;
            }
                
            case 3: { // DOWNLOAD - Baixar arquivo do servidor
                strcpy(command, "LIST");
                send(s, command, strlen(command), 0);
                
                // Recebe lista de arquivos disponíveis
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_received = recv(s, buffer, BUFFER_SIZE - 1, 0);
                if (bytes_received <= 0) {
                    printf("Erro ao receber lista de arquivos\n");
                    break;
                }
                buffer[bytes_received] = '\0';
                printf("\nArquivos disponíveis para download:\n%s\n", buffer);
                
                // Obtém nome do arquivo para download
                printf("Digite o nome do arquivo para download: ");
                fgets(filename, MAX_PATH, stdin);
                filename[strcspn(filename, "\n")] = '\0';
                
                if (strlen(filename) == 0) {
                    printf("Nome de arquivo inválido.\n");
                    break;
                }
                
                // Obtém diretório de destino
                get_download_path(downloadPath);
                char fullPath[MAX_PATH];
                sprintf(fullPath, "%s\\%s", downloadPath, filename);
                
                // Envia comando DOWNLOAD
                sprintf(command, "DOWNLOAD %s", filename);
                send(s, command, strlen(command), 0);
                
                printf("\nBaixando %s para %s\n", filename, downloadPath);
                
                // Abre arquivo para escrita binária
                FILE *file = fopen(fullPath, "wb");
                if (file == NULL) {
                    printf("Erro ao criar arquivo.\n");
                    break;
                }
                
                long total_received = 0;
                int file_size = 0;
                
                // Recebe dados do servidor
                memset(buffer, 0, BUFFER_SIZE);
                bytes_received = recv(s, buffer, BUFFER_SIZE, 0);
                if (bytes_received > 0) {
                    // Verifica se é mensagem de erro
                    if (strstr(buffer, "não encontrado") != NULL) {
                        printf("%s", buffer);
                        fclose(file);
                        remove(fullPath);
                        break;
                    }
                    
                    // Escreve dados no arquivo
                    fwrite(buffer, 1, bytes_received, file);
                    total_received += bytes_received;
                    file_size = total_received * 3; // Estimativa inicial
                    
                    // Continua recebendo dados
                    while ((bytes_received = recv(s, buffer, BUFFER_SIZE, 0)) > 0) {
                        fwrite(buffer, 1, bytes_received, file);
                        total_received += bytes_received;
                        int progress = (int)((total_received * 100) / (total_received + 1024));
                        show_progress(progress > 100 ? 100 : progress);
                    }
                    
                    fclose(file);
                    show_complete_message("Download de", filename);
                    printf("Total recebido: %ld bytes\n", total_received);
                }
                break;
            }
                
            case 4: { // DELETE - Excluir arquivo no servidor
                strcpy(command, "LIST");
                send(s, command, strlen(command), 0);
                
                // Recebe lista de arquivos
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_received = recv(s, buffer, BUFFER_SIZE - 1, 0);
                if (bytes_received <= 0) {
                    printf("Erro ao receber lista de arquivos\n");
                    break;
                }
                buffer[bytes_received] = '\0';
                printf("\nArquivos no servidor:\n%s\n", buffer);
                
                // Obtém nome do arquivo para exclusão
                printf("Digite o nome do arquivo para excluir: ");
                fgets(filename, MAX_PATH, stdin);
                filename[strcspn(filename, "\n")] = '\0';
                
                if (strlen(filename) == 0) {
                    printf("Nome de arquivo inválido.\n");
                    break;
                }
                
                // Envia comando DELETE
                sprintf(command, "DELETE %s", filename);
                printf("\nExcluindo %s...\n", filename);
                
                // Barra de progresso simulada
                for (int i = 0; i <= 100; i += 20) {
                    show_progress(i);
                    Sleep(100);
                }
                
                send(s, command, strlen(command), 0);
                
                // Recebe confirmação
                memset(buffer, 0, BUFFER_SIZE);
                bytes_received = recv(s, buffer, BUFFER_SIZE - 1, 0);
                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';
                    show_complete_message("Delete de", filename);
                    printf("Resposta do servidor: %s\n", buffer);
                }
                break;
            }
                
            case 5: // LOCAL - Listar arquivos no diretório local
                printf("\nDiretório atual: %s\n", currentDir);
                list_local_files(currentDir);
                break;
                
            case 6: // EXIT - Desconectar do servidor
                strcpy(command, "EXIT");
                send(s, command, strlen(command), 0);
                closesocket(s);
                WSACleanup();
                printf("Desconectado.\n");
                return 0;
                
            default:
                printf("Comando inválido.\n");
        }
    }
    
    // Limpeza final (não deve ser alcançado normalmente)
    closesocket(s);
    WSACleanup();
    return 0;
}