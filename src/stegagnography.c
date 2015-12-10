#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define IP "192.168.1.5"
#define PORT 3000

int k = 0, row = 0, col = 0, key_col = 0, key_row = 0, steg_bits = 0;
int width_img = 0, width_steg = 0, height_steg = 0;
int socket_desc = 0;

int* get_key(char* path, int* key_size)
{
    FILE *key_file;
    int* key;
    int size;
    char read;
    
    key_file = fopen(path, "r");
    if (key_file != NULL)
    {
        fseek(key_file, 0, SEEK_END);
        size = ftell(key_file);
        fseek(key_file, 0, SEEK_SET);

        key = malloc(sizeof(int)*size);
        while (fscanf(key_file, "%c", &read) != EOF)
        {
            key[*key_size] = read - '0';
            (*key_size)++;
        }
        fclose(key_file);
    }
    else
    {
        errx(1, "It was not possible to open the key file");
    }
    return key;
}

char* get_steg_bytes(char* path, int* key, int key_size, int steg_bytes)
{
    int i = 0, file_position = 0, steg_position = 0;
    char read;
    char* bytes;
    FILE *steg_file;

    steg_file = fopen(path, "r+b");

    if (steg_file == NULL)
    {
        errx(1, "It was not possible to open the steg file");
    }

    bytes = (char*) malloc(sizeof(char)*steg_bytes + 1);

    for (i = 0; i < steg_bytes; ++i, ++k)
    {
        steg_position = key[k%key_size];
        
        if (steg_position)
        {
            key_col = (steg_position - 1) / 3;

            key_row = (steg_position - 1) % 3;
            file_position = (row + key_row) * width_img + col + key_col;

            fseek(steg_file, file_position, SEEK_SET);
            fscanf(steg_file, "%c", &read);

            bytes[i] = read;

            if (steg_bytes == (int)ceil(128.0 / steg_bits))
            {
                unsigned char cal_real_bit = 0xFF;
                cal_real_bit <<= steg_bits;
                fseek(steg_file, file_position, SEEK_SET);
                read &= cal_real_bit;
                fwrite(&read, 1, 1, steg_file);
            }
        }
        else
        {
            i--;
        }
        
        col += 3;

        if(col + 3 > width_img)
        {
            col = 0;
            row += 3;
        }
    }
    fclose(steg_file);
    return bytes;
}

char* generate_steg(char* steg_bytes, int bytes_size)
{
    int i, j, position, bit;
    char* result_steg;
    char aux = 0;

    result_steg = (char*) malloc((sizeof(char) * bytes_size));

    for (i = 0; i < bytes_size; ++i)
    {
        position = 1;
        aux = 0;
        for (j = 0; j < steg_bits; ++j)
        {
            bit = steg_bytes[i] & position;
            position <<= 1;

            switch (j % 5)
            {
                case 0:
                    if(bit)
                        aux |= 128;
                    break;
                case 1:
                    if(bit)
                        aux |= 64;
                    break;
                case 2:
                    if(bit)
                        aux |= 32;
                    break;
                case 3:
                    if(bit)
                        aux |= 16;
                    break;
                case 4:
                    if(bit)
                        aux |= 8;
                    break;
                default:
                    errx(1, "Error when calculating the bits");
                    break;
            }
        }
        result_steg[i] = aux;
    }
    return result_steg;
}


char* generate_hash(char* hash_bytes, int bytes_size)
{
    int i, j, h = 0, position, bit;
    char* hash_steg;
    char aux = 0;

    hash_steg = (char*) malloc(sizeof(char) * 16);

    for (i = 0; i < bytes_size; ++i)
    {
        position = 1;
        for (j = 0; j < steg_bits && h < 16; ++j)
        {
            bit = hash_bytes[i] & position;
            position <<= 1;

            switch ((i*steg_bits + j) % 8)
            {
                case 0:
                    if(bit)
                        aux |= 128;
                    break;
                case 1:
                    if(bit)
                        aux |= 64;
                    break;
                case 2:
                    if(bit)
                        aux |= 32;
                    break;
                case 3:
                    if(bit)
                        aux |= 16;
                    break;
                case 4:
                    if(bit)
                        aux |= 8;
                    break;
                case 5:
                    if(bit)
                        aux |= 4;
                    break;
                case 6:
                    if(bit)
                        aux |= 2;
                    break;
                case 7:
                    if(bit)
                        aux |= 1;
                    hash_steg[h] = aux;
                    h++;
                    aux = 0;
                    break;
                default:
                    errx(1, "Error when calculating the bits");
                    break;
            }
        }
    }
    return hash_steg;
}


