/* *******************************/
/* FGA / Eng. Software / FRC     */
/* Prof. Fernando W. Cruz        */
/* Codigo: System Call Select()  */
/* *******************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#define STDIN 0

fd_set master, read_fds, write_fds;
struct sockaddr_in myaddr, remoteaddr;
int max_file_descriptors, socket_file_descriptor, new_file_descriptor, file_descriptor, j, nbytes, yes = 1;
socklen_t addrlen;
char buf[256];

typedef struct person {
  char *nome;
  int socket;
  int room;
} person;

void broadcast_message()
{
    // Função responsável por enviar a mensagem recebida para todos os clientes conectados, exceto o próprio remetente.
    for (j = 0; j <= max_file_descriptors; j++)
    {
        if (FD_ISSET(j, &master))
        {
            if ((j != file_descriptor) && (j != socket_file_descriptor))
            {
                send(j, buf, nbytes, 0);
            }
        }
    } /* fim for */
} /* fim envia_msg */

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        printf("Digite IP e Porta para este servidor\n");
        exit(1);
    }

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = inet_addr(argv[1]);
    myaddr.sin_port = htons(atoi(argv[2]));

    memset(&(myaddr.sin_zero), '\0', 8);
    addrlen = sizeof(remoteaddr);

    // Associa o socket a um endereço IP e porta local
    bind(socket_file_descriptor, (struct sockaddr *)&myaddr, sizeof(myaddr));

    // Coloca o socket em um estado de escuta, aguardando conexões entrantes
    listen(socket_file_descriptor, 10);

    // Adiciona o socket do servidor ao conjunto de descritores de arquivo "master"
    FD_SET(socket_file_descriptor, &master);

    // Adiciona o descritor de arquivo padrão de entrada (stdin) ao conjunto "master"
    FD_SET(STDIN, &master);

    // Inicializa o valor máximo de descritor de arquivo como sendo o do socket do servidor
    max_file_descriptors = socket_file_descriptor;

    for (;;)
    {
        // Copia o conjunto de descritores de arquivo "master" para o conjunto "read_fds" antes de chamar a função select()
        read_fds = master;

        // Realiza a chamada do sistema "select" para monitorar os descritores de arquivo em busca de atividade
        select(max_file_descriptors + 1, &read_fds, NULL, NULL, NULL);

        for (file_descriptor = 0; file_descriptor <= max_file_descriptors; file_descriptor++)
        {
            if (FD_ISSET(file_descriptor, &read_fds))
            {
                if (file_descriptor == socket_file_descriptor)
                {
                    // Se o socket de leitura for o socket principal (sd), indica que há uma nova conexão entrante.

                    // Aceita a nova conexão e cria um novo socket para comunicação com o cliente
                    new_file_descriptor = accept(socket_file_descriptor, (struct sockaddr *)&remoteaddr, &addrlen);

                    // Adiciona o novo socket ao conjunto de descritores de arquivo "master"
                    FD_SET(new_file_descriptor, &master);

                    // Atualiza o valor máximo de descritor de arquivo, se necessário
                    if (new_file_descriptor > max_file_descriptors)
                        max_file_descriptors = new_file_descriptor;
                }
                else
                {
                    // Se não for o socket principal, trata-se de uma mensagem recebida de um cliente.

                    // Limpa o buffer antes de receber a mensagem
                    memset(&buf, 0, sizeof(buf));

                    // Recebe a mensagem do cliente
                    nbytes = recv(file_descriptor, buf, sizeof(buf), 0);

                    // Chama a função broadcast_message() para enviar a mensagem recebida para os outros clientes
                    broadcast_message();
                } /* fim-do else*/
            }     /* fim do if */
        }         /* fim for interno*/
    }             /* fim-for ;; */
    return (0);
} /* fim main*/

