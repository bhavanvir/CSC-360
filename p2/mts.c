#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

struct timespec start, stop;
double accum;

pthread_mutex_t station_mutex, track_mutex;
pthread_cond_t load_cond, cross_cond;

typedef struct train_t
{
    int train_number;
    int priority;
    char direction;
    int loading_time;
    int crossing_time;
    bool ready;
} train_t;

typedef struct node_t
{
    int priority;
    train_t *train_data;
    struct node_t *next;
} node_t;

node_t *station_west;
node_t *station_east;

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
    return (*head == NULL ? true : false);
}

void print_output(train_t *curr_train, int print_flag)
{
    if (clock_gettime(CLOCK_REALTIME, &stop) == -1)
        perror("Error at clock_gettime with stop");

    accum = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1000000000.0;
    int minutes = (int)accum / 60;
    int hours = (int)accum / 3600;

    char *direction = curr_train->direction == 'E' || curr_train->direction == 'e' ? "East" : "West";

    if (print_flag == 1)
        printf("%02d:%02d:%04.1f Train %2d is ready to go %4s\n", hours, minutes, accum, curr_train->train_number, direction);
    else if (print_flag == 2)
        printf("%02d:%02d:%04.1f Train %2d is ON on the main track going %4s\n", hours, minutes, accum, curr_train->train_number, direction);
    else if (print_flag == 3)
        printf("%02d:%02d:%04.1f Train %2d is OFF the main track going %4s\n", hours, minutes, accum, curr_train->train_number, direction);
}

void *load_train(void *arg)
{
    train_t *curr_train = arg;

    usleep(curr_train->loading_time * 100000);

    char *direction = curr_train->direction == 'E' || 'e' ? "East" : "West";

    print_output(curr_train, 1);

    pthread_mutex_lock(&station_mutex);
    curr_train->direction == 'E' || curr_train->direction == 'e' ? push(&station_east, curr_train) : push(&station_west, curr_train);
    pthread_mutex_unlock(&station_mutex);

    pthread_cond_signal(&load_cond);
    while (curr_train->ready != true)
    {
    };

    print_output(curr_train, 2);
    usleep(curr_train->crossing_time * 100000);
    print_output(curr_train, 3);

    pthread_cond_signal(&cross_cond);
}

int main(int argc, char *argv[])
{
    /* INPUT INTEGRITY CHECK */
    if (argv[1] == NULL)
    {
        printf("Expected input file of the form: *.txt\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "r");

    if (fp == NULL)
        perror("Error at fp");

    /* TIMER START */
    if (clock_gettime(CLOCK_REALTIME, &start) == -1)
        perror("Error at clock_gettime with start");

    /* TRAIN ALLOCATION */
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
        train[i].ready = false;
        i++;
    }
    fclose(fp);

    /* PTHREAD HANDLING */
    pthread_mutex_init(&station_mutex, NULL);
    pthread_mutex_init(&track_mutex, NULL);

    pthread_cond_init(&load_cond, NULL);
    pthread_cond_init(&cross_cond, NULL);

    pthread_t train_thread[total_trains];

    for (int i = 0; i < total_trains; i++)
    {
        if (pthread_create(&train_thread[i], NULL, &load_train, (void *)&train[i]) != 0)
        {
            fprintf(stderr, "Error at train_thread[%d]", i);
            perror("");
        }
    }

    for (int i = 0; i < total_trains; i++)
        pthread_join(train_thread[i], NULL);

    pthread_mutex_destroy(&station_mutex);
    pthread_mutex_destroy(&track_mutex);

    pthread_cond_destroy(&load_cond);
    pthread_cond_destroy(&cross_cond);

    return 0;
}