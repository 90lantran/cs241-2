/** @file libdictioanry.c */

/*
 * Machine Problem #1
 * CS 241
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdictionary.h"

const int NO_KEY_EXISTS  = 1; /**< Return value if no key exists in the dictionary. @see dictionary_get() */
const int KEY_EXISTS     = 2; /**< Return value if a key exists in the dictionary. @see dictionary_add() */ 
const int ILLEGAL_FORMAT = 3; /**< Return value if the format of the input is illegal. @see dictionary_parse() */


/**
 * Initializes the dictionary.  (If your data structure does not require any
 * initialization logic, this function may be empty.)
 *
 * You may assume that:
 * - This function will only be called once per dicitonary_t instance.
 * - This function will be the first function called on each dictionary_t instance.
 * - The dictionary pointer will be valid, non-NULL pointer.
 *
 * @param d
 *   A pointer to the dictionary.  Since all values are passed by value in C,
 *   you should never directly modify the parameter d.  For example:
 *   @code
 *     d = malloc( sizeof(dictionary_t) );
 *     ...
 *   @endcode
 *   ...will not change d when d is returned to the caller.  Instead, you
 *   should modify only the members of the structure that is pointed to by d,
 *   such as:
 *   @code
 *     d->entry = malloc( sizeof(dictionary_entry_t) );
 *     d->entry->value = ...
 *     ...
 *   @endcode
 */
void dictionary_init(dictionary_t *d)
{
    d->entry = malloc( sizeof(dictionary_entry_t) );
    d->entry->first = NULL;
}


/**
 * Adds the key-value pair (key, value) to the dictionary, if and only if
 * the dictionary does not already contain a key with the same name as key.
 * This function does NOT make a copy of the key or value.  (You should NOT
 * use strcpy() in the function at all.)
 *
 * You may assume that:
 * - The stirngs key and value will not be modified outside of the dictionary.
 * - The parameters will be valid, non-NULL pointers.
 *
 * @param d
 *   A pointer to an initalized dictionary data structure.
 * @param key
 *   The key to be added to the dictionary
 * @param value
 *   The value to be assoicated with the key in the dictionary.
 *
 * @retval 0
 *   Success
 * @retval KEY_EXISTS
 *   The dictionary already contains they specified key.
 */
int dictionary_add(dictionary_t *d, const char *key, const char *value)
{
    dictionary_entry_t *start = d->entry;
	dictionary_t *search = d->entry->first;
    if(search != NULL){
        if(strcmp(search->key, key) == 0)
            return KEY_EXISTS;
        while(search->next)
        {
            if(strcmp(search->next->key, key) == 0)
                return KEY_EXISTS;
            search = search->next;
        }
        search->next = malloc( sizeof(dictionary_t) );
        search = search->next;
    }
    else
    {
        search = malloc( sizeof(dictionary_t) );
        start->first = search;
    }
    search->next = NULL;
    search->key = key;
    search->val = value;
    search->entry = start;
    return 0;
}


/**
 * Parses the key_value string and add the parsed key and value to the
 * dictionary. This function must make a call to dictionary_add() when
 * adding to the dictionary.
 *
 * The format of the key_value will be the format of an HTTP header
 * (@link http://www.w3.org/Protocols/HTTP/1.0/draft-ietf-http-spec.html#Message-Headers 
 * you can read more aboud the headers here, but this is not necessary to understand @endlink),
 * where the contents will be the KEY (one or more non-colon characters), a colon,
 * a single space, and the rest will be the VALUE. (While the KEY cannot be an empty
 * string, it is possible for the VALUE to be an empty string.)
 *
 * This function should NOT copy key_value and should NOT create any additional memory to store
 * the KEY and VALUE pieces of the string.  Instead, this function should modify the key_value
 * string in-place, if necessary.  If the function fails, key_value should be unmodified.
 *
 * Valid inputs:
 * - "Host: www.cs.uiuc.edu"
 * - "MyKey: MyValue"
 * - "a: b"
 * - "SomeLongKey: ", where there is a trailing space after the colon
 * - "Strange: but:this:is:okay: and: so: is: this"
 * - "e: :e"
 * - "This is not common: but it's not illegal"
 * - "Strange:   case", where the value would be "  case"
 *
 * Illegal inputs:
 * - ": www.cs.uiuc.edu", since the KEY is zero-length
 * - "MyKey, MyValue", since no colon, space exists
 * - "a:b", since no colon, space exists.
 * - "a:: b", since the first colon isn't followed by a space.
 *
 * You may assume that:
 * - All whitespace surrounding key_value has been removed.
 * - The parameters will be valid, non-NULL pointers.
 *
 * @param d
 *   A pointer to an initalized dictionary data structure.
 * @param key_value
 *   The key-value pair that is to be parsed and added to the dictionary.
 *   
 * @retval 0 
 *   Success (the key_value was parsed and added to the dictionary)
 * @reval KEY_EXISTS
 *   The dictionary already contains a key with the same name as the KEY in key_value
 * @retval ILLEGAL_FORMAT
 *   The format of key_value is illegal.
 */
