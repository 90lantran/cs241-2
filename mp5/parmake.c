/** @file parmake.c */
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "parser.h"
#include "rule.h"

//initialize queue to hold rules
queue_t* rule_list;
int rule_ct;
queue_t satisfied;
queue_t working;
rule_t* target_rule = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sat_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t work_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

int is_satisfied(rule_t* rule)
{
    int i;
    int count = 0;
    char* target = rule->target;
    char* curr_rule;
    pthread_mutex_lock(&sat_lock);
    for(i=0; i<queue_size(&satisfied); i++)
    {
        curr_rule = (char*)queue_at(&satisfied, i);
        pthread_mutex_unlock(&sat_lock);
        if(strcmp(curr_rule, target) == 0)
        {
            count++;
        }
        pthread_mutex_lock(&sat_lock);
    }
    pthread_mutex_unlock(&sat_lock);
    return count;
}

int add_satisfied(rule_t* rule)
{
    int i;
    char* target = rule->target;
    char* curr_rule;
    for(i=0; i<queue_size(&satisfied); i++)
    {
        curr_rule = (char*)queue_at(&satisfied, i);
        if(strcmp(curr_rule, target) == 0)
        {
            printf("already satisfied\n");
            return 1;
        }
    }
    queue_enqueue(&satisfied, (void*)target);
    return 0;
}

void disp_sat()
{
    //pthread_mutex_lock(&sat_lock);
    int i;
    char* curr_rule;
    pthread_mutex_lock(&sat_lock);
    for(i=0; i<queue_size(&satisfied); i++)
    {
        curr_rule = (char*)queue_at(&satisfied, i);
        pthread_mutex_unlock(&sat_lock);
        printf("%s\n", curr_rule);
        pthread_mutex_lock(&sat_lock);
    }
    pthread_mutex_unlock(&sat_lock);
    return;
    //pthread_mutex_unlock(&sat_lock);
}

int now_worked_on(rule_t* rule)
{
    int i;
    char* target = rule->target;
    char* curr_rule;
    for(i=0; i<queue_size(&working); i++)
    {
        curr_rule = (char*)queue_at(&working, i);
        if(strcmp(curr_rule, target) == 0)
        {
            //printf("already being worked on\n");
            return 1;
        }
    }
    queue_enqueue(&working, (void*)target);
    return 0;
    //printf("rule %s added to queue\n", rule->target);
}

int worked_on(rule_t* rule)
{
    int i;
    int count = 0;
    char* target = rule->target;
    char* curr_rule;
    pthread_mutex_lock(&work_lock);
    for(i=0; i<queue_size(&working); i++)
    {
        curr_rule = (char*)queue_at(&working, i);
        pthread_mutex_unlock(&work_lock);
        if(strcmp(curr_rule, target) == 0)
        {
            count++;
        }
        pthread_mutex_lock(&work_lock);
    }
    pthread_mutex_unlock(&work_lock);
    return count;
}

void not_worked_on(rule_t* rule)
{
    int i;
    char* target = rule->target;
    char* curr_rule;
    int count = 0;
    for(i=0; i<queue_size(&working); i++)
    {
        curr_rule = (char*)queue_at(&working, i);
        if(strcmp(curr_rule, target) == 0)
        {
            count++;
            queue_remove_at(&working, i);
        }
    }
    if(count == 0)
        printf("rule %s not found in working queue\n", rule->target);
    return;
}

rule_t* findRule(char* target)
{
    int pos = queue_size(rule_list)-1;
    while(pos>=0)
    {
        rule_t* rule = (rule_t*)queue_at(rule_list, pos);
        if(strcmp((rule)->target, target) == 0)
            return rule;
        pos--;
    }
    return NULL;
}

int can_run(rule_t* rule)
{
    
    int i;
    for(i=0; i<queue_size(rule->deps); i++)
    {   
        char* curr_dep = queue_at(rule->deps, i);
        rule_t* curr_rule = findRule(curr_dep);
        if(curr_rule && !is_satisfied(curr_rule))
        {
            return 0;
        }
    }  
    
    return 1;
}

