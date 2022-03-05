#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>

typedef struct train_t
{
    int train_number;
    int priority;
    char direction;
    int loading_time;
    int crossing_time;
} train_t;

typedef struct node_t
{
    int priority;
    train_t *train_data;
    struct node_t *next;
} node_t;

void push(node_t **head, train_t *train_data)
{
    node_t *front = *head;

    node_t *tmp = (node_t *)malloc(sizeof(node_t));
    tmp->train_data = train_data;
    tmp->priority = train_data->priority;
    tmp->next = NULL;

    if (*head == NULL)
    {
        *head = tmp;
        return;
    }

    if (tmp->train_data->loading_time == front->train_data->loading_time && tmp->train_data->train_number < front->train_data->train_number)
    {
        tmp->next = front;
        front = tmp;
    }
    else
    {
        while (tmp->train_data->loading_time == front->next->train_data->loading_time && tmp->train_data->train_number < front->next->train_data->train_number)
            front = front->next;

        tmp->next = front->next;
        front->next = tmp;
    }
}

void pop(node_t **head)
{
    node_t *tmp = *head;
    *head = (*head)->next;
    free(tmp);
}

bool isEmpty(node_t **head)
{
    return *head == NULL;
}

pthread_mutex_t station_mutex, track_mutex;
pthread_cond_t load_cond, cross_cond;

void *load_train(void *arg)
{
    train_t *curr_train = arg;

    pthread_mutex_lock(&station_mutex);

    pthread_mutex_unlock(&station_mutex);
}

int main(int argc, char *argv[])
{
    pthread_mutex_init(&station_mutex, NULL);
    pthread_mutex_init(&track_mutex, NULL);

    pthread_cond_init(&load_cond, NULL);
    pthread_cond_init(&cross_cond, NULL);

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

    pthread_t train_thread[total_trains];

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