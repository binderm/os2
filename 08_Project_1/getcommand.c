#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "getcommand.h"
#include "command.h"
#include "util.h"

#define BUFINC   50
#define PIPE '|'
#define IN '<'
#define OUT '>'

static char *_getline(FILE *);
static commandlist *parseline(char *);
static command *parsecommand(char *);
static int get_redirects(char *, char **, char **);

commandlist * getcommandlist(FILE *stream)
{
   char * line = _getline(stream);
   commandlist *clist;
   if (line == NULL)
   {
      return NULL;
   }
   clist = parseline(line);
   free(line);
   return clist;
}

static void clearline(FILE *stream)
{
   int c;
   while ((c = fgetc(stream)) != EOF)
   {
      if (c == '\n')
         break;
   }
}

static char *_getline(FILE *stream)
{
   char *line = NULL;
   int len = 0;

   do
   {
      len += BUFINC;
      line = (char *)safe_realloc(line, len + 1);
      if (fgets(line + len - BUFINC, BUFINC + 1, stream) == NULL)
      {
         if (feof(stream))
         {
            free(line);
            return NULL;
         }
         else
         {
            free(line);
            fprintf(stderr, "error on input\n");
            clearline(stream);
            return NULL;
         }
      }
   } while (line[strlen(line) - 1] != '\n');

   line[strlen(line) - 1] = '\0';
   return line;
}

static void parseError(char *msg)
{
   fprintf(stderr, "%s\n", msg);
}

static commandlist *parseline(char *line)
{
   char *cmdstr, *next;
   commandlist *clist;
   command *cmd;

   clist = new_commandlist();

   next = line;
   while (next != NULL && *next != '\0')
   {
      cmdstr = next;
      next = strchr(cmdstr, PIPE);
      if (next != NULL)
      {
         *next = '\0';
         next++;
      }
      cmd = parsecommand(cmdstr);
      if (cmd == NULL)
      {
         parseError("empty pipeline stage");
         delete_commandlist(clist);
         free(clist);
         return NULL;
      }
      insert_command(clist, cmd);
   }
   if (next != NULL && *next == '\0')
   {
      if (clist->head != NULL)
      {
         parseError("empty pipeline stage");
      }
      delete_commandlist(clist);
      free(clist);
      return NULL;
   }

   return clist;
}

command *parsecommand(char *cmdstr)
{
   char *stdinstart, *stdoutstart;
   char *str;
   command *cmd = new_command();

   if (get_redirects(cmdstr, &stdinstart, &stdoutstart))
   {
      delete_command(cmd);
      free(cmd);
      return NULL;
   }

   cmd->in = stdinstart ? safe_strdup(stdinstart) : NULL;
   cmd->out = stdoutstart ? safe_strdup(stdoutstart) : NULL;

   /* since we're simplifying things by allowing redirection only after all
      arguments, we now know that cmdstr refers to only the command and its
      arguments
   */

   str = strtok(cmdstr, " \t");
   if (str == NULL)
   {
      delete_command(cmd);
      free(cmd);
      return NULL;
   }
   cmd->cmd = safe_strdup(str);

   while ((str = strtok(NULL, " \t")) != NULL)
   {
      insert_last(cmd->args, safe_strdup(str));
   }

   return cmd;
}

static int get_redirects(char *cmdstr, char **stdinstart, char **stdoutstart)
{
   *stdinstart = strchr(cmdstr, IN);
   *stdoutstart = strchr(cmdstr, OUT);
   if (*stdinstart != NULL)
   {
      **stdinstart = '\0';
      (*stdinstart)++;
   }
   if (*stdoutstart != NULL)
   {
      **stdoutstart = '\0';
      (*stdoutstart)++;
   }
   if (*stdinstart != NULL && (strchr(*stdinstart, IN) != NULL
       || (*stdoutstart != NULL && strchr(*stdoutstart, IN) != NULL)))
   {
      fprintf(stderr, "Ambiguous input redirect.\n");
      return -1;
   }
   if (*stdoutstart != NULL && (strchr(*stdoutstart, OUT) != NULL
       || (*stdinstart != NULL && strchr(*stdinstart, OUT) != NULL)))
   {
      fprintf(stderr, "Ambiguous output redirect.\n");
      return -1;
   }

   /* point the redirects to the filename only */
   if (*stdinstart != NULL)
   {
      *stdinstart = strtok(*stdinstart, " \t");
      if (*stdinstart == NULL)
      {
         fprintf(stderr, "Missing name for redirect.\n");
         return -1;
      }
      if (strtok(NULL, " \t") != NULL)
      {
         fprintf(stderr, "Arguments after redirect.\n");
         return -1;
      }
   }
   if (*stdoutstart != NULL)
   {
      *stdoutstart = strtok(*stdoutstart, " \t");
      if (*stdoutstart == NULL)
      {
         fprintf(stderr, "Missing name for redirect.\n");
         return -1;
      }
      if (strtok(NULL, " \t") != NULL)
      {
         fprintf(stderr, "Arguments after redirect.\n");
         return -1;
      }
   }

   return 0;
}