void remove_dep(rule_t* rule, char* dep)
{
    int dep_ct = queue_size(rule->deps);
    char* curr_dep;
    int i;
    for(i=0; i<dep_ct; i++)
    {
        curr_dep = queue_at(rule->deps, i);
        if(strcmp(curr_dep, dep) == 0)
        {
            queue_remove_at(rule->deps, i);
            return;
        }
    }
}

/* commands are removed from the commands queue as they are run so queue_size(rule->commands)
    can be used to check if a rule has been satisfied (and the dependency has been satisfied) */
/* To run a rule: */
void run_commands(rule_t* rule)
{
    pthread_mutex_lock(&queue_lock);
    int rule_ct = queue_size(rule->commands);
    int i;
    while(rule_ct > 0)
    {
        char* command = queue_dequeue(rule->commands);
        pthread_mutex_unlock(&queue_lock);
        if(system(command) != 0)   //Run each command in the order they were listed in the makefile.
        {
            exit(1);
        }
        free(command);
        pthread_mutex_lock(&queue_lock);
        //queue_remove_at(rule->commands, 0); //Once all commands have ran for a rule, this rule is completed.
        rule_ct = queue_size(rule->commands);
    }
    pthread_mutex_unlock(&queue_lock);
}


void run_rule(rule_t* rule)
{
    int i = 0;
    /*
    while(satisfied[i])
    {
        printf("%s", satisfied[i]);
        i++;
    }
    printf("\n");
    */
    //printf("1%s\n", rule->target);
    pthread_mutex_lock(&lock);
    //pthread_mutex_lock(&sat_lock);
    //pthread_mutex_lock(&work_lock);
    if(worked_on(rule) || is_satisfied(rule))
    {
        //pthread_mutex_unlock(&sat_lock);
        pthread_mutex_unlock(&lock);
        return;
    }
    //pthread_mutex_unlock(&sat_lock);
    pthread_mutex_lock(&work_lock);
    int there = now_worked_on(rule);
    if(there)
        return;
    pthread_mutex_unlock(&work_lock);

    pthread_mutex_unlock(&lock);

    rule_t* curr_rule;
    char* curr_dep;
    int dep_ct = queue_size(rule->deps);
    int run_deps = 0;   // counts how many dependencies are rules
    //printf("2%s\n", rule->target);
    i = 0;
    //pthread_mutex_lock(&work_lock);
    //pthread_mutex_lock(&sat_lock);
    //pthread_mutex_lock(&queue_lock);

    while(!can_run(rule))               //deal with each dependency that is a rule
    {

        dep_ct = queue_size(rule->deps);
        //pthread_mutex_unlock(&work_lock);
        //pthread_mutex_unlock(&sat_lock);
        curr_dep = (char*)queue_at(rule->deps, i%dep_ct);
        curr_rule = findRule(curr_dep);
        //thread_mutex_unlock(&queue_lock);
        if(curr_rule)                       //if the dependency is a rule
        {
            run_deps++;
            //pthread_mutex_lock(&work_lock);
            if(!worked_on(curr_rule))
            {
                //pthread_mutex_unlock(&work_lock);

                //pthread_mutex_lock(&queue_lock);
                //int dep_ct = queue_size(curr_rule->deps);   //find the number of dependencies to check
                //pthread_mutex_unlock(&queue_lock);

                //pthread_mutex_lock(&sat_lock);
                if(!is_satisfied(curr_rule))
                {
                    //pthread_mutex_unlock(&sat_lock);

                    pthread_mutex_lock(&work_lock);
                    not_worked_on(rule);
                    pthread_mutex_unlock(&work_lock);

                    //pthread_mutex_unlock(&sat_lock);
                    //pthread_mutex_unlock(&queue_lock); 
                    run_rule(curr_rule);       //recursively meet its dependencies

                    return;
                }
                //pthread_mutex_unlock(&sat_lock);
            }
            pthread_mutex_unlock(&work_lock);
        }
        i++;
    }
    //printf("%s %d\n", rule->target, can_run(rule));
    pthread_mutex_unlock(&work_lock);
    //pthread_mutex_unlock(&sat_lock);
    //pthread_mutex_unlock(&queue_lock); 
    if(worked_on(rule) > 1)
    {
        printf("hmm\n");
    }
    if(run_deps == 0 && dep_ct > 0)                //case where all dependencies are not rules
    {
        for(i = 0; i<dep_ct; i++)           
        {
            if(access(rule->target, F_OK) != 0) //if the target is not a file, run its commands
            {
                //pthread_mutex_lock(&queue_lock);
                run_commands(rule);
                //pthread_mutex_unlock(&queue_lock);
            }
            else                                //otherwise:
            {
                int run = 1;
                struct stat buf;
                if (stat(rule->target, &buf) == -1)     //get mod time of rule
                {
                    perror("stat");
                    exit(1);
                }
                //printf("%s", ctime(&buf.st_mtime));
                int j;
                for(j = 0; j<dep_ct; j++)
                {
                    struct stat curr_buf;
                    curr_dep = (char*)queue_at(rule->deps, i);
                    stat(curr_dep, &curr_buf); //Check the modification times of all the dependency files.
                    //printf("%s", ctime(&curr_buf.st_mtime));

//If the modification time of the rule file is NEWER (more recent) than the modification time of ALL the dependencies,
                    //printf("%f\n", difftime(curr_buf.st_mtime, buf.st_mtime));
                    if(difftime(curr_buf.st_mtime, buf.st_mtime) < 0)
                    {
                        run = 0;    //dont run the rule
                    }
                }
                if(run)             //otherwise run the rule
                {
                    //pthread_mutex_lock(&queue_lock);
                    run_commands(rule);
                    //pthread_mutex_unlock(&queue_lock);
                }
            }
        }
        //pthread_mutex_lock(&lock);
        pthread_mutex_lock(&sat_lock);
        add_satisfied(rule);
        pthread_mutex_unlock(&sat_lock);

        pthread_mutex_lock(&work_lock);
        not_worked_on(rule);
        pthread_mutex_unlock(&work_lock);

        //pthread_mutex_unlock(&lock);
        return;
    }
    
    run_commands(rule);
    //pthread_mutex_lock(&lock);
    pthread_mutex_lock(&sat_lock);
    add_satisfied(rule);
    pthread_mutex_unlock(&sat_lock);

    pthread_mutex_lock(&work_lock);
    not_worked_on(rule);
    pthread_mutex_unlock(&work_lock);

    //pthread_mutex_unlock(&lock);
    return;
}

