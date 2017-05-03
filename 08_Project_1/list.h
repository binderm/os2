#ifndef LIST
#define LIST

struct listnode
{
   char * str;
   struct listnode *next;
};

struct list
{
   int len;
   struct listnode *head;
   struct listnode *tail;
};

void delete_list(struct list *l);
void insert_last(struct list *l, char *str);

#endif