int dictionary_parse(dictionary_t *d, char *key_value)
{
	char *key, *val;
    key = key_value;
    int len = strlen(key_value);
    int done = 0;
    if(key_value[0] == ':')
        return ILLEGAL_FORMAT;
    for(int i=0; i<len && done==0; i++)
    {
        if(key_value[i] == ':' && key_value[i+1] == ' ')
        {
            key_value[i] = '\0';
            if(i+2<len)
            {
                val = key_value+i+2;
            }
            else
            {
                val = NULL;
            }
            done = 1;
        }
    }
    if(done == 0)
        return ILLEGAL_FORMAT;
    return dictionary_add(d, key, val);
}


/**
 * Returns the value of the key-value element for a specific key.
 * If the key does not exist, this function returns NULL. 
 *
 * You may assume that:
 * - The parameters will be valid, non-NULL pointers.
 *
 * @param d
 *   A pointer to an initalized dictionary data structure.
 * @param key
 *   The key to lookup in the dictionary.
 *   
 * @return
 *  - the value of the key-value element, if the key exists in the dictionary
 *  - NULL, otherwise.
 */
const char *dictionary_get(dictionary_t *d, const char *key)
{
    dictionary_t *search = d->entry->first;
    if(search != NULL){
        if(*(search->key) == *key)
            return search->val;
        while(search->next)
        {
            if(*(search->next->key) == *key)
                return search->next->val;
            search = search->next;
        }
    }
	return NULL;
}


/**
 * Removes the key-value pair for a given key from the dictionary, if it exists.
 *
 * You may assume that:
 * - The parameters will be valid, non-NULL pointers.
 *
 * @param d
 *   A pointer to an initalized dictionary data structure.
 * @param key
 *   The key to remove from the dictionary.
 *   
 * @retval 0
 *   Success.
 * @retval NO_KEY_EXISTS
 *   The dictionary did not contain key.
 */
int dictionary_remove(dictionary_t *d, const char *key)
{
	dictionary_entry_t *entry = d->entry;
    if(entry->first && entry->first->next)
    {
        dictionary_t *follow = entry->first;
        dictionary_t *search = follow->next;
        while(search->next)
        {
            if(strcmp(search->key, key) == 0)
            {
                follow->next = search->next;
                free(search);
                return 0;
            }
            follow = search;
            search = search->next;
        }
        if(strcmp(search->key, key) == 0)
        {
            follow->next = NULL;
            free(search);
            return 0;
        }
    }
    if(strcmp(entry->first->key, key) == 0)
    {
        free(entry->first);
        entry->first = NULL;
        return 0;
    }

    return NO_KEY_EXISTS;
}


/**
 * Frees any memory associated with the dictionary.
 *
 * You may assume that:
 * - This function will only be called once per dicitonary_t instance.
 * - This function will be the last function called on each dictionary_t instance.
 * - The dictionary pointer will be valid, non-NULL pointer.
 *
 * (Since <tt>d</tt> was provided for you by the programmer and not something you
 *  created, you should NOT free <tt>d</tt> itself.  Only free member elements of
 *  the data structure.)
 *
 * @param d
 *   A pointer to an initalized dictionary data structure.
 */
void dictionary_destroy(dictionary_t *d)
{
    dictionary_entry_t *entry = d->entry;
    while(entry->first && entry->first->next)
    {
        dictionary_t *follow = entry->first;
        dictionary_t *search = follow->next;
        while(search->next)
        {
            follow = search;
            search = search->next;
        }
        free(search);
        follow->next = NULL;
    }
    free(entry);
}

void dict_print(dictionary_t *d)
{
    dictionary_entry_t *entry = d->entry;
    if(!entry->first) printf("Empty\n");
    dictionary_t *iter = entry->first;
    while(iter)
    {
        printf("%s: %s\n",iter->key, iter->val);
        iter = iter->next;
    }
    printf("\n");
}
