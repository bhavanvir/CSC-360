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

// Struct containing train information
typedef struct train_t
{
    int train_number;
    int priority;
    char direction;
    int loading_time;
    int crossing_time;
    bool ready;
} train_t;

// Struct containing node information for a linked list based priority queue
typedef struct node_t
{
    int priority;
    train_t *train_data;
    struct node_t *next;
} node_t;

// Priority queues for the east and west stations
node_t *station_east;
node_t *station_west;

void push(node_t **head, train_t *train_data)
{
    node_t *front = *head;

    // Allocate memory for a new node to be pushed into the linked list
    node_t *tmp = (node_t *)malloc(sizeof(node_t));
    tmp->train_data = train_data;
    tmp->priority = train_data->priority;
    tmp->next = NULL;

    if (*head == NULL)
    {
        *head = tmp;
        return;
    }

    // If the loading times are the same for two trains, push the train that appears first in the input file to the front of the priority queue
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

// Free memory associated with a certain node in the priority queue
void pop(node_t **head)
{
    node_t *tmp = *head;
    *head = (*head)->next;
    free(tmp);
}

// If the priority queue is empty return true, if not return false
bool isEmpty(node_t **head)
{
    return (*head == NULL ? true : false);
}

// Print the state of the train depending on whether the state is READY, ON or OFF
void print_output(train_t *curr_train, char *print_flag)
{
    // Adapted from the tutorial slides
    if (clock_gettime(CLOCK_REALTIME, &stop) == -1)
        perror("Error at clock_gettime with stop");

    accum = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1000000000.0;
    int minutes = (int)accum / 60;
    int hours = (int)accum / 3600;

    // Set the direction string by checking the direction of the current train being interacted with
    char *direction = curr_train->direction == 'E' || curr_train->direction == 'e' ? "East" : "West";

    // Compare the print flag to the READY, ON and OFF states
    if (strcmp(print_flag, "READY") == 0)
        printf("%02d:%02d:%04.1f Train %2d is ready to go %4s\n", hours, minutes, accum, curr_train->train_number, direction);
    else if (strcmp(print_flag, "ON") == 0)
        printf("%02d:%02d:%04.1f Train %2d is ON on the main track going %4s\n", hours, minutes, accum, curr_train->train_number, direction);
    else if (strcmp(print_flag, "OFF") == 0)
        printf("%02d:%02d:%04.1f Train %2d is OFF the main track after going %4s\n", hours, minutes, accum, curr_train->train_number, direction);
}

// Load the train and push to the appropriate east or west stations
void *load_train(void *arg)
{
    train_t *curr_train = arg;

    char *direction = curr_train->direction == 'E' || 'e' ? "East" : "West";

    // Sleep the program for the appropriate amount of time in seconds
    usleep(curr_train->loading_time * 100000);

    print_output(curr_train, "READY");

    // Lock the station mutex and push the current train to the appropriate east or west station
    pthread_mutex_lock(&station_mutex);
    curr_train->direction == 'E' || curr_train->direction == 'e' ? push(&station_east, curr_train) : push(&station_west, curr_train);
    pthread_mutex_unlock(&station_mutex);

    pthread_cond_signal(&load_cond);
    // Wait until the current train is ready to be placed on the track
    while (curr_train->ready != true)
        ;

    print_output(curr_train, "ON");
    usleep(curr_train->crossing_time * 100000);
    print_output(curr_train, "OFF");

    pthread_cond_signal(&cross_cond);
}

// Pop the train with the highest priority off the track
void send_train(int total_trains)
{
    // Directional character used to determine which train should be popped off the train
    char direction[1];
    // Counter to track how many trains have been sent in each direction
    int sent_east, sent_west = 0;
    for (int i = total_trains - 1; i >= 0; i--)
    {
        // Lock the track mutex and determine the train with the highest priority
        pthread_mutex_lock(&track_mutex);

        // If both east and west stations are empty, then wait until it is not
        if (isEmpty(&station_east) == true && isEmpty(&station_west) == true)
            pthread_cond_wait(&load_cond, &track_mutex);

        // If both stations or one of the stations are not empty, check the starvation case and set the priority direction accordingly
        if (isEmpty(&station_east) == false && isEmpty(&station_west) == false)
        {
            sent_east == 4 ? strncpy(direction, "W", 1) : strncpy(direction, "E", 1);
            sent_west == 4 ? strncpy(direction, "E", 1) : strncpy(direction, "E", 1);
            sent_east = sent_east == 4 ? 0 : sent_east;
            sent_west = sent_west == 4 ? 0 : sent_west;
            station_east->priority > station_west->priority ? strncpy(direction, "E", 1) : strncpy(direction, "W", 1);
        }
        else if (isEmpty(&station_east) == false && isEmpty(&station_west) == true)
            strncpy(direction, "E", 1);
        else if (isEmpty(&station_east) == true && isEmpty(&station_west) == false)
            strncpy(direction, "W", 1);

        // Check the priority, increment the appropriate direction counter, set the train's ready state to true and pop it from the station queue
        if (strcmp(direction, "E") == 0)
        {
            sent_east++;
            station_east->train_data->ready = true;
            pthread_mutex_lock(&station_mutex);
            pop(&station_east);
            pthread_mutex_unlock(&station_mutex);
            sent_west = 0;
        }
        else if (strcmp(direction, "W") == 0)
        {
            sent_west++;
            station_west->train_data->ready = true;
            pthread_mutex_lock(&station_mutex);
            pop(&station_west);
            pthread_mutex_unlock(&station_mutex);
            sent_east = 0;
        }
        pthread_cond_wait(&cross_cond, &track_mutex);
        pthread_mutex_unlock(&track_mutex);
    }
}

int main(int argc, char *argv[])
{
    // If the input arguments are less than 2, print an error message and exit the program
    if (argv[1] == NULL)
    {
        printf("Expected input file of the form: *.txt\n");
        exit(1);
    }

    // Open the trains.txt file in read mode, and check whether an error is produced when it's opened
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL)
        perror("Error at fp");

    // Start the clock timer
    if (clock_gettime(CLOCK_REALTIME, &start) == -1)
        perror("Error at clock_gettime with start");

    // Count the number of lines in the input file and have that be the total number of trains
    int total_trains = 0;
    for (int c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n')
            total_trains++;
    // Rewind the file so that it can be read again
    rewind(fp);

    // Allocate memory for all trains in the input file
    train_t *train = malloc(total_trains * sizeof(*train));

    char col1[1], col2[3], col3[3];
    int i = 0;
    // Read each column of the input file and assign the appropriate attributes to each train
    while (fscanf(fp, "%s %s %s", col1, col2, col3) != EOF)
    {
        train[i].train_number = i;
        // If the train direction is 'E' or 'W' set the priority as 1 (high), else set it as 0 (low)
        train[i].priority = col1[0] == 'E' || col1[0] == 'W' ? 1 : 0;
        train[i].direction = col1[0];
        train[i].loading_time = atoi(col2);
        train[i].crossing_time = atoi(col3);
        train[i].ready = false;
        // Increment the train number index
        i++;
    }
    fclose(fp);

    // Initialize a mutex for the station and track, and a condition variable for loading and crossing
    pthread_mutex_init(&station_mutex, NULL);
    pthread_mutex_init(&track_mutex, NULL);

    pthread_cond_init(&load_cond, NULL);
    pthread_cond_init(&cross_cond, NULL);

    pthread_t train_thread[total_trains];

    // Create a thread for each train and check whether an error is produced
    for (int i = 0; i < total_trains; i++)
    {
        if (pthread_create(&train_thread[i], NULL, &load_train, (void *)&train[i]) != 0)
            perror("Error at train_thread[i]");
    }

    // Dispatch the correct sequence of trains
    send_train(total_trains);

    // Join all of the train threads
    for (int i = 0; i < total_trains; i++)
        pthread_join(train_thread[i], NULL);

    // Destroy both mutexs and condition variables
    pthread_mutex_destroy(&station_mutex);
    pthread_mutex_destroy(&track_mutex);

    pthread_cond_destroy(&load_cond);
    pthread_cond_destroy(&cross_cond);

    return 0;
}