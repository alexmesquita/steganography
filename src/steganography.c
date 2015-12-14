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
#include <sys/types.h>
#include <stdbool.h>

#define IP "192.168.133.26"
#define PORT 3001
#define SIZE_MSG 4096
#define HASH_SIZE 128.0

int k = 0, row = 0, col = 0, key_col = 0, key_row = 0, steg_bits = 0, bytes_read = 0;
int width_img = 0, width_steg = 0, height_steg = 0;
int descriptor_client = 0;
int* key;
char *steg_bytes, *hash_bytes, *hash_steg, *hash_img;
FILE *result_file;
bool truncate = false;

void print_percentage();
void sigint();
int* get_key(char* path, int* key_size);
char* get_steg_bytes(char* path, int* key, int key_size, int steg_bytes);
void generate_steg(char* steg_bytes, int bytes_size);
char* generate_hash(char* hash_bytes, int bytes_size);
char* create_hash(char* path);
void send_img();
void down_client();
void save_file(char* steg, char* file, int size);

int main(int argc, char* argv[])
{
    signal(SIGINT, sigint);
    int key_size = 0;

    printf("Process ID: %d\n", getpid());
    if(argc != 7)
    {
        errx(-1, "Usage: <video> <key> <width_video> <width_result> <height_result> <bits_depth>");
    }
    else
    {
        key = get_key(argv[2], &key_size);
        width_img = atoi(argv[3]);
        width_steg = atoi(argv[4]);
        height_steg = atoi(argv[5]);
        steg_bits = atoi(argv[6]);
        steg_bytes = get_steg_bytes(argv[1], key, key_size, width_steg * height_steg);
        generate_steg(steg_bytes, width_steg * height_steg);
        hash_bytes = get_steg_bytes(argv[1], key, key_size, ceil(HASH_SIZE / steg_bits));
        hash_steg = generate_hash(hash_bytes, ceil(HASH_SIZE / steg_bits));
        save_file(hash_steg, "res/hash", 16);
        hash_img = create_hash(argv[1]);

        if(strcmp(hash_steg, hash_img) == 0 || truncate == true)
        {
            send_img();
        }
    }

    down_client();
    return 0;
}


void print_percentage()
{
    printf("\nGet percentage!\n");
    if(bytes_read != width_steg * height_steg)
    {
        printf("Bytes read: %d\n", bytes_read);
        printf("Progress: %.2f%%\n", (100.0 * bytes_read) / (width_steg * height_steg));
    }
    else
    {
        printf("The extraction was completed\n");
    }
}

void sigint()
{
    print_percentage();
    truncate = true;
}

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
        errx(-1, "It was not possible to open the key file");
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
        errx(-1, "It was not possible to open the steg file");
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

            if (steg_bytes == (int)ceil(HASH_SIZE / steg_bits))
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

void generate_steg(char* steg_bytes, int bytes_size)
{
    int j, position, bit;
    char aux = 0;

    result_file = fopen("res/result.y", "w");
    
    if (result_file == NULL)
    {
        errx(-1, "It was not possible to open the result file");
    }

    fprintf(result_file, "P5 %d %d 255 ", width_steg, height_steg);

    for (bytes_read = 0; bytes_read < bytes_size; ++bytes_read)
    {
        position = 1;
        aux = 0;
        if(truncate == false)
        {
            for (j = 0; j < steg_bits; ++j)
            {
                bit = steg_bytes[bytes_read] & position;
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
                        errx(-1, "Error when calculating the bits");
                        break;
                }
            }
        }
        fwrite(&aux, 1, 1, result_file);
    }
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
                    errx(-1, "Error when calculating the bits");
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
        fwrite(steg , 1 , size, steg_file);
    }
    else
    {
        errx(-1, "It was not possible to open the result file");
    }
    fclose(steg_file);
}

char* create_hash(char* path)
{
    char *md5 = malloc(sizeof(char) * MD5_DIGEST_LENGTH);

    FILE *file = fopen (path, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[SIZE_MSG];

    if (file == NULL) {
        errx(-1, "The file can't be opened");
    }

    MD5_Init(&mdContext);
    
    while ((bytes = fread (data, 1, SIZE_MSG, file)) != 0)
    {
        MD5_Update (&mdContext, data, bytes);
    }
    
    MD5_Final ((unsigned char*)md5, &mdContext);
    save_file(md5, "res/hash_orig", MD5_DIGEST_LENGTH);
    fclose (file);

    return md5;
}

/*
*   Habilita o socket a aceitar uma nova comunicacao
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
        errx(-1, "Error creating socket");
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
        errx(-1, "Error connecting to the server");
    }

    return socket_descriptor;
}

void send_img()
{
    printf("Sending image...\n");
    FILE *file = fopen("res/result.y", "r");
    if(!file)
    {
        errx(-1, "Error in open file");
    }

    char buffer[4096];
    int bytes = 0;
    while((bytes = fread(buffer, sizeof(char), 4096, file)))
    {
        descriptor_client = do_connect(PORT, IP);
        
        if (send(descriptor_client, &bytes, sizeof(bytes), 0) == -1)
        {
            errx(-1, "Error sending message to the server");
        }

        if (send(descriptor_client, buffer, bytes, 0) == -1)
        {
            errx(-1, "Error sending message to the server");
        }

        char confirm;
        if (recv(descriptor_client, &confirm, sizeof(confirm), 0) == -1) 
        {
            errx(-1, "Error when receiving confirmation");
        }
        close(descriptor_client);
    }

    if(descriptor_client)
    {
        close(descriptor_client);
    }
}

void down_client()
{
    if(key)
        free(key);
    if(steg_bytes)
        free(steg_bytes);
    if(hash_steg)
        free(hash_steg);
    if(hash_img)
        free(hash_img);
    if(result_file)
        fclose(result_file);
    printf("Down Client\n");
}
