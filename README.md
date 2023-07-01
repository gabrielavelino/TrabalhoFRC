# Tel Chat


## Alunos
|Matrícula | Aluno |
| -- | -- |
| 18/0100831  |  Gabriel Avelino Freire |
| 18/0102613 |  ítalo Fernandes Sales de Serra |
| 18/0130722  |  Samuel Nogueira Bacelar |

## Descrição

O objetivo desse trabalho é criar uma aplicação que gerencia chat rooms (salas de bate-papo), utilizando C. Utilizando a arquitetura TCP/IP, o servidor é capaz de controlar os comandos realizados pelos clientes. Que são acessados por meio do comando telnet.

## Como utilizar

Servidor:

- Compilar o código
```jsx
gcc main.c -o server
```
- Para rodar o programa
```jsx
./server <ip> <porta>
```

Cliente

Para utilizar o chat é preciso ter o comando telnet instalado em sua máquina.

```jsx
telnet <ip do servidor> <porta do servidor>
```

Após isso, é só seguir as instruções para ver os comandos disponíveis.