void* thread_dispatch(void* target_queue)
{
    int i;
    for(i=0; i<queue_size((queue_t*)target_queue); i++)
    {
        char* curr_target = queue_at((queue_t*)target_queue, i);
        if(!(target_rule = findRule(curr_target)))              //find the rule with that target name
        {
            printf("Rule \'%s\' not found\n", curr_target);
            exit(1);
        }

        //pthread_mutex_lock(&sat_lock);
        while(!is_satisfied(target_rule))
        {
            //pthread_mutex_unlock(&sat_lock);
            run_rule(target_rule);
        }
        //pthread_mutex_unlock(&sat_lock);
    }
    return NULL;
}

/* Create rule, set target, and add to queue */
void parsed_new_target(char* target)
{
    rule_t* rule = malloc(sizeof(rule_t));
    rule_init(rule);
    rule->target = malloc(50*sizeof(char));
    strcpy(rule->target, target);
    //queue_init(rule->deps);
    //queue_init(rule->commands);
    queue_enqueue(rule_list, (void*)rule);
    //printf("Rule \'%s\' added\n", target);
}
/*  Add parsed_new_dependency */
void parsed_new_dependency(char* target, char* dependency)
{
	if(strlen(dependency)>0)
	{
    	rule_t* rule = findRule(target);
        char* dep = malloc(50*sizeof(char));
        strcpy(dep, dependency);
    	if(rule)
    	{
        	queue_enqueue(rule->deps, (void*)dep);
    	}
    	//printf("Dependency \'%s\' added to rule \'%s\'\n", dependency, target);
    }
}
/* Add commands */
void parsed_new_command(char *target, char *command)
{
	if(strlen(command)>0)
	{
    	rule_t* rule = findRule(target);
        char* comm = malloc(200*sizeof(char));
        strcpy(comm, command);
    	if(rule)
    	{
        	queue_enqueue(rule->commands, comm);
    	}
    	//printf("Command \'%s\' added to rule \'%s\'\n", command, target);
    }
}
/**
 * Entry point to parmake.
 */
