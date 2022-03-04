#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct train_t
{
    int train_number;
    int priority;
    char direction;
    int loading_time;
    int crossing_time;
} train_t;

void *load_train(void *arg)
{
}

int main(int argc, char *argv[])
{
    if (argv[1] == NULL)
    {
        printf("Expected input file of the form: *.txt\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "r");

    if (fp == NULL)
        perror("Error at fp");

    int total_trains = 0;
    for (int c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n')
            total_trains++;
    rewind(fp);

    train_t *train = malloc(total_trains * sizeof(*train));

    char col1[1], col2[3], col3[3];
    int i = 0;
    while (fscanf(fp, "%s %s %s", col1, col2, col3) != EOF)
    {
        train[i].train_number = i;
        train[i].priority = col1[0] == 'E' || col1[0] == 'W' ? 1 : 0;
        train[i].direction = col1[0];
        train[i].loading_time = atoi(col2);
        train[i].crossing_time = atoi(col3);
        i++;
    }
    fclose(fp);

    pthread_t train_thread[total_trains];

    for (int i = 0; i < total_trains; i++)
    {
        if (pthread_create(&train_thread[i], NULL, &load_train, (void *)train) != 0)
        {
            fprintf(stderr, "Error at train_thread[%d]", i);
            perror("");
        }
    }

    for (int i = 0; i < total_trains; i++)
        pthread_join(train_thread[i], NULL);

    return 0;
}