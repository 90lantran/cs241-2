/** @file log.c */
#include <stdlib.h>
#include <string.h>
#include "log.h"



/**
 * Initializes the log.
 *
 * You may assuem that:
 * - This function will only be called once per instance of log_t.
 * - This function will be the first function called per instance of log_t.
 * - All pointers will be valid, non-NULL pointer.
 *
 * @returns
 *   The initialized log structure.
 */
void log_init(log_t *l) {
    l->start = l;
    l->next = NULL;
    l->item = NULL;
    l->size = 0;
}

/**
 * Frees all internal memory associated with the log.
 *
 * You may assume that:
 * - This function will be called once per instance of log_t.
 * - This funciton will be the last function called per instance of log_t.
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure to be destoryed.
 */
void log_destroy(log_t* l) {
    log_t* start = l->start;
    log_t* curr = start;
    log_t* next = curr->next;
    while(next)
    {
        free(curr->item);
        free(curr);
        curr = next;
        next = next->next;
    }
    free(curr->item);
    free(curr);
}

/**
 * Push an item to the log stack.
 *
 *
 * You may assume that:
* - All pointers will be valid, non-NULL pointer.
*
 * @param l
 *    Pointer to the log data structure.
 * @param item
 *    Pointer to a string to be added to the log.
 */
void log_push(log_t* l, const char *item) {
    log_t* start = l->start;
    start->size++;
    log_t* curr = start->next;
    log_t* logItem = malloc(sizeof(log_t));
    logItem->item = malloc(strlen(item)+1);

    unsigned long i;
    for (i = 0; i < strlen(item)+1; i++)
    {
        logItem->item[i] = item[i];
    }
    logItem->start = start;
    if(curr)
        logItem->next = curr;
    else
        logItem->next = NULL;
    start->next = logItem;
}


/**
 * Preforms a newest-to-oldest search of log entries for an entry matching a
 * given prefix.
 *
 * This search starts with the most recent entry in the log and
 * compares each entry to determine if the query is a prefix of the log entry.
 * Upon reaching a match, a pointer to that element is returned.  If no match
 * is found, a NULL pointer is returned.
 *
 *
 * You may assume that:
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure.
 * @param prefix
 *    The prefix to test each entry in the log for a match.
 *
 * @returns
 *    The newest entry in the log whose string matches the specified prefix.
 *    If no strings has the specified prefix, NULL is returned.
 */
char *log_search(log_t* l, const char *prefix) {
    size_t clen = strlen(prefix);
    log_t* curr = l->start->next;
    while(curr)
    {
        if(strncmp(prefix, curr->item, clen) == 0)
            return curr->item;
        curr = curr->next;
    }
    return NULL;
}