int main(int argc, char **argv)
{
	/*	PART 1	*/
	int opt;				//used by getopt and switch
	char* mfpath = NULL;	//makefile path
	int maxthreads = 1;		//max thread count
    int threadct = 0;       //current thread ct
	char** targets;	//array of targets
	targets = malloc(50*sizeof(char*));
    rule_list = malloc(sizeof(queue_t));
    queue_init(rule_list);
    queue_init(&working);
    queue_init(&satisfied);
    int i, j;
    for(i=0; i<50; i++)                             //make space for targets
    { 
        targets[i] = malloc(50*sizeof(char));
    }
	opterr = 0;
	//call getopt until all options are read
	while ((opt = getopt(argc, argv, "f:j:")) != -1)
	{
		switch(opt)
		{
			case 'f':
				mfpath = optarg;
				break;

			case 'j':
				maxthreads = atoi(optarg);
				break;

			default:
				printf("Invalid args\n");
		}
	}
	//if makefile not provided, check if it can be found
	if(!mfpath)
	{
		if(access("./makefile", F_OK) == 0)
			mfpath = "./makefile";
		else if(access("./Makefile", F_OK) == 0)
			mfpath = "./Makefile";
		else
		{
			printf("No makefile found\n");
			return 1;
		}
	}

	//printf("makefile=%s; thread count=%d; optind=%d\n", mfpath, threadct, optind);
	int targetct = 0;
    int no_target = 0;
	//copy targets into array
	if (optind >= argc) {
        //printf("No targets specified. Using \'all\'.\n");
        no_target = 1;
    } else {
    	while(optind < argc)
    	{
   			strcpy(targets[targetct], argv[optind]);
   			//printf("target argument = %s\n", targets[targetct]);
   			targetct++;
   			optind++;
   		}
        //strcpy(targets[targetct], \0');
   	}
   	for(i=targetct; i<50; i++)
    {
        free(targets[i]);
        targets[i] = NULL;
    }

   	/*	PART 2	*/
   	parser_parse_makefile(mfpath, targets, parsed_new_target, parsed_new_dependency, parsed_new_command);
    rule_ct = queue_size(rule_list);
    if(no_target)
    {
        char* first_target = ((rule_t*)queue_at(rule_list, 0))->target;
        targets[targetct] = malloc(50*sizeof(char));
        strcpy(targets[targetct], first_target);
        targetct++;
    }
    //printf("Rule count: %d\n", rule_ct);

    /*  PART 3  */
    queue_t target_q;
    queue_init(&target_q);
    i=0;
    while(targets[i])
    {
        queue_enqueue(&target_q, (void*)targets[i]);
        i++;
    }

    if(threadct < maxthreads)
    {
        pthread_t threads[maxthreads];
        for(i=0; i<maxthreads; i++)
        {
            threadct++;
            pthread_create(&threads[i], NULL, &thread_dispatch, (void*)&target_q);
        }
        for(i=0; i<maxthreads; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }
    /* FREE STUFF */
    while(queue_size(rule_list)>0)
    {
        rule_t* curr_rule = queue_dequeue(rule_list);
        while(queue_size(curr_rule->deps)>0)
            free(queue_dequeue(curr_rule->deps));
        free(curr_rule->target);
        rule_destroy(curr_rule);
        free(curr_rule);
    }
    queue_destroy(rule_list);
    free(rule_list);

    queue_destroy(&target_q);
    queue_destroy(&working);
    queue_destroy(&satisfied);
    for(i=0; i<50; i++)                          
    { 
        free(targets[i]);
    }
    free(targets);
    
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&sat_lock);
    pthread_mutex_destroy(&work_lock);
    pthread_mutex_destroy(&queue_lock);
}
