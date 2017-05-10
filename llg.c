#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/queue.h>

#define FILEBUF 4096
#define INITIAL_SIZE 16

static char const WHITESPACE[] = "\t\r\n\v";

typedef struct idx_list_node {
    ssize_t idx;
    SLIST_ENTRY(idx_list_node) entries;
} idx_list_node_t;

typedef SLIST_HEAD(idx_list_head, idx_list_node) idx_list_head_t;

char *load_file(FILE *f) {
    char buf[4096];
    ssize_t n;
    char *str = NULL;
    size_t len = 0;
    int fn = fileno(f);
    while (n = read(fn, buf, sizeof buf)) {
        if (n < 0) {
            if (errno == EAGAIN)
                continue;
            perror("read");
            break;
        }
        str = realloc(str, len + n + 1);
        memcpy(str + len, buf, n);
        len += n;
        str[len] = '\0';
    }
    return str;
}

static int
cmpstringp(const void *p1, const void *p2)
{
    return strcmp(* (char * const *) p1, * (char * const *) p2);
}

bool *visited;
ssize_t *toppath;
ssize_t *stack;
ssize_t toppath_depth;

static void __traverse_paths_recurse(idx_list_head_t graph[], ssize_t graph_size, ssize_t pos, ssize_t depth) {
    idx_list_node_t *e;

    stack[depth-1] = pos;
    visited[pos] = true;

    if (toppath_depth < depth) {
        toppath_depth = depth;
        memcpy(toppath, stack, depth * sizeof(ssize_t));

    }
    SLIST_FOREACH(e, &graph[pos], entries) {
        if (!visited[e->idx]) {
            __traverse_paths_recurse(graph, graph_size, e->idx, depth + 1);
        }
    }
    visited[pos] = false;
}

ssize_t *traverse_paths(idx_list_head_t graph[], ssize_t graph_size) {
    ssize_t i;
    visited = calloc(graph_size, sizeof(bool));
    toppath = calloc(graph_size, sizeof(size_t));
    stack = calloc(graph_size, sizeof(size_t));
    toppath_depth = 0;
    for (i=0; i < graph_size; i++)
        __traverse_paths_recurse(graph, graph_size, i, 1);
    toppath[toppath_depth+1] = -1;
    free(stack);
    free(visited);
    return toppath;
}

int main(int argc, char** argv) {
    ssize_t i, j;
    char *text = load_file(stdin);

    ssize_t tokens_cap = INITIAL_SIZE;
    ssize_t tokens_count = 0;
    char **tokens = realloc(NULL, tokens_cap * sizeof(char *));
    if (!tokens) {
        perror("Unable to allocate memory");
        abort();
    }
    char *token;

    token = strtok(text, WHITESPACE);
    while (token) {
        if (tokens_count + 1 > tokens_cap) {
            tokens_cap *= 2;
            tokens = realloc(tokens, tokens_cap * sizeof(char *));
            if (!tokens) {
                perror("Unable to allocate memory");
                abort();
            }
        }
        tokens[tokens_count] = token;
        tokens_count++;
        token = strtok(NULL, WHITESPACE);
    }

    if (tokens_count > 1) {
        qsort(tokens, tokens_count, sizeof (char *), cmpstringp);
        for (i=1, j=0; i<tokens_count; i++) {
            if (strcmp(tokens[i], tokens[j]) != 0) {
                j++;
                tokens[j] = tokens[i];
            }
        }
        tokens_count = j + 1;
        tokens = realloc(tokens, tokens_count * sizeof(char *));
        if (!tokens) {
            perror("Unable to allocate memory");
            abort();
        }
    }

    idx_list_head_t graph[tokens_count];
    for (i=0; i < tokens_count; i++) {
        SLIST_INIT(&graph[i]);
    }

    idx_list_node_t *e;
    for (i=0; i < tokens_count; i++) {
        char lastletter = tokens[i][strlen(tokens[i])-1];
        for (j=0; j < tokens_count; j++) {
            if (i==j) continue;
            char firstletter = tokens[j][0];
            if (lastletter == firstletter) {
                e = malloc(sizeof(idx_list_node_t));
                e->idx = j;
                SLIST_INSERT_HEAD(&graph[i], e, entries);
            }
        }
    }

    traverse_paths(graph, tokens_count);
    for (i=0; i < toppath_depth && toppath[i] >= 0; i++) {
        puts(tokens[toppath[i]]);
    }

    for (i=0; i < tokens_count; i++) {
		while (!SLIST_EMPTY(&graph[i])) {           /* List Deletion. */
			e = SLIST_FIRST(&graph[i]);
			SLIST_REMOVE_HEAD(&graph[i], entries);
			free(e);
		}
    }
    free(tokens);
    free(text);
}
