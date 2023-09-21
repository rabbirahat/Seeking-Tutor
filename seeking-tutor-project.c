#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX 100000

int new_visited[MAX]; ///this array keeps track of if a student is visited or not at a certain time

int priority_number[MAX];
int arrival_time[MAX];

int help[MAX]; ///to keep the track of how many times a student visited
int student_ids[MAX]; ///student der id gula
int tutor_ids[MAX]; ///tutor der id gula

sem_t student;
sem_t coordinator;
sem_t tutor[MAX];
sem_t mutexLock;

int requesting_time = 0;
int students_done = 0; ///finished student

int help_number = 0;
int total_chair = 0;
int occupied_chairs=0;
int total_student = 0;
int total_tutor = 0;


void *student_thread(void *student_id)
{
    int stu_id=*(int*)student_id; //converting void to int

    while(1)
    {
        if(help[stu_id-1] == help_number) //max kotobar student ashte parbe
        {

            sem_wait(&mutexLock);
            students_done++; ///how many students have tutored fully
            sem_post(&mutexLock);

            printf("\n\n\t\t\t student %d terminates\n\n",stu_id);
            if(students_done == total_student)
            {
                printf("\n\t\t  All Students Have Recieved Help\n");
            }
            //notify coordinate to terminate
            sem_post(&student);
            pthread_exit(NULL); //thread ends
        }


        //seat e aise
        sem_wait(&mutexLock); //mutex was initially 1, when we call sem_wait(), it turns 0, it is locked right now
        if(occupied_chairs == total_chair)
        {
            //seat pay nai
            sem_post(&mutexLock); ///mutex unlockes it, by turning the value 1
            continue;
        }

        occupied_chairs++; ///seat paise
        requesting_time++; //request number of student, it keeps the track of when the student came
        new_visited[stu_id-1]=requesting_time;

        printf("\nStudent Thread: Student %d takes a Seat.\nStudent Thread: Empty Chairs = %d\n",stu_id,total_chair-occupied_chairs);

        sem_post(&mutexLock); //unlocked for other students

        //notify coordinator that student seated.
        sem_post(&student);

        //wait to be tutored.
        sem_wait(&tutor[stu_id-1]); //semaphore array is used to signal only the specific student which finished studies

        printf("\nStudent Thread: Student %d Received Help.\n",stu_id);

        //tutored times++ after tutoring.
        sem_wait(&mutexLock);
        help[stu_id-1]++;
        printf("\nStudent Thread: Student %d's Priority now is %d\n",stu_id, help[stu_id-1]);
        sem_post(&mutexLock);
    }
}

void *coordinator_thread()
{
    while(1)
    {
        //if all students finish, tutors threads and coordinate thread terminate.
        if(students_done==total_student)
        {
            //terminate tutors first
            for(int i=0; i<total_tutor; i++)
            {
                //notify tutors to terminate
                sem_post(&coordinator);
            }

            //terminate coordinate itself
            printf("\n\n\t\t\tcoordinator terminates\n\n");
            pthread_exit(NULL); //thread ends

        }

        //wait student's notification
        sem_wait(&student); // waiting for students signal

        sem_wait(&mutexLock);
        //find the students who just seated and push them into the priority queue
        for(int i=0; i<total_student; i++)
        {
            if(new_visited[i]>-1)
            {
                priority_number[i] = help[i];
                arrival_time[i] = new_visited[i];//to save the time when the student came

                printf("\nCoordinator Thread: Student %d with Priority %d in the queue.\n",student_ids[i],help[i]);
                new_visited[i]=-1;

                //notify tutor that student is in the queue.
                sem_post(&coordinator);
            }
        }
        sem_post(&mutexLock);
    }
}

void *tutor_thread(void *tutor_id)
{
    int t_id=*(int*)tutor_id;

    while(1)
    {
        //if all students finish, tutors threads terminate.
        if(students_done==total_student)
        {
            sem_wait(&mutexLock);
            printf("\n\n\t\t\t tutor %d terminates\n\n",t_id);

            sem_post(&mutexLock);

            pthread_exit(NULL); //thread ends
        }

        int max_request=total_student*help_number+1;	//this is the maximum serial a student can get
        int max_priority = help_number-1;
        int s_id=-1; //it is something like a flag, if tutor finds any student in priority queue, it will contain that students id, otherwise it will be -1.
        //wait coordinator's notification
        sem_wait(&coordinator);

        sem_wait(&mutexLock);
        //find student with highest priority from priority queue;
        //if students with same priority, who come first has higher priority
        for(int i=0; i<total_student; i++)
        {
            if(priority_number[i]>-1 && priority_number[i]<= max_priority)
            {
                if(arrival_time[i]<max_request)
                {
                    max_priority=priority_number[i]; //
                    max_request=arrival_time[i]; //who comes first , here we are trying to find the minimum arrival time if the priotiy is same
                    s_id=student_ids[i];
                }
            }


        }

        //in case no student in the queue.
        if(s_id==-1)
        {
            sem_post(&mutexLock);
            continue;
        }

        //(reset the priority queue)
        priority_number[s_id-1] = -1;
        arrival_time[s_id-1] = -1;

        //occupied chair--
        occupied_chairs--;
        sem_post(&mutexLock);

        //after tutoring
        sem_wait(&mutexLock);
        printf("\nTutor Thread: Student %d is tutored by Tutor %d\n",s_id,t_id);
        sem_post(&mutexLock);

        //update shared data so student can know who tutored him.
        sem_post(&tutor[s_id-1]);

    }
}






int main()
{

    printf("Seeking Tutor Problem..........\n\n");

    printf("Total number of students: ");
    scanf("%d", &total_student);

    printf("Total number of Tutors: ");
    scanf("%d", &total_tutor);

    printf("Total number of Chairs: ");
    scanf("%d", &total_chair);

    printf("Maximum number of help a student can get: ");
    scanf("%d", &help_number);



    //init arrays
    for(int i=0; i<total_student; i++)
    {
        new_visited[i]=-1;  ///-1 dea karon it acts like a flag, used in a condition
        priority_number[i] = -1;
        arrival_time[i] = -1;
        help[i]=0; ///this is not flag, this actually increases
    }


    //init mutex and semaphore
    sem_init(&student,0,0);
    sem_init(&coordinator,0,0);
    sem_init(&mutexLock,0,1);
    for(int i=0; i<total_student; i++)
    {
        sem_init(&tutor[i],0,0);
    }

    //allocate threads
    pthread_t students[total_student];
    pthread_t tutors[total_tutor];
    pthread_t coordinator;

    //create threads

    ///for student
    for(int i = 0; i < total_student; i++)
    {
        student_ids[i] = i + 1; //saved in array
        pthread_create(&students[i], NULL, student_thread, (void*) &student_ids[i]);

    }


    ///create threads for tutors
    for(int i = 0; i < total_tutor; i++)
    {
        tutor_ids[i] = i + 1;
        pthread_create(&tutors[i], NULL, tutor_thread, (void*) &tutor_ids[i]);

    }



    ///create thread for coordinator
    pthread_create(&coordinator,NULL,coordinator_thread,NULL);




    //join threads, to connect the threads to main program

    for(int i =0; i < total_student; i++)
    {
        pthread_join(students[i],NULL);
    }

    for(int i =0; i < total_tutor; i++)
    {
        pthread_join(tutors[i],NULL);
    }

    pthread_join(coordinator, NULL);

}