void save_file(char* steg, char* file, int size)
{
    FILE *steg_file;
    steg_file = fopen(file, "w");
    if (steg_file != NULL)
    {
        if(strcmp(file, "res/result.y"))
        {

        }
        fwrite(steg , 1 , size, steg_file);
    }
    else
    {
        errx(1, "It was not possible to open the result file");
    }
    fclose(steg_file);
}

char* create_hash(char* path)
{
    char *md5 = malloc(sizeof(char) * MD5_DIGEST_LENGTH);

    FILE *file = fopen (path, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

    if (file == NULL) {
        errx(1, "The file can't be opened");
    }

    MD5_Init(&mdContext);
    
    while ((bytes = fread (data, 1, 1024, file)) != 0)
    {
        MD5_Update (&mdContext, data, bytes);
    }
    
    MD5_Final ((unsigned char*)md5, &mdContext);
    save_file(md5, "res/hash_orig", MD5_DIGEST_LENGTH);
    fclose (file);

    return md5;
}

/*
*   Habilida o socket a aceitar uma nova comunicacao
*   Return: Descritor do socket
*/
int do_connect()
{
    struct sockaddr_in addr_struct;
    int socket_descriptor;

    /*Cria um novo socket*/
    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor == -1)
    {
        errx(1, "Erro ao criar o socket");
    }

    /*Define as propriedades da conexao*/
    addr_struct.sin_family = AF_INET;
    addr_struct.sin_port = htons(PORT);
    addr_struct.sin_addr.s_addr = inet_addr(IP);
    bzero(&(addr_struct.sin_zero), 8);

    /*Cria uma conexao com as propriedades passadas*/
    if (connect(socket_descriptor,(struct sockaddr *) &addr_struct, sizeof(struct sockaddr)) == -1) 
    {
        close(socket_descriptor);
        errx(1, "Erro ao conectar com o servidor");
    }

    return socket_descriptor;
}

void send_img(char* img, int size)
{
    socket_desc = do_connect(PORT, IP);

    if (send(socket_desc, &size, size, 0) == -1)
    {
        close(socket_desc);
        errx(1, "Erro ao enviar mensagem ao servidor");
    }

    int i;
    for (i = 0; i < size / 1024; i++)
    {
        if (send(socket_desc, &img + i * 1024, 1024, 0) == -1)
        {
            close(socket_desc);
            errx(1, "Erro ao enviar mensagem ao servidor");
        }
    }

    if (i % 1024)
    {
        if (send(socket_desc, &img, i % 1024, 0) == -1)
        {
            close(socket_desc);
            errx(1, "Erro ao enviar mensagem ao servidor");
        }
    }
}

int main(int argc, char* argv[])
{
    int* key;
    int key_size = 0;
    char *steg_bytes, *result_steg, *hash_bytes, *hash_steg, *hash_img;
    switch (argc)
    {
        case 7:
            key = get_key(argv[2], &key_size);
            width_img = atoi(argv[3]);
            width_steg = atoi(argv[4]);
            height_steg = atoi(argv[5]);
            steg_bits = atoi(argv[6]);
            steg_bytes = get_steg_bytes(argv[1], key, key_size, width_steg * height_steg);
            result_steg = generate_steg(steg_bytes, width_steg * height_steg);
            save_file(result_steg, "res/result.y", width_steg * height_steg);
            hash_bytes = get_steg_bytes(argv[1], key, key_size, ceil(128.0 / steg_bits));
            hash_steg = generate_hash(hash_bytes, ceil(128.0 / steg_bits));
            save_file(hash_steg, "res/hash", 16);
            hash_img = create_hash(argv[1]);

            if(strcmp(hash_steg, hash_img) == 0)
            {
                printf("Bateu\n");
                send_img(result_steg, width_steg * height_steg);
            }
            break;
        default:
            errx(1, "invalid arguments");
    }

    free(key);
    free(steg_bytes);
    free(result_steg);
    free(hash_steg);
    free(hash_img);
    return 0;
}
